package gov.nasa.jpl.iondtn.backend;

import android.app.Application;
import android.content.SharedPreferences;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;

import gov.nasa.jpl.iondtn.IBundleReceiverListener;
import gov.nasa.jpl.iondtn.IBundleService;
import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.DtnBundle;

/**
 * Runnable (i.e. Thread) that is executed in order to receive bundles from
 * ION and hand them to the subscribed listener.
 *
 * Each registered EID listener has it's own runnable. When the listener is
 * unregistered, the ReceiverRunnable terminates as well.
 *
 * @author Robert Wiewel
 */
public class ReceiverRunnable implements Runnable {
    private static final String TAG = "ReceiverRunnable";
    private IBundleReceiverListener mListener;
    private String eid;
    private IBundleService.Stub mBinder;
    private Application parent;
    private File f;
    private Long dataPtr;
    private boolean listener_died;

    /**
     * Constructor for the runnable
     * @param listener The client's listener implementation
     * @param eid The EID that the listener is listening for
     * @param dataPtr The data object pointer for the endpoint
     * @param mBinder The binder object of the BundleService
     * @param parent The parent application
     */
    public ReceiverRunnable(IBundleReceiverListener listener, String eid,
                            IBundleService.Stub mBinder,
                            long dataPtr,
                            Application parent) {
        this.mListener = listener;
        this.eid = eid;
        this.mBinder = mBinder;
        this.parent = parent;
        this.dataPtr = dataPtr;
        this.listener_died = false;
    }

    private class deathDetector implements IBinder.DeathRecipient {

        @Override
        public void binderDied() {
            listener_died = true;
        }
    }

    /**
     * Function that is executed in Runnable routine
     */
    @Override
    public void run() {

        Log.i(TAG, "New ReceiverThread started!");
        
        DtnBundle payload;

        try {
            f = File.createTempFile("run", "tmp", parent.getFilesDir());
        }
        catch (IOException e) {
            Log.e(TAG, "run: Failed to create new file \"" + this.hashCode() +
                    "\"! Cannot " +
                    "receive " +
                    "bundles!");
            e.printStackTrace();
            return;
        }

        SharedPreferences sharedPref = PreferenceManager
                .getDefaultSharedPreferences((parent.getApplicationContext())
                        .getApplicationContext());
        int threshold = Integer.parseInt(sharedPref.getString
                (parent.getString(R.string.threshold_data_file),
                        "1000"));

        try {
            mListener.asBinder().linkToDeath(new deathDetector(), 0);
        }
        catch (RemoteException e) {
            Log.e(TAG, "run: Failed to link termination binder");
            e.printStackTrace();
            return;
        }

        while (true) {
            payload = waitForBundle(this.dataPtr, threshold, f.getAbsolutePath());

            if (NativeAdapter.getStatus()
                    != NativeAdapter.SystemStatus.STARTED) {
                Log.i(TAG, "Forcefully shutting down ReceiverThread due to " +
                        "ION shutdown!");
                return;
            }

            if (Thread.interrupted()) {
                break;
            }

            if (listener_died) {
                try {
                    mBinder.closeEndpoint();
                }
                catch (RemoteException e) {
                    Log.e(TAG, "run: Failed to close Endpoint due to RemoteException");
                }
                break;
            }

            if (payload == null) {
                continue;
            }

            Log.d(TAG, "run: Got EID: " + payload.getEID());

            if (Thread.interrupted() || !notifyListener(payload)) {
                break;
            }
        }
        Log.i(TAG, "Gracefully shutting down ReceiverThread!");
    }

    private boolean notifyListener(DtnBundle received) {
        // Return immediately if the string is empty
        // An empty string is returned when the timeout occurs
        if (received == null) {
            return true;
        }

        try {
            if (received.getPayloadType() == DtnBundle.payload_type
                    .FILE_DESCRIPTOR) {
                Log.d(TAG, "notifyListener: Providing file descriptor!");
                received.setPayloadFD(ParcelFileDescriptor.open(f,
                        ParcelFileDescriptor.MODE_READ_ONLY));
                mListener.notifyBundleReceived(received);

                // Clear the (temp) output file from previous usages
                PrintWriter writer = new PrintWriter(f);
                writer.print("");
                writer.close();
            }
            else {
                mListener.notifyBundleReceived(received);
            }
        }
        catch (RemoteException e) {
            Log.e(TAG, "notifyListener: Listener no longer valid. " +
                    "Stopping!");
            return false;
        }
        catch (IOException e) {
            Log.e(TAG, "notifyListener: Failed to notify listener. " +
                    "IOException");
        }
        return true;
    }

    private native DtnBundle waitForBundle(long jniDataPtr, int
            payloadThreshold, String fdPath);
}
