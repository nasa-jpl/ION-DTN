package gov.nasa.jpl.camerashare;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.graphics.Point;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.os.RemoteException;
import android.support.v4.content.FileProvider;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Display;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Toast;

import org.apache.commons.io.FileUtils;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;

import gov.nasa.jpl.iondtn.IBundleReceiverListener;

public class GalleryActivity extends AppCompatActivity {
    private static final String TAG = "GalAct";
    File folder_gallery;
    GalleryRecyclerViewAdapter adapter;
    ArrayList<File> picList;
    RecyclerView.LayoutManager layoutManager;
    RecyclerView recyclerView;
    String local_eid;

    // BundleService object
    private gov.nasa.jpl.iondtn.IBundleService mService;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Set up the layout of the activity
        setContentView(R.layout.activity_gallery);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        }

        // Configure the recycler gallery view
        recyclerView = findViewById(R.id.gallery);
        recyclerView.setHasFixedSize(true);
        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        int columns = size.x/ 250;
        if (columns < 0) columns = 1;
        layoutManager = new GridLayoutManager(getApplicationContext(), columns);
        recyclerView.setLayoutManager(layoutManager);
        recyclerView.setHasFixedSize(true);
        recyclerView.setItemViewCacheSize(20);
        recyclerView.setDrawingCacheEnabled(true);
        recyclerView.setDrawingCacheQuality(View.DRAWING_CACHE_QUALITY_HIGH);

        // Get the previously defined sink eid and determine folder accordingly
        SharedPreferences sharedPref = getApplicationContext()
                .getSharedPreferences(getString(R.string.preference_file_key)
                        , Context.MODE_PRIVATE);
        local_eid = sharedPref.getString("own_sink_eid", "dtn:none");
        if (sharedPref.getBoolean("local", true)) {
            folder_gallery = new File(getExternalFilesDir(Environment
                    .DIRECTORY_PICTURES), sharedPref.getString
                    ("own_sink_eid", "ipn:1.1"));
        }
        else {
            folder_gallery = new File(getExternalFilesDir(Environment
                    .DIRECTORY_PICTURES), sharedPref.getString
                    ("remote_sink_eid", "ipn:1.1"));
        }

        // Create path if it doesn't exist yet
        if (!folder_gallery.exists()) {
            folder_gallery.mkdirs();
        }

        // Initialize a list for all received pictures (residing in the
        // specified folder)
        picList = new ArrayList<>(Arrays.asList
                (folder_gallery.listFiles()));

        // Set the adapter for the gallery view
        adapter = new GalleryRecyclerViewAdapter
                (picList, this);
        recyclerView.setAdapter(adapter);
    }

    // Create a file that can hold the image filled by the photo capture intent
    private File createImageFile() throws IOException {
        // Create an image file name
        String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new
                Date());
        String imageFileName = "JPEG_" + timeStamp + "_";
        File storageDir = folder_gallery;
        File image = File.createTempFile(
                imageFileName,  /* prefix */
                ".jpg",         /* suffix */
                storageDir      /* directory */
        );

        Log.d(TAG, "createImageFile: Created file with path \"" + image
                .getAbsolutePath() + "\"");

        return image;
    }

    // Establish a context menu that allows opening/sharing and deleting of
    // pictures
    public boolean onContextItemSelected(MenuItem item) {
        AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) item.getMenuInfo();
        int clickedItemPosition = item.getOrder();
        Log.d(TAG, "onContextItemSelected: message received " +
                clickedItemPosition);
        switch (item.getItemId()) {
            case 0:
                Log.d(TAG, "onContextItemSelected: message open");
                Intent intent = new Intent();
                intent.setAction(Intent.ACTION_VIEW);
                Uri photoURI = FileProvider.getUriForFile(this,
                        "com.example.android.fileprovider",
                        picList.get(clickedItemPosition));
                intent.setDataAndType(photoURI, "image/*").addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                startActivity(intent);
                return true;
            case 1:
                Log.d(TAG, "onContextItemSelected: message delete");
                if (folder_gallery.listFiles()[clickedItemPosition].exists()) {
                    folder_gallery.listFiles()[clickedItemPosition].delete();
                    picList.remove(clickedItemPosition);
                    adapter.notifyItemRemoved(clickedItemPosition);
                }
                return true;
            default:
                Log.d(TAG, "onContextItemSelected: message default");
                return super.onContextItemSelected(item);
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        if (mService == null) {
            Log.d(TAG, "onStart: (Re-)Binding service");

            // Create Intent
            Intent serviceIntent = new Intent()
                    .setComponent(new ComponentName(
                            "gov.nasa.jpl.iondtn",
                            "gov.nasa.jpl.iondtn.services.BundleService"));
            // Request to bind the service based on the intent
            bindService(serviceIntent, mConnection, BIND_AUTO_CREATE);
            recyclerView.setEnabled(false);
        }
    }

    // Service connection object
    private ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            Log.d(TAG, "onServiceConnected: Service bound!\n");

            // Save the service object
            mService = gov.nasa.jpl.iondtn.IBundleService.Stub.asInterface(service);

            SharedPreferences sharedPref = getApplicationContext()
                    .getSharedPreferences(getString(R.string.preference_file_key)
                            , Context.MODE_PRIVATE);

            Toast.makeText(getApplicationContext(), "Ion-DTN connected!",
                    Toast.LENGTH_SHORT).show();

            // Load sink eid from the shared preferences
            String srcEID = sharedPref.getString("own_sink_eid",
                    "dtn:none");

            // Enable gallery object
            recyclerView.setEnabled(true);

            // Open an endpoint to receive picture files
            try {
                if (!mService.openEndpoint(srcEID, listener)) {
                    throw new RemoteException("EID already bound!");
                }
            }
            catch (RemoteException e) {
                Toast.makeText(getApplicationContext(), "EID already in use. Select other EID!",
                        Toast.LENGTH_LONG).show();
                Log.e(TAG, "onServiceConnected: Failed to connect");
                e.printStackTrace();
                unbindService(mConnection);
                mService = null;
                finish();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            Log.d(TAG, "onServiceDisconnected: Service unbound!\n");
        }
    };

    @Override
    protected void onStop() {
        super.onStop();
        Log.d(TAG, "onStop: Unbinding service");

        if (mService != null) {
            try {
                mService.closeEndpoint();
            } catch (RemoteException e) {
                Log.e(TAG, "onServiceConnected: Failed to close Endpoint");
            }
            // Reset service element (GC will handle!)
            mService = null;

            // Unbind service
            unbindService(mConnection);
        }
    }

    // Implement the listener stub resolver for the BundleService
    IBundleReceiverListener.Stub listener = new IBundleReceiverListener.Stub() {

        @Override
        public int notifyBundleReceived(gov.nasa.jpl.iondtn.DtnBundle b) throws
                RemoteException {
            // Check which payload type the bundle encapsulates
            if (b.getPayloadType() == gov.nasa.jpl.iondtn.DtnBundle.payload_type
                    .BYTE_ARRAY) {
                try {
                    Toast.makeText(getApplicationContext(), "Received small " +
                            "bundle with content: " + new String(b
                            .getPayloadByteArray(), "UTF-8"), Toast.LENGTH_LONG)
                            .show();
                }
                catch (UnsupportedEncodingException e) {
                    Log.e(TAG, "notifyBundleReceived: UTF-8 encoding is not " +
                            "supported on this device");
                }
            }
            else {
                String line;

                FileInputStream in = new FileInputStream(b.getPayloadFD()
                        .getFileDescriptor());

                File photoFile = null;
                try {
                    photoFile = createImageFile();
                } catch (IOException ex) {
                    // Error occurred while creating the File
                    Log.e(TAG, "dispatchTakePictureIntent: Error occured");
                    ex.printStackTrace();
                }
                // Continue only if the File was successfully created
                if (photoFile != null) {
                    try {
                        FileUtils.copyInputStreamToFile(in, photoFile);
                    }
                    catch (IOException e) {
                        // Error occurred while creating the File
                        Log.e(TAG, "dispatchTakePictureIntent: Error occured " +
                                "while copying the file");
                        e.printStackTrace();
                    }
                    picList.add(photoFile);

                    // Don't add an object when update is in progress
                    if (!recyclerView.isComputingLayout()) {
                        adapter.notifyItemInserted(picList.indexOf(photoFile));
                    }
                }
            }

            return 0;
        }
    };
}
