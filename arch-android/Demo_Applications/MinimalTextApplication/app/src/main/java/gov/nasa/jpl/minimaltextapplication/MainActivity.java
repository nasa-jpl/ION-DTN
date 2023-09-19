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
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
// Jay L. Gao
import java.nio.file.FileAlreadyExistsException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

//Jay L. Gao
import android.os.Handler;

import gov.nasa.jpl.iondtn.DtnBundle;
import gov.nasa.jpl.iondtn.IBundleReceiverListener;

public class MainActivity extends AppCompatActivity {
    public static final String TAG = "MainActivity";
    private gov.nasa.jpl.iondtn.IBundleService mService;
    boolean subscribed = false;

    //Jay L. Gao: add handler in the main UI thread (mainactivity) to repeatedly send bundle
    private Handler mHandler = new Handler();

    String received = "Received:\n\n";

    Button buttonSend;
    Button buttonSubUnsub;
    TextView textViewReceive;
    // Jay L. Gao: add the scroll view from main activity xml
    ScrollView receivedScrollView;
    EditText editTextPayload;
    EditText editTextDestEid;
    EditText editTextSinkId;
    // Jay L. Gao: Read in number of payloads to send
    EditText numPayloadToSend;
    // Jay L. Gao: regex to detect the command strlen and the number of character per payload
    String regex_keyword = "^strlen=";
    String regex_num = "[0-9]+";
    Pattern keywordPattern = Pattern.compile(regex_keyword);
    Pattern numberPattern = Pattern.compile(regex_num);
    // Jay L. Gao: counter of received Payload
    int receivedPayloadCount = 0;
    // Jay L. Gao: counter of Payload to sent and already sent during current cycle
    int countPayloadToSend = 0;
    int countPayloadAlreadySent = 0;
    int sendIntervalMSec = 100;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Bind GUI elements to the xml resources
        buttonSend = findViewById(R.id.buttonSend);
        buttonSubUnsub = findViewById(R.id.buttonSubUnsub);
        // Jay L. Gao: add scroll view
        receivedScrollView = findViewById(R.id.receivedScrollView);
        textViewReceive = findViewById(R.id.textViewReceive);
        editTextPayload = findViewById(R.id.editTextPayload);
        editTextDestEid = findViewById(R.id.editTextDestEid);
        editTextSinkId = findViewById(R.id.editTextSinkId);
        // Jay L. Gao
        numPayloadToSend = findViewById(R.id.editTextNumberSigned7);

        // Disable all GUI elements
        buttonSend.setEnabled(false);
        buttonSubUnsub.setEnabled(false);
        editTextPayload.setEnabled(false);
        editTextSinkId.setEnabled(false);
        editTextDestEid.setEnabled(false);
        // Jay L. Gao
        numPayloadToSend.setEnabled(false);

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

                // Reset the payload text field (intuitive user
                // experience, sent -> gone)
                // Jay L. Gao: don't clear text to reduce typing
                // editTextPayload.setText("");

                // start repeating task here
                // disable the button?
                startRepeatingSend();

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
                        // Jay L. Gao
                        numPayloadToSend.setEnabled(false);

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

    // Jay L. Gao: this will start repeating a number of bundle transmission
    private void startRepeatingSend() {
        // set the number of payload to sent for the current cycle
        countPayloadToSend = Integer.parseInt(numPayloadToSend.getText().toString());
        countPayloadAlreadySent = 0;
        // disable button so payload cannot be sent until cycle is over
        buttonSend.setEnabled(false);
        // start repeating task
        mHandler.postDelayed(mSendPayloadRunnable, sendIntervalMSec);
    }

    private void stopRepeatingSend(){
        mHandler.removeCallbacks(mSendPayloadRunnable);
        buttonSend.setEnabled(true);
    }

    // Jay L. Gao:


    // Jay L. Gao: create runnable to repeat bundle transmission
    private Runnable mSendPayloadRunnable = new Runnable() {
        @Override
        public void run() {
            try {
                // Check if the service is available, abort action if not
                if (mService == null) {
                    Toast.makeText(getApplicationContext(), "Service not " +
                            "available! Reconnecting! Try again!", Toast
                            .LENGTH_LONG).show();
                    bindBundleService();
                    return;
                }
                // Jay L. Gao: determine if there is payload command
                String payloadStr = editTextPayload.getText().toString();
                Matcher k = keywordPattern.matcher(payloadStr);
                if (k.find()) {
                    Log.d("PayloadCommandString:", "Yes");
                    Matcher n = numberPattern.matcher(payloadStr);
                    if (n.find()) {
                        int payloadLength = Integer.parseInt(n.group(0));
                        StringBuilder buf = new StringBuilder();
                        Log.d("PayloadCommandLength:", String.valueOf(payloadLength));
                        // construct a string of length
                        for (int i = 1; i <= payloadLength; i++) {
                            buf.append("a");
                        }
                        payloadStr = buf.toString();
                    }
                }

                // Jay L. Gao: start a for loop
                //Create new bundle to transmit
                DtnBundle b = new DtnBundle(
                        editTextDestEid.getText().toString(),
                        0,
                        300,
                        DtnBundle.Priority.BULK,
                        payloadStr.getBytes("UTF-8"));

                // Use the bundleService interface to send the data to
                // the IonDTN provider application
                if (!mService.sendBundle(b)) {
                    Toast.makeText(getApplicationContext(), "Service not " +
                            "available!", Toast.LENGTH_LONG).show();
                }

                // increment counter and stop if
                countPayloadAlreadySent = countPayloadAlreadySent + 1;
                Log.d("Answer: ", String.valueOf(countPayloadAlreadySent));
                // should we stop?
                if(countPayloadAlreadySent < countPayloadToSend) {
                    mHandler.postDelayed(this, sendIntervalMSec);
                }
                else {
                    stopRepeatingSend();
                }
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
    };

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
            // Jay L. Gao
            numPayloadToSend.setEnabled(false);

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
            // Jay L. Gao: clear received string
            received = "";
            // Check which payload type the bundle encapsulates
            if (b.getPayloadType() == gov.nasa.jpl.iondtn.DtnBundle.payload_type
                    .BYTE_ARRAY) {
                try {
                    // Receive the data from the bundle and extract the
                    // ByteArray directly
                    // Jay L. Gao: add count, short display, add length
                    receivedPayloadCount++;
                    String payloadText = new String(b.getPayloadByteArray(), "UTF-8");
                    int payloadSize = payloadText.length();
                    received += String.format("Source: %s, Count: %d, ", b.getEID(), receivedPayloadCount);
                    received += String.format(" size: %d, Payload: ",payloadSize);
                    if (payloadSize > 15) {
                        received += payloadText.substring(0,15);
                    }
                    else
                    {
                        received += payloadText;
                    }
                    received += "\n";

                    // Update the GUI
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            textViewReceive.setText(received);
                            // Jay L. Gao: makes sure scroll view rolls to bottom to show new content
                            receivedScrollView.fullScroll(ScrollView.FOCUS_DOWN);
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
                    // Jay L. Gao: for large payload, it will not be received as byte array
                    //             but handled here as (file?)
                    receivedPayloadCount++;
                    while ((line = br.readLine()) != null) {
                        received += "Source: " + b.getEID() + ", Count: " + String.valueOf(receivedPayloadCount);
                        received += ", Size: " + String.valueOf(line.length()) + ", Line: ";
                        if (line.length() > 15){
                            received += line.substring(0,15);
                        }
                        else
                        {
                            received += line;
                        }
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
                        // Jay L. Gao: makes sure scroll view rolls to bottom to show new content
                        receivedScrollView.fullScroll(ScrollView.FOCUS_DOWN);
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
            // Jay L. Gao
            numPayloadToSend.setEnabled(true);
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
