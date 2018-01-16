package gov.nasa.jpl.camerashare;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {
    // GUI objects
    EditText editOwnShareEID;
    EditText editRemoteShareEID;
    RadioButton radioOpenOwnShare;
    RadioButton radioOpenRemoteShare;
    Button confirmButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Initialize parent class
        super.onCreate(savedInstanceState);

        // Inflate layout of activity
        setContentView(R.layout.dialog_start);

        // Bind the GUI elements to their objects
        editOwnShareEID = findViewById(R.id.editsink);
        editRemoteShareEID = findViewById(R.id.editjoineid);
        radioOpenOwnShare = findViewById(R.id.radioButton2);
        radioOpenRemoteShare = findViewById(R.id.radioButton);
        confirmButton = findViewById(R.id.button);

        // Load the shared preferences and check if preferences already
        // exist. If that is the case, load the settings into the GUI elements
        SharedPreferences sharedPref = getApplicationContext()
                .getSharedPreferences(getString(R.string.preference_file_key)
                        , Context.MODE_PRIVATE);
        if (sharedPref.contains("own_sink_eid")) {
            editOwnShareEID.setText(sharedPref.getString("own_sink_eid",
                    "ipn:1.1"));
        }
        if (sharedPref.contains("last_remote_eid")) {
            editRemoteShareEID.setText(sharedPref.getString("last_remote_eid",
                    "ipn:1.1"));
        }

        // (De-)Activate GUI elements
        editRemoteShareEID.setEnabled(false);
        radioOpenOwnShare.setChecked(true);

        // Set a change listener for the radiobutton to (de-)activiate GUI
        // elements
        radioOpenOwnShare.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                if (b) {
                    editRemoteShareEID.setEnabled(false);
                }
                else {
                    editRemoteShareEID.setEnabled(true);
                }
            }
        });

        // Set the onClick listener for the setup button (that launches
        // either the CameraActivity or the GalleryActivity)
        confirmButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                // Sanity check
                if (editOwnShareEID.getText().length() == 0 ||
                        (editRemoteShareEID.getText().length() == 0 &&
                                radioOpenRemoteShare.isChecked())) {
                    Toast.makeText(getApplicationContext(), "Please enter " +
                            "required EIDs!", Toast.LENGTH_LONG).show();
                    return;
                }

                // Load shared preferences and set certain values
                SharedPreferences sharedPref = getApplicationContext()
                        .getSharedPreferences(getString(R.string.preference_file_key)
                                , Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putBoolean("local", radioOpenOwnShare.isChecked());
                editor.putString("own_sink_eid", editOwnShareEID.getText().toString());
                editor.putString("remote_sink_eid", editRemoteShareEID.getText().toString());
                editor.apply();

                // Determine state of GUI radio button to choose child activity
                if(radioOpenOwnShare.isChecked()) {
                    // Start GalleryActivity intent
                    Intent galleryIntent = new Intent(getApplicationContext(),
                            GalleryActivity.class);
                    startActivity(galleryIntent);
                }
                else {
                    // Start CameraActivity intent
                    Intent cameraIntent = new Intent(getApplicationContext(),
                            CameraActivity.class);
                    startActivity(cameraIntent);
                }
            }
        });
    }


}
