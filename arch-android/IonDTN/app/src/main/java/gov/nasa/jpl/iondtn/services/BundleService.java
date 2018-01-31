package gov.nasa.jpl.iondtn.services;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import org.apache.commons.io.FileUtils;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import gov.nasa.jpl.iondtn.DtnBundle;
import gov.nasa.jpl.iondtn.IBundleReceiverListener;
import gov.nasa.jpl.iondtn.IBundleService;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;
import gov.nasa.jpl.iondtn.backend.ReceiverRunnable;

/**
 * The BundleService provides sending/receiving functionality of ION-DTN to
 * client applications
 *
 * @author Robert Wiewel
 */
public class BundleService extends Service {
    private final static String TAG = "BundleService";

    /**
     * Handles the startup process and configuration of the service
     * @param intent The intent that caused the startup (i.e. the service
     *               request)
     * @param flags The flags of the request
     * @param startId The startId assigned to the service
     * @return The startup behaviour of the service, in this case sticky
     *          (i.e. continuously running)
     */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "onStartCommand: Received start command.");
        return START_STICKY;
    }

    /**
     * Creates a new instance of this service
     */
    @Override
    public void onCreate() {

        if (NativeAdapter.getStatus() != NativeAdapter.SystemStatus.STARTED) {
            Log.i(TAG, "Creating request failed because ION is not running!");
            return;
        }
        // Attach the service to the running ION instance
        attachION();

        super.onCreate();
    }

    /**
     * Handles the destruction of a service object
     */
    @Override
    public void onDestroy() {
        super.onDestroy();

        try {
            // Detach from the ION instance
            detachION();
        }
        catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "onDestroy: ION was not bound and started, so detach " +
                    "is not possible");
        }
    }

    /**
     * Manages the Binding process
     * @param intent The intent requesting the binding process
     * @return the IBinder object required for binding
     */
    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "Received binding.");

        // Ensure that an ION instance is actually running, otherwise
        // starting this service makes no sense
        if (NativeAdapter.getStatus() != NativeAdapter.SystemStatus.STARTED) {
            Log.i(TAG, "Binding request failed because ION is not running!");
            return null;
        }

        return new BundleServiceBinder();
    }

    private static ArrayList<BundleServiceBinder> list = new ArrayList<>();

    private class BundleServiceBinder extends IBundleService.Stub {
        private long endPointSapPointer = 0;
        private Thread listener_thread;
        private String local_eid;
        private ArrayList<File> temp_files;
        private Semaphore sem;

        BundleServiceBinder() {
            super();
            local_eid = "";
            temp_files = new ArrayList<>();
            sem = new Semaphore(1);
        }

        @Override
        public boolean openEndpoint(String src_eid, IBundleReceiverListener
                listener) throws RemoteException {

            // Ensure that an ION instance is actually running, otherwise
            // starting this request makes no sense
            if (NativeAdapter.getStatus() != NativeAdapter.SystemStatus.STARTED) {
                Log.i(TAG, "Request failed because ION is not running!");
                return false;
            }

            // Check for possible clients that have subscribed to the EID before
            for (BundleServiceBinder b : list) {
                if (b.getRegisteredLocalEID().equals(src_eid)) {
                    Log.e(TAG, "openEndpoint: Endpoint already in use");
                    return false;
                }
            }

            // Abort if Endpoint is already open
            if (endPointSapPointer !=  0) {
                Log.e(TAG, "openEndpoint: Endpoint already in use");
                return false;
            }

            Log.d(TAG, "openEndpoint: Finished cleaning up!");

            // Connect to endpoint and save src eid
            local_eid = src_eid;
            endPointSapPointer = openEndpointION(src_eid);

            if (endPointSapPointer != 0) {
                Log.d(TAG, "openEndpoint: Opened endpoint for " + src_eid);
            }
            else {
                Log.e(TAG, "openEndpoint: Endpoint not open! Abort.");
                return false;
            }

            // Add oneself to the client list
            list.add(this);

            Log.d(TAG, "openEndpoint: Registering new listener");

            // Otherwise create a new one
            Thread t = new Thread(new ReceiverRunnable(listener, local_eid,
                    this, endPointSapPointer, getApplication()));
            t.start();
            listener_thread = t;
            return true;
        }

        @Override
        public boolean closeEndpoint() throws RemoteException {

            // Ensure that an ION instance is actually running, otherwise
            // starting this request makes no sense
            if (NativeAdapter.getStatus() != NativeAdapter.SystemStatus.STARTED) {
                Log.i(TAG, "Request failed because ION is not running!");
                return false;
            }

            if (endPointSapPointer == 0) {
                Log.e(TAG, "closeEndpoint: Endpoint not open!");
                return false;
            }

            try {
                if (!sem.tryAcquire(5, TimeUnit.SECONDS)) {
                    Log.e(TAG, "Could not get semaphore! Sender has likely " +
                            "crashed! Still shutting down endpoint!");
                }
            }
            catch (InterruptedException e) {
                Log.e(TAG, "sendBundle: Interrupted while taking " +
                        "semaphore! Abort!");
                return false;
            }

            if (listener_thread != null) {
                listener_thread.interrupt();
                listener_thread = null;
            }

            // Delete all created temporary files for the endpoint
            for (File f : temp_files) {
                if (!f.delete()) {
                    Log.e(TAG, "closeEndpoint: Failed to delete temporary " +
                            "file " + f.getName());
                }
                temp_files.remove(f);
            }

            Log.d(TAG, "closeEndpoint: Close endpoint for " + this.local_eid);
            // Remove oneself from the client list
            list.remove(this);
            boolean retVal = closeEndpointION(endPointSapPointer);

            endPointSapPointer = 0;
            local_eid = "";
            sem.release();

            return  retVal;
        }

        @Override
        public boolean sendBundle(DtnBundle b) throws RemoteException {
            File tmp_file;
            int retVal;

            // Ensure that an ION instance is actually running, otherwise
            // starting this request makes no sense
            if (NativeAdapter.getStatus() != NativeAdapter.SystemStatus.STARTED) {
                Log.i(TAG, "Request failed because ION is not running!");
                return false;
            }

            // Check if bundle data is byte array or file descriptor and
            // send accordingly
            if (b.getPayloadType() == DtnBundle.payload_type.BYTE_ARRAY) {
                Log.d(TAG, "Sending byte_array payload to destination EID " +
                        b.getEID());

                try {
                    if (!sem.tryAcquire(2, TimeUnit.SECONDS)) {
                        Log.e(TAG, "Could not get semaphore! Abort!");
                        return false;
                    }
                }
                catch (InterruptedException e) {
                    Log.e(TAG, "sendBundle: Interrupted while taking " +
                            "semaphore! Abort!");
                    return false;
                }

                if (endPointSapPointer == 0) {
                    Log.e(TAG, "sendData: Endpoint not open! Sending from " +
                            "dtn:none");
                }

                retVal = sendBundleION(b.getEID(),
                                      b.getPriority().ordinal(),
                                      b.getTimeToLive(),
                                      b.getPayloadByteArray(),
                                      endPointSapPointer);
                sem.release();

                return (retVal >= 0);
            } else {

                try {
                    FileInputStream in = new FileInputStream(b.getPayloadFD()
                            .getFileDescriptor());
                    tmp_file = File.createTempFile("abc", ".tmp", getFilesDir());
                    FileUtils.copyInputStreamToFile(in, tmp_file);

                    in.close();
                }
                catch (IOException e) {
                    Log.e(TAG, "sendFile: IO Exception");
                    // Return error state as file could not be successfully
                    // put into the data structure
                    return false;
                }

                try {
                    if (!sem.tryAcquire(2, TimeUnit.SECONDS)) {
                        Log.e(TAG, "Could not get semaphore! Abort!");
                        return false;
                    }
                }
                catch (InterruptedException e) {
                    Log.e(TAG, "sendBundle: Interrupted while taking " +
                            "semaphore! Abort!");
                    return false;
                }

                if (endPointSapPointer == 0) {
                    Log.e(TAG, "sendData: Endpoint not open! Sending from " +
                            "dtn:none");
                }

                retVal = sendBundleFileION(b.getEID(),
                                           b.getPriority().ordinal(),
                                           b.getTimeToLive(),
                                           tmp_file.getAbsolutePath(),
                                           endPointSapPointer);
                sem.release();

                temp_files.add(tmp_file);

                return (retVal >= 0);
            }
        }

        private String getRegisteredLocalEID() {
            return this.local_eid;
        }
    }

    private native int sendBundleION(String dest_eid, int qos, int ttl, byte[]
            payload, long id);
    private native int sendBundleFileION(String dest_eid,
                                         int qos, int ttl,
                                         String file_path, long id);
    private native int attachION();
    private native void detachION();
    private native long openEndpointION(String src_eid);
    private native boolean closeEndpointION(long id);
}
