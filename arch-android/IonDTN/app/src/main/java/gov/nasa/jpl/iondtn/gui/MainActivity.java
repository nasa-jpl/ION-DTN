package gov.nasa.jpl.iondtn.gui;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.support.annotation.NonNull;
import android.util.Log;
import android.support.design.widget.NavigationView;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

import gov.nasa.jpl.iondtn.INodeAdminListener;
import gov.nasa.jpl.iondtn.INodeAdminService;
import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;
import gov.nasa.jpl.iondtn.gui.MainFragments.ConfigurationFragment;
import gov.nasa.jpl.iondtn.gui.MainFragments.ContactFragment;
import gov.nasa.jpl.iondtn.gui.MainFragments.EndpointFragment;
import gov.nasa.jpl.iondtn.gui.MainFragments.InOutductFragment;
import gov.nasa.jpl.iondtn.gui.MainFragments.ProtocolFragment;
import gov.nasa.jpl.iondtn.gui.MainFragments.RangeFragment;
import gov.nasa.jpl.iondtn.gui.MainFragments.SchemeFragment;
import gov.nasa.jpl.iondtn.gui.MainFragments.SecurityFragment;
import gov.nasa.jpl.iondtn.gui.MainFragments.StatusFragment;
import gov.nasa.jpl.iondtn.gui.Setup.StartupActivity;
import gov.nasa.jpl.iondtn.services.NodeAdministrationService;

/**
 * Main Application class. Handles start/stop of ION-DTN and configuration of
 * node
 *
 * @author Robert Wiewel
 */
public class MainActivity extends AppCompatActivity
        implements NavigationView.OnNavigationItemSelectedListener,
        FragmentFeedback {

    private NavigationView navigationView;
    public static final int REQUEST_READ_PERMISSION = 786;
    private final static String TAG = "MainActivity";
    private gov.nasa.jpl.iondtn.INodeAdminService mService;
    
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("iondtn");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SharedPreferences sharedPref = getApplicationContext()
                .getSharedPreferences(getString(R.string.preference_file_key)
                        , Context.MODE_PRIVATE);

        if (!sharedPref.contains("setup complete")) {
            Intent setup_intent = new Intent(getApplicationContext(),
                    StartupActivity.class);
            setup_intent.setFlags(setup_intent.getFlags()
                    | Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_TASK_ON_HOME);
            startActivity(setup_intent);
            finish();
            return;
        }

        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        DrawerLayout drawer = findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawer.addDrawerListener(toggle);
        toggle.syncState();

        navigationView = findViewById(R.id.nav_view);
        navigationView.setNavigationItemSelectedListener(this);

        navigationView.setCheckedItem(R.id.nav_status);

        Intent i = new Intent(getApplicationContext(),
                NodeAdministrationService.class);
        startService(i);

        updateStatusChanged(NativeAdapter.getStatus().toString());

        // Check that the activity is using the layout version with
        // the fragment_container FrameLayout
        if (findViewById(R.id.fragment_container) != null) {

            // However, if we're being restored from a previous state,
            // then we don't need to do anything and should return or else
            // we could end up with overlapping fragments.
            if (savedInstanceState == null) {
                getSupportFragmentManager().beginTransaction().add(R.id
                        .fragment_container, new StatusFragment()).commit();
            }
        }
    }

    /**
     * Gets called whenever the back button is pressed in order to evaluate
     * the required behaviour
     */
    @Override
    public void onBackPressed() {
        DrawerLayout drawer = findViewById(R.id.drawer_layout);

        navigationView.setCheckedItem(R.id.nav_status);

        if (getSupportActionBar() != null) {
            getSupportActionBar().setTitle("IonDTN");
        }

        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
        } else {
            super.onBackPressed();
        }

    }

    /**
     * Handles selections in the navigation drawer and thus allows navigation
     * between fragments
     * @param item The selected item
     * @return true if the selection was handled properly
     */
    @SuppressWarnings("StatementWithEmptyBody")
    @Override
    public boolean onNavigationItemSelected(@NonNull MenuItem item) {
        // Handle navigation view item clicks here.
        int id = item.getItemId();

        DrawerLayout drawer = findViewById(R.id.drawer_layout);

        switch (id) {
            case R.id.nav_about:
                Intent about_intent = new Intent(this, AboutActivity.class);
                startActivity(about_intent);
                break;

            case R.id.nav_contact:
                while (getSupportFragmentManager().popBackStackImmediate()) {}
                getSupportFragmentManager().beginTransaction()
                        .addToBackStack("ct").replace(R.id
                        .fragment_container, new
                        ContactFragment()).commit();
                break;

            case R.id.nav_range:
                while (getSupportFragmentManager().popBackStackImmediate()) {}
                getSupportFragmentManager().beginTransaction()
                        .addToBackStack("rf").replace(R.id
                        .fragment_container, new
                        RangeFragment()).commit();
                break;

            case R.id.nav_scheme:
                while (getSupportFragmentManager().popBackStackImmediate()) {}
                getSupportFragmentManager().beginTransaction()
                        .addToBackStack("sf").replace(R.id
                        .fragment_container, new
                        SchemeFragment()).commit();
                break;

            case R.id.nav_endpoint:
                while (getSupportFragmentManager().popBackStackImmediate()) {}
                getSupportFragmentManager().beginTransaction()
                        .addToBackStack("ef").replace(R.id
                        .fragment_container, new
                        EndpointFragment()).commit();
                break;

            case R.id.nav_protocol:
                while (getSupportFragmentManager().popBackStackImmediate()) {}
                getSupportFragmentManager().beginTransaction()
                        .addToBackStack("ef").replace(R.id
                        .fragment_container, new
                        ProtocolFragment()).commit();
                break;

            case R.id.nav_inoutduct:
                while (getSupportFragmentManager().popBackStackImmediate()) {}
                getSupportFragmentManager().beginTransaction()
                        .addToBackStack("iof").replace(R.id
                        .fragment_container, new
                        InOutductFragment()).commit();
                break;

            case R.id.nav_security:
                while (getSupportFragmentManager().popBackStackImmediate()) {}
                getSupportFragmentManager().beginTransaction()
                        .addToBackStack("sf").replace(R.id
                        .fragment_container, new
                        SecurityFragment()).commit();
                break;

            case R.id.nav_status:
                if (getSupportActionBar() != null) {
                    getSupportActionBar().setTitle("IonDTN");
                }
                while (getSupportFragmentManager().popBackStackImmediate()) {}
                break;

            case R.id.nav_settings:
                Intent settings_intent = new Intent(this, SettingsActivity.class);
                startActivity(settings_intent);
                break;
        }

        drawer.closeDrawer(GravityCompat.START);
        invalidateOptionsMenu();
        return true;
    }

    /**
     * Deactivates all menu items of the navigation drawer
     */
    public void deactivateDrawerMenu() {
        NavigationView navigationView= findViewById(R.id.nav_view);

        Menu menuNav=navigationView.getMenu();
        menuNav.setGroupEnabled(R.id.menu_config, false);
        menuNav.setGroupEnabled(R.id.menu_general, false);
    }

    /**
     * Activates all menu items of the navgation drawer
     * @param partial Specifies if everything should be activated or only the
     *                items that are allowed when ION is switched off
     */
    public void activateDrawerMenu(boolean partial) {
        NavigationView navigationView= findViewById(R.id.nav_view);

        Menu menuNav=navigationView.getMenu();

        if (!partial) {
            menuNav.setGroupEnabled(R.id.menu_config, true);
        }
        else {
            menuNav.setGroupEnabled(R.id.menu_config, false);
            MenuItem item = menuNav.findItem(R.id.nav_status);

            item.setEnabled(true);
        }
        menuNav.setGroupEnabled(R.id.menu_general, true);
    }

    /**
     * Handle status changes of the ION-DTN instance
     * @param status the changed status
     */
    public void updateStatusChanged(String status){
        Log.e(TAG, "updateStatusChanged: Status changed");
        NativeAdapter.SystemStatus stat = NativeAdapter.SystemStatus
                .valueOf(status);

        android.support.v4.app.Fragment fg = getSupportFragmentManager()
                .findFragmentById
                (R.id.fragment_container);
        if (fg != null && fg instanceof ConfigurationFragment) {
            ConfigurationFragment cfg = (ConfigurationFragment) fg;
            cfg.onIonStatusUpdate(stat);
        }

        switch (stat) {
            case STARTING:
            case STOPPING:
                deactivateDrawerMenu();
                break;

            case STARTED:
                activateDrawerMenu(false);
                break;

            case STOPPED:
                activateDrawerMenu(true);
                break;
        }
    }

    private final INodeAdminListener.Stub listener = new INodeAdminListener.Stub
            () {

        @Override
        public void notifyStatusChanged(String status) throws RemoteException {
            updateStatusChanged(status);
        }
    };

    private ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            Log.d(TAG, "onServiceConnected: Service bound!\n");
            mService = gov.nasa.jpl.iondtn.INodeAdminService.Stub.asInterface(service);

            try {
                mService.registerStatusChangeListener(listener);
            }
            catch (RemoteException e) {
                Toast.makeText(getApplicationContext(), "Failed to register " +
                        "status listener!", Toast
                        .LENGTH_SHORT).show();
            }

            android.support.v4.app.Fragment fg = getSupportFragmentManager()
                    .findFragmentById
                            (R.id.fragment_container);
            if (fg != null && fg instanceof ConfigurationFragment) {
                ConfigurationFragment cfg = (ConfigurationFragment) fg;
                cfg.onServiceAvailable();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            mService = null;
            // This method is only invoked when the service quits from the other end or gets killed
            // Invoking exit() from the AIDL interface makes the Service kill itself, thus invoking this.
            Log.d(TAG, "onServiceDisconnected: Service disconnected.\n");
        }
    };

    /**
     * Defines the behaviour when the activity is destroyed
     */
    @Override
    protected void onDestroy() {
        if (mService != null) {
            try {
                mService.unregisterStatusChangeListener(listener);
            } catch (RemoteException e) {
                Toast.makeText(getApplicationContext(), "Failed to unregister" +
                        " " +
                        "status listener!", Toast
                        .LENGTH_SHORT).show();
            }
        }
        super.onDestroy();
    }

    /**
     * Defines the behaviour when the activity is started
     */
    @Override
    protected void onStart() {
        Intent i = new Intent(getApplicationContext(),
                NodeAdministrationService.class);
        Log.d(TAG, "onStart: binding to NodeAdministrationService");
        bindService(i, mConnection, BIND_AUTO_CREATE);


        super.onStart();
    }

    /**
     * Defines the behaviour when the activity is stopped
     */
    @Override
    protected void onStop() {
        unbindService(mConnection);
        super.onStop();
    }

    /**
     * Returns the administration service of the activity (which is required
     * by the fragments and other parts of the application for configuring
     * the ION node
     */
    public INodeAdminService getAdminService() {
        return mService;
    }

    /**
     * Handles title changes
     * @param title The new title of the activities window
     * @param color The new color of the activities window
     */
    @Override
    protected void onTitleChanged(CharSequence title, int color) {
        super.onTitleChanged(title, color);

        // Set selected NavBar item to status (as the backstack is empty)
        // (necessary, because pressing the back button doesn't change the
        // NavBar selection)
        if (navigationView != null && title == "IonDTN") {
            navigationView.setCheckedItem(R.id.nav_status);
        }
    }
}
