'''
bridge_adapter.py: ION TCP4 CLA bridge adapter.

Connects ION's shared memory pool / control channel interface to the
dtn-demo-agent TCPCLv4 implementation.  Runs in-process with the
TCPCL agent using a shared GLib MainLoop.

Usage:
    PYTHONPATH=~/dtn-demo-agent/src:bpv7/tcp4 \\
        python3 -m bridge.bridge_adapter --config bridge_config.yaml

Author: iondev33

Copyright (c) 2026, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
'''

import argparse
import logging
import signal
import ssl
import sys
from io import BytesIO

# Python 3.12 removed ssl.match_hostname (verification is now built into
# SSLSocket.do_handshake).  dtn-demo-agent still references it, so provide
# a no-op shim to avoid AttributeError at runtime.
if not hasattr(ssl, 'match_hostname'):
    ssl.match_hostname = lambda cert, hostname: None

import yaml

from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib as glib

from tcpcl.agent import Agent
from tcpcl.config import Config, ConnectConfig, ListenConfig
from tcpcl.session import ContactHandler

from bridge.bridge_pool import Pool
from bridge.bridge_ctrl import (
    CtrlChannel, CtrlMsg,
    CTRL_SLOT_READY, CTRL_BUNDLE_RECV, CTRL_XFER_SUCCESS,
    CTRL_XFER_FAILURE, CTRL_SLOT_RELEASE,
    CTRL_SESSION_UP, CTRL_SESSION_DOWN, CTRL_KEEPALIVE,
)

LOGGER = logging.getLogger(__name__)

KEEPALIVE_INTERVAL = 15  # seconds
KEEPALIVE_TIMEOUT = 45   # seconds
KEEPALIVE_CHECK_MS = 5000  # GLib timer interval


class BridgeContactHandler(ContactHandler):
    '''ContactHandler subclass that hooks bundle send/receive signals
    to relay data between ION shared memory pools and TCPCLv4 sessions.
    '''

    def __init__(self, bridge, hdl_kwargs, bus_kwargs):
        super().__init__(hdl_kwargs, bus_kwargs)
        self._bridge = bridge

    def send_bundle_finished(self, bid, length, result):
        '''Override: notify the bridge when a TX transfer completes.'''
        super().send_bundle_finished(bid, length, result)
        try:
            self._bridge.on_send_finished(int(bid), str(result))
        except Exception:
            LOGGER.exception('Error in on_send_finished')

    def recv_bundle_finished(self, bid, length, result):
        '''Override: notify the bridge when a complete bundle arrives.'''
        super().recv_bundle_finished(bid, length, result)
        try:
            if str(result) == 'success':
                self._bridge.on_recv_finished(self, int(bid))
        except Exception:
            LOGGER.exception('Error in on_recv_finished')

    def session_state_changed(self, state):
        '''Override: notify the bridge on session state transitions.'''
        super().session_state_changed(state)
        try:
            self._bridge.on_session_state(self, str(state))
        except Exception:
            LOGGER.exception('Error in on_session_state')


class BridgeAgent(Agent):
    '''Agent subclass that creates BridgeContactHandler instances.'''

    def __init__(self, bridge, config, **kwargs):
        self._bridge = bridge
        super().__init__(config, **kwargs)

    def _bind_handler(self, **kwargs):
        '''Override to create BridgeContactHandler instead of
        ContactHandler.'''
        path = self._get_obj_path()
        hdl = BridgeContactHandler(
            bridge=self._bridge,
            hdl_kwargs=kwargs,
            bus_kwargs=dict(conn=self._config.bus_conn,
                            object_path=path)
        )
        self._handlers.append(hdl)
        self._path_to_handler[path] = hdl
        hdl.set_on_close(lambda: self._unbind_handler(hdl))
        self.connection_opened(path)
        return hdl


class BridgeAdapter:
    '''Main bridge: connects ION shared memory pools to TCPCL sessions.

    One BridgeAdapter instance serves one ION duct (one pair of
    TX/RX pools and control channels) and manages one or more
    TCPCLv4 sessions to the configured peer.
    '''

    def __init__(self, config):
        self.duct_name = config['duct_name']

        LOGGER.info('Bridge adapter starting: duct=%s', self.duct_name)

        # --- ION side ---

        tx_pool_name = f'/ion_tcp4_tx_{self.duct_name}'
        rx_pool_name = f'/ion_tcp4_rx_{self.duct_name}'
        tx_ctrl_path = f'/tmp/ion_tcp4/tx_{self.duct_name}.sock'
        rx_ctrl_path = f'/tmp/ion_tcp4/rx_{self.duct_name}.sock'

        self.tx_pool = Pool(tx_pool_name)
        self.rx_pool = Pool(rx_pool_name)
        self.tx_ctrl = CtrlChannel(tx_ctrl_path)
        self.rx_ctrl = CtrlChannel(rx_ctrl_path)

        # Transfer tracking
        self._tx_pending = {}     # tcpcl_transfer_id -> ion_xfer_id
        self._rx_pending = {}     # ion_xfer_id -> rx_slot_index
        self._next_rx_xfer_id = 1

        # --- TCPCL side ---

        tcpcl_config = Config()
        tcpcl_config.node_id = config.get('node_id', '')
        tcpcl_config.tls_enable = config.get('tls_enable', False)
        tcpcl_config.stop_on_close = False

        # TLS configuration
        if config.get('tls_ca_file'):
            tcpcl_config.tls_ca_file = config['tls_ca_file']
        if config.get('tls_cert_file'):
            tcpcl_config.tls_cert_file = config['tls_cert_file']
        if config.get('tls_key_file'):
            tcpcl_config.tls_key_file = config['tls_key_file']

        # Connection mode: active (connect) or passive (listen)
        if config.get('peer_address'):
            tcpcl_config.init_connect.append(
                ConnectConfig(address=config['peer_address'],
                              port=config.get('peer_port', 4556)))
            LOGGER.info('TCPCL active mode: will connect to %s:%d',
                        config['peer_address'], config.get('peer_port', 4556))

        if config.get('listen_address'):
            tcpcl_config.init_listen.append(
                ListenConfig(address=config['listen_address'],
                             port=config.get('listen_port', 4556)))
            LOGGER.info('TCPCL passive mode: will listen on %s:%d',
                        config['listen_address'], config.get('listen_port', 4556))

        self.agent = BridgeAgent(bridge=self, config=tcpcl_config)

        # --- GLib integration ---

        # Watch ION control sockets for incoming messages
        glib.io_add_watch(self.tx_ctrl.fileno(),
                          glib.IO_IN | glib.IO_HUP | glib.IO_ERR,
                          self._on_tx_readable)
        glib.io_add_watch(self.rx_ctrl.fileno(),
                          glib.IO_IN | glib.IO_HUP | glib.IO_ERR,
                          self._on_rx_readable)

        # Periodic keepalive check
        glib.timeout_add(KEEPALIVE_CHECK_MS, self._keepalive_tick)

    def _get_active_handler(self):
        '''Find an active TCPCL session handler.

        :return: A ContactHandler in session, or None.
        '''
        for hdl in self.agent._handlers:
            if hdl._in_sess:
                return hdl
        return None

    # --- TX direction: ION -> TCPCL peer ---

    def _on_tx_readable(self, fd, condition):
        '''GLib callback: TX control socket has data.'''
        if condition & (glib.IO_HUP | glib.IO_ERR):
            LOGGER.error('TX control channel error/hangup')
            return False

        try:
            msg = self.tx_ctrl.recv()
        except ConnectionError:
            LOGGER.error('TX control channel disconnected')
            return False

        if msg.type == CTRL_SLOT_READY:
            self._handle_slot_ready(msg)
        elif msg.type == CTRL_KEEPALIVE:
            self.tx_ctrl.send_keepalive()
        else:
            LOGGER.warning('Unexpected TX ctrl message: %s', msg)

        return True  # keep watching

    def _handle_slot_ready(self, msg):
        '''Process a SLOT_READY: read from TX pool, send via TCPCL.'''
        data, xfer_id = self.tx_pool.read(msg.slot_index)
        if data is None:
            LOGGER.error('TX slot %d not in READY state', msg.slot_index)
            return

        # Release the TX slot immediately (data is in Python memory now)
        self.tx_pool.release(msg.slot_index)

        handler = self._get_active_handler()
        if handler is None:
            LOGGER.warning('No active TCPCL session, failing xfer %d',
                           xfer_id)
            self.tx_ctrl.send(CtrlMsg(
                type=CTRL_XFER_FAILURE, slot_index=0, xfer_id=xfer_id))
            return

        LOGGER.info('TX: sending %d bytes, ion_xfer=%d', len(data), xfer_id)
        tcpcl_id = handler.send_bundle_fileobj(BytesIO(data))
        self._tx_pending[tcpcl_id] = xfer_id

    def on_send_finished(self, tcpcl_transfer_id, result):
        '''Called by BridgeContactHandler when TCPCL transfer completes.'''
        ion_xfer_id = self._tx_pending.pop(tcpcl_transfer_id, None)
        if ion_xfer_id is None:
            LOGGER.warning('send_finished for unknown tcpcl_id=%d',
                           tcpcl_transfer_id)
            return

        if 'success' in result:
            LOGGER.info('TX: xfer %d succeeded', ion_xfer_id)
            self.tx_ctrl.send(CtrlMsg(
                type=CTRL_XFER_SUCCESS, xfer_id=ion_xfer_id))
        else:
            LOGGER.warning('TX: xfer %d failed: %s', ion_xfer_id, result)
            self.tx_ctrl.send(CtrlMsg(
                type=CTRL_XFER_FAILURE, xfer_id=ion_xfer_id))

    # --- RX direction: TCPCL peer -> ION ---

    def on_recv_finished(self, handler, tcpcl_transfer_id):
        '''Called by BridgeContactHandler when a bundle arrives via TCPCL.'''
        # Extract the bundle data from the TCPCL handler
        data = handler.recv_bundle_pop_data(str(tcpcl_transfer_id))
        if isinstance(data, (list, bytearray)):
            data = bytes(data)

        LOGGER.info('RX: received %d bytes via TCPCL', len(data))

        # Acquire an RX pool slot
        rx_slot = self.rx_pool.acquire()
        if rx_slot < 0:
            # All slots full; retry a few times with short delay
            for _ in range(50):
                glib.main_context_default().iteration(False)
                rx_slot = self.rx_pool.acquire()
                if rx_slot >= 0:
                    break

        if rx_slot < 0:
            LOGGER.error('RX: no free slots, dropping bundle')
            return

        # Write bundle into RX pool and notify tcp4cli
        xfer_id = self._next_rx_xfer_id
        self._next_rx_xfer_id += 1

        self.rx_pool.write_and_publish(rx_slot, data, xfer_id)
        self.rx_ctrl.send(CtrlMsg(
            type=CTRL_BUNDLE_RECV, slot_index=rx_slot, xfer_id=xfer_id))
        self._rx_pending[xfer_id] = rx_slot
        LOGGER.info('RX: published to slot %d, xfer=%d', rx_slot, xfer_id)

    def _on_rx_readable(self, fd, condition):
        '''GLib callback: RX control socket has data.'''
        if condition & (glib.IO_HUP | glib.IO_ERR):
            LOGGER.error('RX control channel error/hangup')
            return False

        try:
            msg = self.rx_ctrl.recv()
        except ConnectionError:
            LOGGER.error('RX control channel disconnected')
            return False

        if msg.type == CTRL_SLOT_RELEASE:
            self.rx_pool.release(msg.slot_index)
            # Clean up pending tracking
            to_remove = [k for k, v in self._rx_pending.items()
                         if v == msg.slot_index]
            for k in to_remove:
                del self._rx_pending[k]
        elif msg.type == CTRL_KEEPALIVE:
            self.rx_ctrl.send_keepalive()
        else:
            LOGGER.warning('Unexpected RX ctrl message: %s', msg)

        return True  # keep watching

    # --- Session lifecycle ---

    def on_session_state(self, handler, state):
        '''Called by BridgeContactHandler on session state transitions.'''
        LOGGER.info('TCPCL session state: %s', state)

        if state == 'established':
            try:
                peer_params = handler.get_session_parameters()
                peer_node = str(peer_params.get('peer_nodeid', ''))
            except Exception:
                peer_node = ''

            payload = peer_node.encode('utf-8')
            self.tx_ctrl.send(CtrlMsg(
                type=CTRL_SESSION_UP, payload=payload))
            self.rx_ctrl.send(CtrlMsg(
                type=CTRL_SESSION_UP, payload=payload))

        elif state == 'ending':
            # Fail any pending TX transfers
            for tcpcl_id, ion_xfer_id in list(self._tx_pending.items()):
                LOGGER.warning('Failing in-flight TX xfer %d (session ending)',
                               ion_xfer_id)
                self.tx_ctrl.send(CtrlMsg(
                    type=CTRL_XFER_FAILURE, xfer_id=ion_xfer_id))
            self._tx_pending.clear()

            self.tx_ctrl.send(CtrlMsg(type=CTRL_SESSION_DOWN))
            self.rx_ctrl.send(CtrlMsg(type=CTRL_SESSION_DOWN))

    # --- Keepalive ---

    def _keepalive_tick(self):
        '''GLib timer callback: send keepalives and check timeouts.'''
        for name, ctrl in [('TX', self.tx_ctrl), ('RX', self.rx_ctrl)]:
            if ctrl.check_timeout(KEEPALIVE_TIMEOUT):
                LOGGER.error('%s ctrl keepalive timeout', name)
            if ctrl.keepalive_due(KEEPALIVE_INTERVAL):
                try:
                    ctrl.send_keepalive()
                except Exception:
                    LOGGER.exception('%s ctrl keepalive send failed', name)
        return True  # repeat timer

    # --- Shutdown ---

    def shutdown(self):
        '''Gracefully shut down the bridge.'''
        LOGGER.info('Bridge adapter shutting down')

        # Fail any pending TX transfers
        for tcpcl_id, ion_xfer_id in list(self._tx_pending.items()):
            try:
                self.tx_ctrl.send(CtrlMsg(
                    type=CTRL_XFER_FAILURE, xfer_id=ion_xfer_id))
            except Exception:
                pass
        self._tx_pending.clear()

        # Close TCPCL sessions
        self.agent.shutdown()

        # Close ION connections
        self.tx_ctrl.close()
        self.rx_ctrl.close()
        self.tx_pool.close()
        self.rx_pool.close()


def main():
    parser = argparse.ArgumentParser(
        description='ION TCP4 CLA Bridge Adapter')
    parser.add_argument('--config', required=True,
                        help='Path to bridge configuration YAML file')
    parser.add_argument('--log-level', default='INFO',
                        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR'])
    args = parser.parse_args()

    logging.basicConfig(
        level=getattr(logging, args.log_level),
        format='%(asctime)s %(name)s %(levelname)s: %(message)s')

    with open(args.config) as f:
        config = yaml.safe_load(f)

    # GLib + D-Bus integration (must be before any D-Bus usage)
    DBusGMainLoop(set_as_default=True)

    adapter = BridgeAdapter(config)

    # Handle SIGTERM/SIGINT gracefully
    def sig_handler(signum, frame):
        LOGGER.info('Received signal %d, shutting down', signum)
        adapter.shutdown()

    signal.signal(signal.SIGTERM, sig_handler)
    signal.signal(signal.SIGINT, sig_handler)

    # Run the GLib event loop (handles both TCPCL and ION I/O)
    adapter.agent.exec_loop()


if __name__ == '__main__':
    main()
