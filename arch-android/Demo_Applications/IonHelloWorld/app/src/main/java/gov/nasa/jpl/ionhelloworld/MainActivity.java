package gov.nasa.jpl.ionhelloworld;

import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import java.io.UnsupportedEncodingException;

import gov.nasa.jpl.iondtn.DtnBundle;

public class MainActivity extends AppCompatActivity {
    Button button;
    EditText editDestEID;


    // Tag for debug logging purposes
    public static final String TAG = "MainActivity";

    // BundleService object
    private gov.nasa.jpl.iondtn.IBundleService mService;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Initialize parent class
        super.onCreate(savedInstanceState);

        // Inflate layout of activity
        setContentView(R.layout.activity_main);

        // Bind layout elements to Java objects
        button = (Button)findViewById(R.id.button);
        editDestEID = (EditText) findViewById(R.id.editText);

        // Disable elements (until service is available)
        button.setEnabled(false);
        editDestEID.setEnabled(false);

        // Define 'click' behavior for button
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String payload = "Hello World";

                // (1) Check if editText for the destination EID is empty, abort
                // in that case
                if (editDestEID.getText().toString().isEmpty()) {
                    Toast.makeText(getApplicationContext(), "Destination " +
                            "EID " +
                            "cannot be empty!", Toast
                            .LENGTH_SHORT).show();
                    return;
                }

                // (2) Ensure that the service is actually available
                if (mService == null) {
                    Toast.makeText(getApplicationContext(), "Service not " +
                            "available!", Toast.LENGTH_LONG).show();
                    return;
                }

                try {
                    // (3) Create a Bundle object that holds all required
                    // metadata and the payload
                    DtnBundle b = new DtnBundle(editDestEID.getText()
                            .toString(),
                            0,
                            300,
                            DtnBundle.Priority.EXPEDITED,
                            payload.getBytes("UTF-8"));

                    // (4) Trigger sending of bundle by handing the bundle
                    // over to the BundleService
                    mService.sendBundle(b);
                }
                // (5) Catch error linked to BundleService (i.e. connection
                // broke)
                catch (RemoteException e) {
                        Toast.makeText(getApplicationContext(), "Failed to " +
                                "open endpoint!", Toast
                                .LENGTH_SHORT).show();
                }
                // (6) Catch error, when the payload cannot be encoded into
                // UTF-8
                catch (UnsupportedEncodingException e) {
                    Log.e(TAG, "onClick: UTF-8 encoding seems not to be " +
                            "available on this platform");
                    Toast.makeText(getApplicationContext(), "Failed to send bundle!", Toast
                            .LENGTH_SHORT).show();
                }
            }
        });
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
        }
    }

    // Service connection object
    private ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            Log.d(TAG, "onServiceConnected: Service bound!\n");

            // Save the service object
            mService = gov.nasa.jpl.iondtn.IBundleService.Stub.asInterface(service);

            // Update GUI
            button.setEnabled(true);
            editDestEID.setEnabled(true);
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

        // Only unbind if bound in the first place
        if (mService != null) {
            // Disable GUI
            button.setEnabled(false);
            editDestEID.setEnabled(false);

            // Unbind service
            unbindService(mConnection);

            // Reset service element (GC will handle!)
            mService = null;
        }
    }

}
