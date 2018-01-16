package gov.nasa.jpl.iondtn.services;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.IBinder;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.support.annotation.Nullable;
import android.support.v4.app.NotificationCompat;
import android.support.v4.content.ContextCompat;
import android.text.format.Formatter;
import android.util.Log;

import java.util.HashSet;
import java.util.Set;

import gov.nasa.jpl.iondtn.INodeAdminListener;
import gov.nasa.jpl.iondtn.INodeAdminService;
import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.IpndFileUpdater;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;
import gov.nasa.jpl.iondtn.gui.MainActivity;

/**
 * The NodeAdministrationService class provides an Service that can be
 * used for managing an ION node asynchronously, i.e. to offload the
 * time-consuming procedures to start and stop ION to a dedicated thread.
 *
 * Because the service is just running in a different thread and not a
 * different task, the access to the launched ION instance (in dedicated
 * threads) is also possible from other services running in the same task
 * (i.e. a service providing an interface for sending and receiving bundles).
 *
 * @author Robert Wiewel
 */

public class NodeAdministrationService extends Service
    implements NativeAdapter.OnIonStateChangeListener {

    private final static String TAG = "NodeAdmService";
    private final static int NOTIFICATION_ID = 42;
    private final static String NOTIF_CHANNEL = "iondtn_channel1";
    private Set<INodeAdminListener> listenerSet;
    private NativeAdapter na;

    /**
     * Returns a communication channel to the service The IBinder object
     * allows the communication between the calling app component and the
     * (started) service.
     *
     * @param intent The intent that was used to bind to this service
     * @return An IBinder object used for the communication between the
     * caller and the callee
     */
    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    /**
     * Allows the start of the NodeAdministrationService  by some other
     * component via Intent. If the service was not created before, the
     * {@link #onCreate()} function is called before. The actual
     * initialization of ION happens in that method
     *
     * @param intent  Intent that is supplied by the component to start the
     *                service
     * @param flags   Additional data about the start request
     * @param startId A unique integer representing the request
     * @return Indicates what semantics the system should use for the
     * service's state
     */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // Ensure that all requests (intents) to the service are handled
        return START_REDELIVER_INTENT;
    }

    /**
     * Is called once when the Service is created and is initializing the
     * Service. This method starts ion in a dedicated thread. ION is stopped
     * when {@link #onDestroy()} is called (by {@link #stopService(Intent)}.
     */
    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate: Starting NodeAdmService");

        na = new NativeAdapter(this);
        listenerSet = new HashSet<>();
        stopForeground(true);
    }

    /**
     * Is called when the destruction of the service is requested
     */
    @Override
    public void onDestroy() {
        Log.d(TAG, "Stopping ION");
    }

    /**
     * Creates a notification that can be used to start the service in the
     * foreground
     *
     * @return the notification object
     */
    private Notification createNotification(String message) {
        // Create a (default) notification channel (required for SDK >= 26)
        createNotificationChannel();

        // Build a notification that is displayed to the user while the
        // service is running
        Intent notificationIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent =
                PendingIntent.getActivity(this, 0, notificationIntent, 0);

        NotificationCompat.Builder nb = new NotificationCompat.Builder(this,
                NOTIF_CHANNEL);

        nb.setOngoing(true)
                .setContentTitle(getText(R.string.notification_title))
                .setContentText(message)
                .setSmallIcon(R.drawable.ic_ion_notification)
                .setContentIntent(pendingIntent)
                .setColor(ContextCompat.getColor(getApplicationContext(), R
                        .color.colorPrimary))
                .build();

        return nb.build();
    }

    /**
     * Creates a notification channel (if the SDK version is 26 or higher).
     * For SDK version below 26, no notification channel is required.
     */
    private void createNotificationChannel() {
        // Check if SDK version is less than 26 -> do not create notification
        // channel in that case
        if (Build.VERSION.SDK_INT < 26) {
            return;
        }

        NotificationManager mNotificationManager =
                (NotificationManager) getSystemService(Context
                        .NOTIFICATION_SERVICE);
        NotificationChannel mChannel =
                new NotificationChannel(NOTIF_CHANNEL,
                        getString(R.string.notification_channel_name),
                        NotificationManager.IMPORTANCE_HIGH);

        // Configure the notification channel.
        mChannel.setDescription("Contains all ION notifications");
        mChannel.enableLights(false);
        mNotificationManager.createNotificationChannel(mChannel);

    }

    /**
     * Handles state changes of the ION instance, manages notification
     * @param status The updated ION status information
     */
    @Override
    public void stateChanged(NativeAdapter.SystemStatus status) {
        switch (status) {
            case STARTED:
                // Start service in foreground again, actually does not change
                // anything except updating the notification message
                startForeground(NOTIFICATION_ID, createNotification(getText(R.string
                        .notification_message_started).toString()));
                break;

            case STOPPED:
                // Stop the foreground execution of the service
                stopForeground(true);
                break;
        }

        for (INodeAdminListener l : listenerSet) {
            try {
                l.notifyStatusChanged(status.name());
            }
            catch (RemoteException e) {
                Log.e(TAG, "stateChanged: Some listener was not " +
                        "available. Continue.");
            }
        }
    }

    private final INodeAdminService.Stub mBinder = new INodeAdminService.Stub() {
        @Override
        public int registerStatusChangeListener(INodeAdminListener lst) throws RemoteException {
            listenerSet.add(lst);
            //lst.notifyStatusChanged(na.getStatus().name());
            return 0;
        }

        @Override
        public int unregisterStatusChangeListener(INodeAdminListener lst) throws RemoteException {
            listenerSet.remove(lst);
            return 0;
        }

        @Override
        public void startION() throws RemoteException {
            String path;

            // Start the service in the foreground to prevent the Android system
            // from killing it in low memory situations
            Log.e(TAG, "startION: called start command");
            startForeground(NOTIFICATION_ID, createNotification(getText(R.string
                    .notification_message_starting).toString()));

            SharedPreferences sharedPref = getApplicationContext()
                    .getSharedPreferences(getString(R.string.preference_file_key)
                            , Context.MODE_PRIVATE);

            if (sharedPref.contains("initialized")) {
                path = sharedPref.getString("startup path", "");
            }
            else {
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putBoolean("initialized", true);
                editor.apply();
                path = sharedPref.getString("init path", "");
            }

            SharedPreferences globalPref = PreferenceManager
                    .getDefaultSharedPreferences(getApplicationContext());


            String propEID;

            // If selected, get current ip address from the Android OS
            if (globalPref.getBoolean("pref_ipnd_eid_autoupdate", false)) {
                propEID = "ipn:" + String.valueOf(sharedPref.getLong("node number", 1)) + ".0";
                Log.d(TAG, "startION: Assigned EID value was " + String
                        .valueOf(sharedPref.getLong("node number", 1)));
            }
            else {
                propEID = globalPref.getString
                        ("pref_ipnd_propagated_eid", "ipn:1.0");
            }

            Boolean status = globalPref.getBoolean
                    ("pref_ipnd_on_off", false);

            String listenAdr;

            // If selected, get current ip address from the Android OS
            if (globalPref.getBoolean("pref_ipnd_ip_listen_autoupdate", false)) {
                WifiManager wifiMgr = (WifiManager) getApplicationContext()
                        .getSystemService(WIFI_SERVICE);
                WifiInfo wifiInfo = wifiMgr.getConnectionInfo();
                int ip = wifiInfo.getIpAddress();
                listenAdr = Formatter.formatIpAddress(ip);
            }
            else {
                // Use the IP adr set by the user
                listenAdr = globalPref.getString("pref_ipnd_listen_adr",
                        "127.0.0.1");
            }

            String listeningPort = globalPref.getString
                    ("pref_ipnd_listen_port", "4550");

            String intervalUnicast = globalPref.getString
                    ("pref_ipnd_interval_unicast", "1");

            String intervalMulticast = globalPref.getString
                    ("pref_ipnd_interval_multicast", "1");

            String intervalBroadcast = globalPref.getString
                    ("pref_ipnd_interval_broadcast", "10");

            String ttlMulticast = globalPref.getString
                    ("pref_ipnd_ttl_multicast", "1");

            String destIP = globalPref.getString
                    ("pref_ipnd_destination_ip", "127.0.0.1");

            Boolean tcpclPropagation = globalPref.getBoolean
                    ("pref_ipnd_tcpcl_propagation", false);

            String tcpclPort = globalPref.getString
                    ("pref_ipnd_tcpcl_port", "4556");


            IpndFileUpdater.updateConfigFile(path,
                    status,
                    propEID,
                    listenAdr,
                    listeningPort,
                    intervalUnicast,
                    intervalMulticast,
                    intervalBroadcast,
                    ttlMulticast,
                    destIP,
                    tcpclPropagation,
                    tcpclPort);

            // Start ION (in a separate thread/runnable)
            na.initNode(path);
        }

        @Override
        public void stopION() throws RemoteException {
            // Stop ION (in a separate thread/runnable)
            na.stopNode();

            // Start service in foreground again, actually does not change
            // anything except updating the notification message
            startForeground(NOTIFICATION_ID, createNotification(getText(R.string
                    .notification_message_stopping).toString()));
        }

        @Override
        public String getContactListString() throws RemoteException {
            return na.getContactList();
        }

        @Override
        public String getRangeListString() throws RemoteException {
            return na.getRangeList();
        }

        @Override
        public String getSchemeListString() throws RemoteException {
            return na.getSchemeList();
        }

        @Override
        public String getEndpointListString() throws RemoteException {
            return na.getEndpointList();
        }

        @Override
        public String getProtocolListString() throws RemoteException {
            return na.getProtocolList();
        }

        @Override
        public String getInductListString() throws RemoteException {
            return na.getInductList();
        }

        @Override
        public String getOutductListString() throws RemoteException {
            return na.getOutductList();
        }

        @Override
        public String getBabRuleListString() throws RemoteException {
            return na.getBabRuleList();
        }

        @Override
        public String getBibRuleListString() throws RemoteException {
            return na.getBibRuleList();
        }

        @Override
        public String getBcbRuleListString() throws RemoteException {
            return na.getBcbRuleList();
        }

        @Override
        public String getKeyListString() throws RemoteException {
            return na.getKeyList();
        }
    };
}