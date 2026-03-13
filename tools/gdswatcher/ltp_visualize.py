#!/usr/bin/env python3
"""
ltp_visualize.py — LTP session timeline visualizer for ION.

Parses a watch character log file produced by gdswatcher.c (with EWCHAR
enabled) and generates:
  1. A per-session timeline plot (PNG)
  2. A statistics summary (stdout + CSV)
  3. A retransmission histogram (PNG)

Usage:
    python3 ltp_visualize.py <watch_log_file> [--out-dir <dir>]

Requirements:
    Python 3.6+, matplotlib

Copyright (c) 2026, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
"""

import argparse
import csv
import os
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class SessionEvent:
    """A single watch-character event associated with an LTP session."""
    timestamp: float        # epoch seconds (with microseconds)
    session_id: str         # session number as string
    event_type: str         # parsed event category (see EVENT_TYPES)
    raw: str                # raw watch string from log
    direction: str = ""     # "tx" or "rx" or ""


# Mapping of regex pattern -> (event_type, direction) for EWCHAR strings.
# Patterns are tried in order; first match wins.
EWCHAR_PATTERNS: List[Tuple[str, str, str]] = [
    # --- Transmitted segments (g) ---
    (r'^\(rcp(\d+)\)g$',   'retx_checkpoint',  'tx'),
    (r'^\(cp(\d+)\)g$',    'checkpoint',        'tx'),
    (r'^\(ds(\d+)\)g$',    'data_segment',      'tx'),
    (r'^\(prs(\d+)\)g$',   'pos_report',        'tx'),
    (r'^\(nrs(\d+)\)g$',   'neg_report',        'tx'),
    (r'^\(rrs(\d+)\)g$',   'retx_report',       'tx'),
    (r'^\(ras(\d+)\)g$',   'report_ack',        'tx'),
    (r'^\(cs(\d+)\)g$',    'cancel_sender',     'tx'),
    (r'^\(cr(\d+)\)g$',    'cancel_receiver',   'tx'),
    (r'^\(ca(\d+)\)g$',    'cancel_ack',        'tx'),

    # --- Received segments (s) ---
    (r'^\(d(\d+)\)s$',     'data_segment',      'rx'),
    (r'^\(rs(\d+)\)s$',    'report_segment',    'rx'),
    (r'^\(ras(\d+)\)s$',   'report_ack',        'rx'),
    (r'^\(cs(\d+)\)s$',    'cancel_sender',     'rx'),
    (r'^\(cas(\d+)\)s$',   'cancel_sender_ack', 'rx'),
    (r'^\(cr(\d+)\)s$',    'cancel_receiver',   'rx'),
    (r'^\(car(\d+)\)s$',   'cancel_receiver_ack', 'rx'),

    # --- Control events ---
    (r'^\((\d+)\)h$',      'session_complete',  ''),
    (r'^\((\d+)\)@$',      'neg_ack_retx',      ''),
    (r'^\((\d+)\)=$',      'checkpoint_timeout', ''),
    (r'^\((\d+)\)\+$',     'report_timeout',    ''),
    (r'^\((\d+)\)f$',      'session_finalized', ''),
]

# Non-session events (single characters without EWCHAR session numbers).
BARE_EVENTS = {
    'e': 'retx_queued',
    'd': 'block_appended',
    't': 'block_delivered',
    '{': 'export_canceled',
    '}': 'import_canceled_remote',
    '[': 'import_canceled_local',
    ']': 'export_canceled_remote',
}

# BP events (for informational display).
BP_PATTERN = re.compile(
    r'^\(([^)]+)\)([abcm#!])$'
)

# Compile EWCHAR patterns.
_compiled_ewchar = [(re.compile(pat), etype, direction)
                     for pat, etype, direction in EWCHAR_PATTERNS]


@dataclass
class SessionStats:
    """Aggregated statistics for one LTP session."""
    session_id: str
    first_time: float = float('inf')
    last_time: float = 0.0
    data_segments_tx: int = 0
    data_segments_rx: int = 0
    checkpoints_tx: int = 0
    retx_checkpoints_tx: int = 0
    reports_tx: int = 0   # pos + neg + retx
    reports_rx: int = 0
    report_acks_tx: int = 0
    report_acks_rx: int = 0
    checkpoint_timeouts: int = 0
    report_timeouts: int = 0
    neg_ack_retx: int = 0
    cancel_events: int = 0
    outcome: str = "unknown"  # "complete", "canceled", "finalized", "unknown"
    events: List[SessionEvent] = field(default_factory=list)

    @property
    def duration(self) -> float:
        if self.last_time > self.first_time:
            return self.last_time - self.first_time
        return 0.0

    @property
    def total_retx(self) -> int:
        return (self.retx_checkpoints_tx + self.checkpoint_timeouts
                + self.report_timeouts + self.neg_ack_retx)


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

def parse_log(filepath: str) -> Tuple[Dict[str, SessionStats], List[dict]]:
    """Parse a gdswatcher log file.

    Returns:
        sessions: dict mapping session_id -> SessionStats
        bare_events: list of dicts for non-session events
    """
    sessions: Dict[str, SessionStats] = {}
    bare_events: List[dict] = []

    line_re = re.compile(r'^(\d+\.\d+)\s+(.+)$')

    with open(filepath, 'r') as f:
        for lineno, line in enumerate(f, 1):
            line = line.rstrip('\n')
            m = line_re.match(line)
            if not m:
                continue

            timestamp = float(m.group(1))
            watch_str = m.group(2)

            # Try EWCHAR patterns.
            matched = False
            for pat, etype, direction in _compiled_ewchar:
                em = pat.match(watch_str)
                if em:
                    sid = em.group(1)
                    evt = SessionEvent(
                        timestamp=timestamp,
                        session_id=sid,
                        event_type=etype,
                        raw=watch_str,
                        direction=direction,
                    )
                    _record_event(sessions, sid, evt)
                    matched = True
                    break

            if matched:
                continue

            # Try bare (non-session) events.
            stripped = watch_str.strip()
            if stripped in BARE_EVENTS:
                bare_events.append({
                    'timestamp': timestamp,
                    'event_type': BARE_EVENTS[stripped],
                    'raw': watch_str,
                })
                continue

            # Try BP events (informational).
            bm = BP_PATTERN.match(watch_str)
            if bm:
                bare_events.append({
                    'timestamp': timestamp,
                    'event_type': f'bp_{bm.group(2)}',
                    'raw': watch_str,
                    'bp_info': bm.group(1),
                })
                continue

            # Unknown line — skip silently.

    return sessions, bare_events


def _record_event(sessions: Dict[str, SessionStats],
                  sid: str, evt: SessionEvent) -> None:
    """Add an event to the appropriate session, updating counters."""
    if sid not in sessions:
        sessions[sid] = SessionStats(session_id=sid)
    ss = sessions[sid]
    ss.events.append(evt)

    if evt.timestamp < ss.first_time:
        ss.first_time = evt.timestamp
    if evt.timestamp > ss.last_time:
        ss.last_time = evt.timestamp

    et = evt.event_type
    d = evt.direction

    if et == 'data_segment' and d == 'tx':
        ss.data_segments_tx += 1
    elif et == 'data_segment' and d == 'rx':
        ss.data_segments_rx += 1
    elif et == 'checkpoint' and d == 'tx':
        ss.checkpoints_tx += 1
    elif et == 'retx_checkpoint' and d == 'tx':
        ss.retx_checkpoints_tx += 1
    elif et in ('pos_report', 'neg_report', 'retx_report') and d == 'tx':
        ss.reports_tx += 1
    elif et == 'report_segment' and d == 'rx':
        ss.reports_rx += 1
    elif et == 'report_ack' and d == 'tx':
        ss.report_acks_tx += 1
    elif et == 'report_ack' and d == 'rx':
        ss.report_acks_rx += 1
    elif et == 'checkpoint_timeout':
        ss.checkpoint_timeouts += 1
    elif et == 'report_timeout':
        ss.report_timeouts += 1
    elif et == 'neg_ack_retx':
        ss.neg_ack_retx += 1
    elif et in ('cancel_sender', 'cancel_receiver', 'cancel_ack'):
        ss.cancel_events += 1

    # Determine outcome.
    if et == 'session_complete':
        ss.outcome = 'complete'
    elif et == 'session_finalized':
        if ss.outcome == 'unknown':
            ss.outcome = 'finalized'
    elif et in ('cancel_sender', 'cancel_receiver'):
        ss.outcome = 'canceled'


# ---------------------------------------------------------------------------
# Statistics output
# ---------------------------------------------------------------------------

def print_summary(sessions: Dict[str, SessionStats],
                  bare_events: List[dict]) -> None:
    """Print a text summary to stdout."""
    if not sessions:
        print("No LTP sessions found in log file.")
        return

    sorted_sessions = sorted(sessions.values(),
                             key=lambda s: s.first_time)

    t0 = sorted_sessions[0].first_time

    print(f"\n{'='*78}")
    print(f"  LTP Session Summary — {len(sessions)} sessions")
    print(f"{'='*78}\n")

    hdr = (f"{'Session':>8s}  {'Start':>10s}  {'Dur(s)':>8s}  "
           f"{'DataTx':>6s}  {'DataRx':>6s}  {'CkPt':>4s}  "
           f"{'ReCkPt':>6s}  {'RptTx':>5s}  {'RptRx':>5s}  "
           f"{'CkTmo':>5s}  {'RpTmo':>5s}  {'Retx':>4s}  {'Outcome':>10s}")
    print(hdr)
    print('-' * len(hdr))

    total_complete = 0
    total_canceled = 0
    total_retx = 0

    for ss in sorted_sessions:
        start_rel = ss.first_time - t0
        print(f"{ss.session_id:>8s}  {start_rel:>10.3f}  "
              f"{ss.duration:>8.3f}  "
              f"{ss.data_segments_tx:>6d}  {ss.data_segments_rx:>6d}  "
              f"{ss.checkpoints_tx:>4d}  {ss.retx_checkpoints_tx:>6d}  "
              f"{ss.reports_tx:>5d}  {ss.reports_rx:>5d}  "
              f"{ss.checkpoint_timeouts:>5d}  {ss.report_timeouts:>5d}  "
              f"{ss.total_retx:>4d}  {ss.outcome:>10s}")

        if ss.outcome == 'complete':
            total_complete += 1
        elif ss.outcome == 'canceled':
            total_canceled += 1
        total_retx += ss.total_retx

    print(f"\n  Completed: {total_complete}   "
          f"Canceled: {total_canceled}   "
          f"Other: {len(sessions) - total_complete - total_canceled}")
    print(f"  Total retransmission events: {total_retx}")

    # Bare event summary.
    if bare_events:
        print(f"\n  Non-session events:")
        counts = defaultdict(int)
        for be in bare_events:
            counts[be['event_type']] += 1
        for etype, count in sorted(counts.items()):
            print(f"    {etype}: {count}")

    print()


def write_csv(sessions: Dict[str, SessionStats], filepath: str) -> None:
    """Write per-session statistics to a CSV file."""
    sorted_sessions = sorted(sessions.values(),
                             key=lambda s: s.first_time)

    fields = [
        'session_id', 'first_time', 'last_time', 'duration',
        'data_segments_tx', 'data_segments_rx',
        'checkpoints_tx', 'retx_checkpoints_tx',
        'reports_tx', 'reports_rx',
        'report_acks_tx', 'report_acks_rx',
        'checkpoint_timeouts', 'report_timeouts',
        'neg_ack_retx', 'total_retx',
        'cancel_events', 'outcome',
    ]

    with open(filepath, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for ss in sorted_sessions:
            writer.writerow({
                'session_id': ss.session_id,
                'first_time': f"{ss.first_time:.6f}",
                'last_time': f"{ss.last_time:.6f}",
                'duration': f"{ss.duration:.6f}",
                'data_segments_tx': ss.data_segments_tx,
                'data_segments_rx': ss.data_segments_rx,
                'checkpoints_tx': ss.checkpoints_tx,
                'retx_checkpoints_tx': ss.retx_checkpoints_tx,
                'reports_tx': ss.reports_tx,
                'reports_rx': ss.reports_rx,
                'report_acks_tx': ss.report_acks_tx,
                'report_acks_rx': ss.report_acks_rx,
                'checkpoint_timeouts': ss.checkpoint_timeouts,
                'report_timeouts': ss.report_timeouts,
                'neg_ack_retx': ss.neg_ack_retx,
                'total_retx': ss.total_retx,
                'cancel_events': ss.cancel_events,
                'outcome': ss.outcome,
            })

    print(f"  CSV written to: {filepath}")


# ---------------------------------------------------------------------------
# Visualization
# ---------------------------------------------------------------------------

def plot_timeline(sessions: Dict[str, SessionStats],
                  out_path: str) -> None:
    """Generate a per-session timeline plot."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        import matplotlib.patches as mpatches
    except ImportError:
        print("  WARNING: matplotlib not available, skipping timeline plot.",
              file=sys.stderr)
        return

    sorted_sessions = sorted(sessions.values(),
                             key=lambda s: s.first_time)
    if not sorted_sessions:
        return

    t0 = sorted_sessions[0].first_time

    # Color map for event types.
    colors = {
        'data_segment':        '#4CAF50',  # green
        'checkpoint':          '#2196F3',  # blue
        'retx_checkpoint':     '#FF9800',  # orange
        'pos_report':          '#8BC34A',  # light green
        'neg_report':          '#F44336',  # red
        'retx_report':         '#FF5722',  # deep orange
        'report_segment':      '#00BCD4',  # cyan
        'report_ack':          '#9E9E9E',  # grey
        'cancel_sender':       '#E91E63',  # pink
        'cancel_receiver':     '#9C27B0',  # purple
        'cancel_ack':          '#795548',  # brown
        'session_complete':    '#4CAF50',  # green
        'session_finalized':   '#607D8B',  # blue-grey
        'checkpoint_timeout':  '#FF9800',  # orange
        'report_timeout':      '#FF5722',  # deep orange
        'neg_ack_retx':        '#F44336',  # red
        'cancel_sender_ack':   '#E91E63',  # pink
        'cancel_receiver_ack': '#9C27B0',  # purple
    }

    # Marker map for event types.
    markers = {
        'data_segment':        '.',
        'checkpoint':          'D',
        'retx_checkpoint':     'D',
        'pos_report':          's',
        'neg_report':          'X',
        'retx_report':         's',
        'report_segment':      's',
        'report_ack':          'v',
        'cancel_sender':       'x',
        'cancel_receiver':     'x',
        'cancel_ack':          'x',
        'session_complete':    '*',
        'session_finalized':   'P',
        'checkpoint_timeout':  '^',
        'report_timeout':      '^',
        'neg_ack_retx':        '<',
    }

    # Limit to at most 50 sessions for readability.
    display_sessions = sorted_sessions[:50]
    n = len(display_sessions)

    fig_height = max(4, n * 0.35 + 2)
    fig, ax = plt.subplots(figsize=(14, fig_height))

    y_labels = []
    for i, ss in enumerate(display_sessions):
        y = n - 1 - i
        y_labels.append(f"S{ss.session_id}")

        # Sort events by timestamp to get chronological order.
        sorted_evts = sorted(ss.events, key=lambda e: e.timestamp)

        # Draw a connecting line through all events so the session
        # history is visible as a continuous path.
        if len(sorted_evts) >= 2:
            times = [e.timestamp - t0 for e in sorted_evts]
            ax.plot(times, [y] * len(times), color='#BDBDBD',
                    linewidth=0.8, zorder=1)

        # Plot each event marker on top of the line.
        for evt in sorted_evts:
            t_rel = evt.timestamp - t0
            color = colors.get(evt.event_type, '#000000')
            marker = markers.get(evt.event_type, 'o')
            size = 20 if marker == '.' else 40
            ax.scatter(t_rel, y, c=color, marker=marker, s=size,
                       zorder=3, edgecolors='none', alpha=0.8)

    ax.set_yticks(range(n))
    ax.set_yticklabels(reversed(y_labels), fontsize=7)
    ax.set_xlabel('Time (seconds from start)')
    ax.set_title('LTP Session Timeline')
    ax.grid(axis='x', alpha=0.3)

    # Legend for key event types.
    legend_items = [
        ('data_segment',       'Data segment'),
        ('checkpoint',         'Checkpoint'),
        ('retx_checkpoint',    'Retx checkpoint'),
        ('report_segment',     'Report (rx)'),
        ('neg_report',         'Neg report (tx)'),
        ('checkpoint_timeout', 'Ckpt timeout'),
        ('report_timeout',     'Rpt timeout'),
        ('session_complete',   'Complete'),
        ('cancel_sender',      'Cancel'),
    ]
    handles = []
    for etype, label in legend_items:
        color = colors.get(etype, '#000000')
        marker = markers.get(etype, 'o')
        h = ax.scatter([], [], c=color, marker=marker, s=40, label=label)
        handles.append(h)
    ax.legend(handles=handles, loc='upper right', fontsize=7,
              ncol=3, framealpha=0.8)

    plt.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"  Timeline plot written to: {out_path}")

    if len(sorted_sessions) > 50:
        print(f"  NOTE: Only first 50 of {len(sorted_sessions)} sessions "
              f"are shown in the plot.")


def plot_retx_histogram(sessions: Dict[str, SessionStats],
                        out_path: str) -> None:
    """Generate a histogram of retransmission counts per session."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
    except ImportError:
        return

    retx_counts = [ss.total_retx for ss in sessions.values()]
    if not retx_counts or max(retx_counts) == 0:
        print("  No retransmissions found, skipping histogram.")
        return

    fig, ax = plt.subplots(figsize=(8, 5))
    max_retx = max(retx_counts)
    bins = range(0, max_retx + 2)
    ax.hist(retx_counts, bins=bins, color='#2196F3', edgecolor='white',
            alpha=0.8, align='left')
    ax.set_xlabel('Retransmission events per session')
    ax.set_ylabel('Number of sessions')
    ax.set_title('LTP Retransmission Distribution')
    ax.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"  Retransmission histogram written to: {out_path}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Visualize LTP session timelines from ION watch logs.')
    parser.add_argument('logfile',
                        help='Path to the ion_watch.log file')
    parser.add_argument('--out-dir', default=None,
                        help='Output directory for plots and CSV '
                             '(default: same dir as logfile)')
    args = parser.parse_args()

    if not os.path.isfile(args.logfile):
        print(f"Error: file not found: {args.logfile}", file=sys.stderr)
        sys.exit(1)

    out_dir = args.out_dir or os.path.dirname(os.path.abspath(args.logfile))
    os.makedirs(out_dir, exist_ok=True)

    base = os.path.splitext(os.path.basename(args.logfile))[0]

    print(f"Parsing {args.logfile} ...")
    sessions, bare_events = parse_log(args.logfile)

    print_summary(sessions, bare_events)

    if sessions:
        csv_path = os.path.join(out_dir, f"{base}_stats.csv")
        write_csv(sessions, csv_path)

        timeline_path = os.path.join(out_dir, f"{base}_timeline.png")
        plot_timeline(sessions, timeline_path)

        hist_path = os.path.join(out_dir, f"{base}_retx_histogram.png")
        plot_retx_histogram(sessions, hist_path)


if __name__ == '__main__':
    main()
