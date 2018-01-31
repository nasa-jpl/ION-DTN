package gov.nasa.jpl.minimaltextapplication;

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
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;

import gov.nasa.jpl.iondtn.DtnBundle;
import gov.nasa.jpl.iondtn.IBundleReceiverListener;

public class MainActivity extends AppCompatActivity {
    public static final String TAG = "MainActivity";
    private gov.nasa.jpl.iondtn.IBundleService mService;
    boolean subscribed = false;

    String received = "Received:\n\n";

    Button buttonSend;
    Button buttonSubUnsub;
    TextView textViewReceive;
    EditText editTextPayload;
    EditText editTextDestEid;
    EditText editTextSinkId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Bind GUI elements to the xml resources
        buttonSend = findViewById(R.id.buttonSend);
        buttonSubUnsub = findViewById(R.id.buttonSubUnsub);
        textViewReceive = findViewById(R.id.textViewReceive);
        editTextPayload = findViewById(R.id.editTextPayload);
        editTextDestEid = findViewById(R.id.editTextDestEid);
        editTextSinkId = findViewById(R.id.editTextSinkId);

        // Disable all GUI elements
        buttonSend.setEnabled(false);
        buttonSubUnsub.setEnabled(false);
        editTextPayload.setEnabled(false);
        editTextSinkId.setEnabled(false);
        editTextDestEid.setEnabled(false);

        // Set the start value for the receiving output
        textViewReceive.setText(received);

        // Set the OnClickListener for the send button
        buttonSend.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // Make sure that a recipient is set in the destination text
                // field
                if (editTextDestEid.getText().toString().isEmpty()) {
                    Toast.makeText(getApplicationContext(), "Destination EID " +
                            "cannot be empty!", Toast
                            .LENGTH_SHORT).show();
                    return;
                }

                // Make sure that the user entered a payload text
                if (editTextPayload.getText().toString().isEmpty()) {
                    Toast.makeText(getApplicationContext(), "Payload cannot " +
                            "be empty!", Toast
                            .LENGTH_SHORT).show();
                    return;
                }

                try {
                    // Check if the service is available, abort action if not
                    if (mService == null) {
                        Toast.makeText(getApplicationContext(), "Service not " +
                                "available! Reconnecting! Try again!", Toast
                                .LENGTH_LONG).show();
                        bindBundleService();
                        return;
                    }

                    // Create new bundle to transmit
                    DtnBundle b = new DtnBundle(
                            editTextDestEid.getText().toString(),
                            0,
                            300,
                            DtnBundle.Priority.BULK,
                            editTextPayload.getText().toString()
                                    .getBytes("UTF-8"));

                    // Use the bundleService interface to send the data to
                    // the IonDTN provider application
                    if (!mService.sendBundle(b)) {
                        Toast.makeText(getApplicationContext(), "Service not " +
                                "available!", Toast.LENGTH_LONG).show();
                    }

                    // Reset the payload text field (intuitive user
                    // experience, sent -> gone)
                    editTextPayload.setText("");

                }
                // Catch exceptions if UTF-8 is not available or the service
                // disconnected unexpectedly
                catch (UnsupportedEncodingException e) {
                    Log.e(TAG, "onClick: UTF-8 encoding seems not to be " +
                            "available on this platform");
                    Toast.makeText(getApplicationContext(), "Failed to send bundle!", Toast
                            .LENGTH_SHORT).show();
                }
                catch (RemoteException e) {
                    Toast.makeText(getApplicationContext(), "Failed to send bundle!", Toast
                            .LENGTH_SHORT).show();
                }
            }
        });

        // Set the OnClickListener for the subscribing/unsubscribing button
        buttonSubUnsub.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (!subscribed) {
                    // Make sure that a sink id is set in the sink id text
                    // field
                    if (editTextSinkId.getText().toString().isEmpty()) {
                        Toast.makeText(getApplicationContext(), "Sink ID " +
                                        "cannot be empty!",
                                Toast.LENGTH_SHORT).show();
                        return;
                    }

                    // Check if the service is available, abort action if not
                    if (mService == null) {
                        Toast.makeText(getApplicationContext(), "Service not " +
                                "available! Reconnecting! Try again!", Toast
                                .LENGTH_LONG).show();
                        bindBundleService();
                        return;
                    }

                    try {
                        if (!mService.openEndpoint(editTextSinkId.getText
                                ().toString(), listener)) {
                            Toast.makeText(getApplicationContext(), "Failed to " +
                                    "open endpoint or EID already in use! " +
                                            "Enter" +
                                            " valid local EID!",
                                    Toast
                                    .LENGTH_SHORT).show();
                            return;
                        }
                    // Handle exception
                    } catch (RemoteException e) {
                        buttonSend.setEnabled(false);
                        buttonSubUnsub.setEnabled(false);
                        editTextPayload.setEnabled(false);
                        editTextSinkId.setEnabled(false);
                        editTextDestEid.setEnabled(false);
                        e.printStackTrace();
                        Toast.makeText(getApplicationContext(), "Failed to " +
                                "use Service! Reconnecting!", Toast
                                .LENGTH_SHORT).show();
                        bindBundleService();
                        return;
                    }
                    // Enable/Disable fields that represent the new state
                    // (subscribed/unsubscribed)
                    subscribed = true;
                    buttonSubUnsub.setText(R.string.button_label_close);
                }
                else {
                    try {
                        // Try to unregister eid listener
                        mService.closeEndpoint();
                    }
                    // Catch the exception when unsubscribing fails
                    catch (RemoteException|NullPointerException e) {
                        Toast.makeText(getApplicationContext(), "Failed to " +
                                "close endpoint!", Toast
                                .LENGTH_SHORT).show();
                        bindBundleService();
                        return;
                    }
                    subscribed = false;
                    buttonSubUnsub.setText(R.string.button_label_open);
                }
            }
        });
    }

    @Override
    protected void onStart() {
        super.onStart();
        bindBundleService();
    }

    void bindBundleService() {
        if (mService == null) {
            Log.d(TAG, "onStart: (Re-)Binding service");
            // Bind to service
            Intent serviceIntent = new Intent()
                    .setComponent(new ComponentName(
                            "gov.nasa.jpl.iondtn",
                            "gov.nasa.jpl.iondtn.services.BundleService"));
            bindService(serviceIntent, mConnection, BIND_AUTO_CREATE);
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy: Unbinding service");
        if (mService != null) {
            // Reset gui elements to represent current system state
            buttonSend.setEnabled(false);
            buttonSubUnsub.setEnabled(false);
            editTextPayload.setEnabled(false);
            editTextSinkId.setEnabled(false);
            editTextDestEid.setEnabled(false);

            if (subscribed) {
                try {
                    mService.closeEndpoint();
                } catch (RemoteException e) {
                    Log.e(TAG, "onStop: Failed to close endpoint" +
                            " while stopping app");
                }
            }

            subscribed = false;
            buttonSubUnsub.setText(getText(R.string.button_label_open));

            // Unbind from the service
            unbindService(mConnection);
        }

        super.onDestroy();
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
                    // Receive the data from the bundle and extract the
                    // ByteArray directly
                    received += "Source: " + b.getEID() + " Payload: ";
                    received += new String(b.getPayloadByteArray(), "UTF-8");
                    received += "\n";

                    // Update the GUI
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            textViewReceive.setText(received);
                        }
                    });
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
                BufferedReader br = new BufferedReader(new InputStreamReader(in));
                try {
                    // Receive the data from the bundle and extract the
                    // FileDescriptor of the actual data file
                    while ((line = br.readLine()) != null) {
                        received += "Source: " + b.getEID() + " Payload: ";
                        received += line;
                        received += "\n";
                    }
                }
                catch (IOException e) {
                    Log.e(TAG, "notifyBundleReceived: Failed to parse file referenced " +
                            "by file descriptor");
                }
                // Update the GUI
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        textViewReceive.setText(received);
                    }
                });
            }

            return 0;
        }
    };

    private ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            Log.d(TAG, "onServiceConnected: Service bound!\n");
            mService = gov.nasa.jpl.iondtn.IBundleService.Stub.asInterface(service);

            // Activate sink eid textEdit and button for selecting own sink
            buttonSend.setEnabled(true);
            buttonSubUnsub.setEnabled(true);
            editTextPayload.setEnabled(true);
            editTextSinkId.setEnabled(true);
            editTextDestEid.setEnabled(true);
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            mService = null;
            // Log that the service disconnected
            Log.d(TAG, "onServiceDisconnected: Service disconnected.\n");
            subscribed = false;
            buttonSubUnsub.setText(getText(R.string.button_label_open));
        }
    };
}
