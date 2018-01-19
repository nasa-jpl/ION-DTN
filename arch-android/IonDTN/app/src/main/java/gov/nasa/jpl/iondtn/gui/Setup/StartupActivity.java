package gov.nasa.jpl.iondtn.gui.Setup;

import android.content.pm.PackageManager;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.ConfigFileManager;
import gov.nasa.jpl.iondtn.gui.MainActivity;

/**
 * The activity that handles the setup process.
 *
 * @author Robert Wiewel
 */
public class StartupActivity extends AppCompatActivity {
    private static final String TAG = "StartupAct";
    private ConfigFileManager fileMgr;

    /**
     * Creates a new object of this type
     * @param savedInstanceState An (optional) previously saved instance of
     *                           this fragment
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_startup);
        fileMgr = new ConfigFileManager(getApplicationContext());

        // Check that the activity is using the layout version with
        // the fragment_container FrameLayout
        if (findViewById(R.id.fragment_container_startup) != null) {

            // However, if we're being restored from a previous state,
            // then we don't need to do anything and should return or else
            // we could end up with overlapping fragments.
            if (savedInstanceState != null) {
                Log.d(TAG, "onCreate: saved instance existed and was discarded!");
            }
            else {
                getSupportFragmentManager().beginTransaction().replace(R.id
                        .fragment_container_startup, new
                        StartPageFragment()).commit();
            }
        }
    }

    /**
     * Called when the object is destroyed
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    /**
     * Handles the result of the permission request dialogue
     * @param requestCode The request code that we set before
     * @param permissions Identifiers of permissions that were requested
     * @param grantResults The result of the permissions request
     */
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[]
            permissions, @NonNull int[] grantResults) {
        switch (requestCode) {
            case MainActivity.REQUEST_READ_PERMISSION: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {

                    return;

                }
            }
        }

        Toast.makeText(getApplicationContext(), "Please grant permission! " +
                "App won't work otherwise", Toast.LENGTH_LONG)
                .show();
    }

    public ConfigFileManager getFileManager() {
        return this.fileMgr;
    }
}
