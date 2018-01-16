package gov.nasa.jpl.iondtn.gui;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.MenuItem;

import gov.nasa.jpl.iondtn.R;

/**
 * This Activity shows displays information about the app and the author
 *
 * @author Robert Wiewel
 */
public class AboutActivity extends AppCompatActivity {

    /**
     * Creates the activity and initializes the internal representation
     * @param savedInstanceState A previous internal representation as a Bundle
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about);

        // enable the back button in the ActionBar
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        }
    }

    /**
     * Defines behavior for option menu selections. Necessary for leaving the
     * AboutActivity via the GUI
     * @param item The selected item
     * @return whether the tap was handled
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            // Respond to the action bar's Up/Home button
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
