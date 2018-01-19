package gov.nasa.jpl.camerashare;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.graphics.Point;
import android.net.Uri;
import android.os.Environment;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.provider.MediaStore;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.content.FileProvider;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Display;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Toast;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;

import gov.nasa.jpl.iondtn.DtnBundle;

public class CameraActivity extends AppCompatActivity {
    private static final String TAG = "CamAct";
    static final int REQUEST_IMAGE_CAPTURE = 1;
    File folder_gallery;
    GalleryRecyclerViewAdapter adapter;
    ArrayList<File> picList;
    File newPhoto;
    RecyclerView.LayoutManager layoutManager;
    RecyclerView recyclerView;
    FloatingActionButton fab;

    // BundleService object
    private gov.nasa.jpl.iondtn.IBundleService mService;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Setup the layout of the activity
        setContentView(R.layout.activity_camera);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        }

        // Set the onClick listener of the floating action button (that
        // launches the picture taking process)
        fab = (FloatingActionButton) findViewById(R.id
                .fabTakePicture);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                dispatchTakePictureIntent();
            }
        });

        // Setup the RecyclerView gallery object
        recyclerView = (RecyclerView)findViewById(R.id.cameraHistory);
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

        // Load the shared preferences and determine the remote sink eid
        SharedPreferences sharedPref = getApplicationContext()
                .getSharedPreferences(getString(R.string.preference_file_key)
                        , Context.MODE_PRIVATE);
        folder_gallery = new File(getExternalFilesDir(Environment
                .DIRECTORY_PICTURES), sharedPref.getString
                ("remote_sink_eid", "ipn:1.1"));

        // Create the path if it doesn't exist yet
        if (!folder_gallery.exists()) {
            folder_gallery.mkdirs();
        }
        Log.d(TAG, "onCreate: Path is " + folder_gallery.getAbsolutePath());

        // Initialize the list holding the information about the pictures
        picList = new ArrayList<>(Arrays.asList
                (folder_gallery.listFiles()));

        // Assign the adapter to the GalleryView
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
                    // Update the gallery view
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

    // Launch a picture capturing intent
    private void dispatchTakePictureIntent() {
        Intent takePictureIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
        // Ensure that there's a camera activity to handle the intent
        if (takePictureIntent.resolveActivity(getPackageManager()) != null) {
            // Create the File where the photo should go
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
                newPhoto = photoFile;
                Uri photoURI = FileProvider.getUriForFile(this,
                        "com.example.android.fileprovider",
                        photoFile);
                takePictureIntent.putExtra(MediaStore.EXTRA_OUTPUT, photoURI);
                startActivityForResult(takePictureIntent, REQUEST_IMAGE_CAPTURE);
            }
        }
    }

    // Handle the return values from the picture taking intent
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {

        if(resultCode == RESULT_OK && newPhoto != null){
            // Add photo to list
            picList.add(newPhoto);
            // Notify the gallery view about the new object
            adapter.notifyItemInserted(picList.indexOf(newPhoto));

            // Determine the destination eid (which has been set in
            // MainActivity)
            SharedPreferences sharedPref = getApplicationContext()
                    .getSharedPreferences(getString(R.string.preference_file_key)
                            , Context.MODE_PRIVATE);
            String destEID = sharedPref.getString("remote_sink_eid",
                    "dtn:none");

            // Sent the picture via ION-DTN
            try {
                // Create a parcel file descriptor of the image file
                ParcelFileDescriptor pfd = ParcelFileDescriptor.open(newPhoto,
                        ParcelFileDescriptor.MODE_READ_ONLY);

                // Create a bundle with the necessary metadata
                DtnBundle b = new DtnBundle(destEID, 0, 300, DtnBundle
                        .Priority.STANDARD, pfd);

                // Send the bundle
                mService.sendBundle(b);
                Log.d(TAG, "onActivityResult: Sent file via bundle!");
            }
            // Catch possibly occuring errors
            catch (FileNotFoundException|RemoteException e) {
                Log.e(TAG, "onActivityResult: Transmission of picture failed");
                Toast.makeText(getApplicationContext(), "Failed to send " +
                        "picture!", Toast.LENGTH_LONG).show();
            }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();

        // Bind to the BundleService whenever the activity is started
        if (mService == null) {
            Log.d(TAG, "onStart: (Re-)Binding service");
            // Create Intent
            Intent serviceIntent = new Intent()
                    .setComponent(new ComponentName(
                            "gov.nasa.jpl.iondtn",
                            "gov.nasa.jpl.iondtn.services.BundleService"));
            // Request to bind the service based on the intent
            bindService(serviceIntent, mConnection, BIND_AUTO_CREATE);
            // Disable the CameraCapture button until a connection has been
            // established
            fab.setEnabled(false);
        }
    }

    // Service connection object
    private ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            Log.d(TAG, "onServiceConnected: Service bound!\n");

            // Save the service object
            mService = gov.nasa.jpl.iondtn.IBundleService.Stub.asInterface(service);

            Toast.makeText(getApplicationContext(), "Ion-DTN connected!",
                    Toast.LENGTH_SHORT).show();

            fab.setEnabled(true);
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            Log.d(TAG, "onServiceDisconnected: Service unbound!\n");
        }
    };

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        // Delete all pictures in the temporary folder
        for (File file : folder_gallery.listFiles())
            file.delete();

        // Delete the folder itself
        if (!folder_gallery.delete()) {
            Log.e(TAG, "onDestroy: Couldn't delete temporary folder");
        }
        Log.d(TAG, "onStop: Unbinding service");

        // Unbind service
        unbindService(mConnection);

        // Reset service element (GC will handle!)
        mService = null;

        super.onDestroy();
    }
}
