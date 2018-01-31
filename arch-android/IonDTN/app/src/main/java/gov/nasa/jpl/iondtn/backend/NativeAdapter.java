package gov.nasa.jpl.iondtn.backend;

import android.os.AsyncTask;
import android.util.Log;

/**
 * The NativeAdapter class provides an abstraction of the JNI calls. Many
 * function calls that are provided by this class are wrapper functions for
 * JNI function calls. Besides these wrappers, the class also manages the
 * status information of the ION instance (i.e. if it is running or not)
 *
 * @author Robert Wiewel
 */

public class NativeAdapter {
    private static final String TAG = "NativeAdapter";

    /**
     * Represents the system status of ION, i.e. if it is running or not
     *
     * @author Robert Wiewel
     */
    public enum SystemStatus {
        STARTED,
        STARTING,
        STOPPED,
        STOPPING
    }

    /**
     * Interface that allows different parts of the application to subscribe
     * to ION status changes
     */
    public interface OnIonStateChangeListener {
        /**
         * Is called when the ION status changes
         * @param status The updated ION status information
         */
        void stateChanged(SystemStatus status);
    }

    /**
     * Private listener for status changes
     */
    private OnIonStateChangeListener mListener;

    /**
     * This variable holds the current status of the assigned ION instance
     */
    private static SystemStatus ionStatus = SystemStatus.STOPPED;

    /**
     * Default constructor that initializes the ION status
     */
    public NativeAdapter(OnIonStateChangeListener listener) {
        mListener = listener;
        ionStatus = SystemStatus.STOPPED;
    }

    /**
     * Private function that allows wrapper functions to change the status of
     * the ION instance
     * @param status The status that the internal status should be set to
     */
    private void setStatus(SystemStatus status) {
        Log.d(TAG, "setStatus: Set status to " + status);
        ionStatus = status;
        mListener.stateChanged(status);
    }

    /**
     * Provides the current status of the ION instance
     * @return The status as an enum of type {@link SystemStatus}.
     */
    public static SystemStatus getStatus() {
        return ionStatus;
    }

    /**
     * Initialize the ION instance with the configuration files provided
     * @param path The path where the configuration files that should be used
     *             can be found
     * @return The return value of the JNI function, i.e. a posix return code
     * (0 on success, negative value in case of an error)
     */
    public int initNode(String path) {
        Log.d(TAG, "initNode: Initializing ION");
        setStatus(SystemStatus.STARTING);
        new initIonTask().execute(path);
        return 0;
    }

    private class initIonTask extends AsyncTask<String, Void, Integer> {
        initIonTask() {}
        boolean success;

        @Override
        protected Integer doInBackground(String... strings) {
            success = initION(strings[0]);
            return 0;
        }

        @Override
        protected void onPostExecute(Integer integer) {
            if (success) {
                setStatus(SystemStatus.STARTED);
            }
            else {
                setStatus(SystemStatus.STOPPED);
            }
            super.onPostExecute(integer);
        }
    }

    /**
     * Start the (previously initialized or started) ION instance
     * @return The return value of the JNI function, i.e. a posix return code
     * (0 on success, negative value in case of an error)
     */
    public int stopNode() {
        Log.d(TAG, "stopNode: Stopping ION");
        setStatus(SystemStatus.STOPPING);
        new stopIonTask().execute();
        return 0;
    }

    private class stopIonTask extends AsyncTask<String, Void, Integer> {
        stopIonTask() {}

        @Override
        protected Integer doInBackground(String... strings) {
            stopION();
            return 0;
        }

        @Override
        protected void onPostExecute(Integer integer) {
            setStatus(SystemStatus.STOPPED);
            super.onPostExecute(integer);
        }
    }

    /**
     * Get a list of all contacts in the ION database
     * @return The contact list as formatted string
     */
    public String getContactList() {
        return getContactListION();
    }

    /**
     * Get a list of all ranges in the ION database
     * @return The range list as formatted string
     */
    public String getRangeList() {
        return getRangeListION();
    }

    /**
     * Get a list of all schemes in the ION database
     * @return The scheme list as formatted string
     */
    public String getSchemeList() {
        return getSchemeListION();
    }

    /**
     * Get a list of all endpoints in the ION database
     * @return The endpoint list as formatted string
     */
    public String getEndpointList() {
        return getEndpointListION();
    }

    /**
     * Get a list of all protocols in the ION database
     * @return The protocol list as formatted string
     */
    public String getProtocolList() {
        return getProtocolListION();
    }

    /**
     * Get a list of all inducts in the ION database
     * @return The induct list as formatted string
     */
    public String getInductList() {
        return getInductListION();
    }

    /**
     * Get a list of all outducts in the ION database
     * @return The outduct list as formatted string
     */
    public String getOutductList() {
        return getOutductListION();
    }

    /**
     * Get a list of all bab rules in the ION database
     * @return The bab rule list as formatted string
     */
    public String getBabRuleList() { return getBabRuleListION(); }

    /**
     * Get a list of all bib rules in the ION database
     * @return The bib rule list as formatted string
     */
    public String getBibRuleList() { return getBibRuleListION(); }

    /**
     * Get a list of all bcb rules in the ION database
     * @return The bcb rule list as formatted string
     */
    public String getBcbRuleList() { return getBcbRuleListION(); }

    /**
     * Get a list of all keys in the ION database
     * @return The key list as formatted string
     */
    public String getKeyList() { return getKeyListION(); }


    private native boolean initION(String absoluteConfigPath);
    private native String stopION();
    private native String getContactListION();
    private native String getRangeListION();
    private native String getSchemeListION();
    private native String getEndpointListION();
    private native String getProtocolListION();
    private native String getInductListION();
    private native String getOutductListION();
    private native String getBabRuleListION();
    private native String getBibRuleListION();
    private native String getBcbRuleListION();
    private native String getKeyListION();


}
