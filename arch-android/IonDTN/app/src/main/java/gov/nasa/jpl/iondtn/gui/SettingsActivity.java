package gov.nasa.jpl.iondtn.gui;


import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.SwitchPreference;
import android.support.v7.app.ActionBar;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.support.v7.app.AlertDialog;
import android.view.MenuItem;
import android.widget.Toast;

import java.util.List;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;

/**
 * Standard Android General Settings Activity
 *
 * A {@link PreferenceActivity} that presents a set of application settings. On
 * handset devices, settings are presented as a single list. On tablets,
 * settings are split by category, with category headers shown to the left of
 * the list of settings.
 * <p>
 * See <a href="http://developer.android.com/design/patterns/settings.html">
 * Android Design: Settings</a> for design guidelines and the <a
 * href="http://developer.android.com/guide/topics/ui/settings.html">Settings
 * API Guide</a> for more information on developing a Settings UI.
 */
public class SettingsActivity extends AppCompatPreferenceActivity{
    /**
     * A preference value change listener that updates the preference's summary
     * to reflect its new value.
     */
    private static Preference.OnPreferenceChangeListener sBindPreferenceSummaryToValueListener = new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange(Preference preference, Object value) {
            String stringValue = value.toString();

            if (preference instanceof ListPreference) {
                // For list preferences, look up the correct display value in
                // the preference's 'entries' list.
                ListPreference listPreference = (ListPreference) preference;
                int index = listPreference.findIndexOfValue(stringValue);

                // Set the summary to reflect the new value.
                preference.setSummary(
                        index >= 0
                                ? listPreference.getEntries()[index]
                                : null);
            } else {
                // For all other preferences, set the summary to the value's
                // simple string representation.
                preference.setSummary(stringValue);
            }
            return true;
        }
    };

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

    /**
     * Helper method to determine if the device has an extra-large screen. For
     * example, 10" tablets are extra-large.
     */
    private static boolean isXLargeTablet(Context context) {
        return (context.getResources().getConfiguration().screenLayout
                & Configuration.SCREENLAYOUT_SIZE_MASK) >= Configuration.SCREENLAYOUT_SIZE_XLARGE;
    }

    /**
     * Binds a preference's summary to its value. More specifically, when the
     * preference's value is changed, its summary (line of text below the
     * preference title) is updated to reflect the value. The summary is also
     * immediately updated upon calling this method. The exact display format is
     * dependent on the type of preference.
     *
     * @see #sBindPreferenceSummaryToValueListener
     */
    private static void bindPreferenceSummaryToValue(Preference preference) {
        // Set the listener to watch for value changes.
        preference.setOnPreferenceChangeListener(sBindPreferenceSummaryToValueListener);

        // Trigger the listener immediately with the preference's
        // current value.
        sBindPreferenceSummaryToValueListener.onPreferenceChange(preference,
                PreferenceManager
                        .getDefaultSharedPreferences(preference.getContext())
                        .getString(preference.getKey(), ""));
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setupActionBar();
    }

    /**
     * Set up the {@link android.app.ActionBar}, if the API is available.
     */
    private void setupActionBar() {
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            // Show the Up button in the action bar.
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean onIsMultiPane() {
        return isXLargeTablet(this);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    public void onBuildHeaders(List<Header> target) {
        loadHeadersFromResource(R.xml.pref_headers, target);
    }

    /**
     * This method stops fragment injection in malicious applications.
     * Make sure to deny any unknown fragments here.
     */
    protected boolean isValidFragment(String fragmentName) {
        return PreferenceFragment.class.getName().equals(fragmentName)
                || GeneralPreferenceFragment.class.getName().equals(fragmentName)
                || IPNDPreferenceFragment.class.getName().equals(fragmentName);
    }

    /**
     * This fragment shows general preferences only. It is used when the
     * activity is showing a two-pane settings UI.
     */
    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    public static class GeneralPreferenceFragment extends PreferenceFragment {
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.pref_general);
            setHasOptionsMenu(true);

            // Bind the summaries of EditText/List/Dialog/Ringtone preferences
            // to their values. When their values change, their summaries are
            // updated to reflect the new value, per the Android Design
            // guidelines.
            bindPreferenceSummaryToValue(findPreference(getString(R.string
                    .refresh_rate_log)));
            bindPreferenceSummaryToValue(findPreference(getString(R.string
                    .log_tail_lines)));
            bindPreferenceSummaryToValue(findPreference(getString(R.string
                    .threshold_data_file)));

            Preference button = findPreference(getString(R
                    .string.ResetButton));
            button.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {

                    if (NativeAdapter.getStatus() != NativeAdapter
                            .SystemStatus.STOPPED) {
                        Toast.makeText(getActivity().getApplicationContext(),
                                "Please stop the ION instance first!",
                                Toast.LENGTH_LONG).show();
                        return true;
                    }

                    AlertDialog.Builder builder = new AlertDialog.Builder(getActivity
                            ());
                    builder.setMessage(R.string.resetDialogMessage)
                            .setTitle(R.string.resetDialogHeader);

                    builder.setPositiveButton(R.string.dialogButtonConfirm,
                            new DialogInterface
                            .OnClickListener() {
                        @SuppressLint("ApplySharedPref")
                        public void onClick(DialogInterface dialog, int id) {
                            SharedPreferences sharedPref = getActivity().getApplicationContext()
                                    .getSharedPreferences(getString(R.string.preference_file_key)
                                            , Context.MODE_PRIVATE);
                            PreferenceManager.getDefaultSharedPreferences(getActivity
                                    ().getApplicationContext()).edit().clear().commit();
                            PreferenceManager.setDefaultValues(getActivity()
                                    .getApplicationContext(), R.xml.pref_general, true);
                            sharedPref.edit().clear().commit();
                            System.exit(0);
                            // App gets restarted automatically due to service

                        }
                    });
                    builder.setNegativeButton(R.string.dialogButtonCancel, new DialogInterface
                            .OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            dialog.dismiss();
                        }
                    });
                    AlertDialog dialog = builder.create();
                    dialog.show();

                    return true;
                }
            });
        }


            @Override
        public boolean onOptionsItemSelected(MenuItem item) {
            int id = item.getItemId();
            if (id == android.R.id.home) {
                getFragmentManager().popBackStack();
                return true;
            }
            return super.onOptionsItemSelected(item);
        }
    }

    /**
     * This fragment shows general preferences only. It is used when the
     * activity is showing a two-pane settings UI.
     */
    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    public static class IPNDPreferenceFragment extends PreferenceFragment {
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.pref_ipnd);
            setHasOptionsMenu(true);

            final SwitchPreference prefIpndSwitch = (SwitchPreference) findPreference
                    ("pref_ipnd_on_off");
            final Preference prefIpndListenPort = findPreference
                    ("pref_ipnd_listen_port");
            final CheckBoxPreference prefIpndIpListenAutoupdate =
                    (CheckBoxPreference) findPreference("pref_ipnd_ip_listen_autoupdate");
            final Preference prefListenAdr = findPreference
                    ("pref_ipnd_listen_adr");

            final CheckBoxPreference prefIpndEIDAutoupdate =
                    (CheckBoxPreference) findPreference("pref_ipnd_eid_autoupdate");
            final Preference prefIpndPropagatedEid = findPreference
                    ("pref_ipnd_propagated_eid");
            final Preference prefIpndIntervalUnicast = findPreference
                    ("pref_ipnd_interval_unicast");
            final Preference prefIpndIntervalMulticast = findPreference
                    ("pref_ipnd_interval_multicast");
            final Preference prefIpndIntervalBroadcast = findPreference
                    ("pref_ipnd_interval_broadcast");
            final Preference prefIpndTtlMulticast = findPreference
                    ("pref_ipnd_ttl_multicast");
            final Preference prefIpndDestinationIp = findPreference
                    ("pref_ipnd_destination_ip");

            final CheckBoxPreference prefIpndTcpclPropagation =
                    (CheckBoxPreference) findPreference("pref_ipnd_tcpcl_propagation");
            final Preference prefIpndTcpclPort = findPreference
                    ("pref_ipnd_tcpcl_port");

            bindPreferenceSummaryToValue(findPreference("pref_ipnd_listen_port"));
            bindPreferenceSummaryToValue(findPreference("pref_ipnd_listen_adr"));
            bindPreferenceSummaryToValue(findPreference("pref_ipnd_propagated_eid"));
            bindPreferenceSummaryToValue(findPreference("pref_ipnd_interval_unicast"));
            bindPreferenceSummaryToValue(findPreference("pref_ipnd_interval_multicast"));
            bindPreferenceSummaryToValue(findPreference("pref_ipnd_interval_broadcast"));
            bindPreferenceSummaryToValue(findPreference("pref_ipnd_ttl_multicast"));
            bindPreferenceSummaryToValue(findPreference("pref_ipnd_destination_ip"));
            bindPreferenceSummaryToValue(findPreference("pref_ipnd_tcpcl_port"));
            
            prefIpndSwitch.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {


                @Override
                public boolean onPreferenceChange(Preference preference,
                                                  Object o) {
                    PreferenceCategory listening = (PreferenceCategory)
                            findPreference("listening");
                    PreferenceCategory beacon = (PreferenceCategory)
                            findPreference("beacons");
                    boolean newValue = (boolean) o;
                    if (newValue) {
                        listening.setEnabled(true);
                        beacon.setEnabled(true);
                    }
                    else {
                        listening.setEnabled(false);
                        beacon.setEnabled(false);
                    }
                    return true;
                }
            });

            prefIpndIpListenAutoupdate.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference,
                                                  Object o) {
                    boolean newValue = (boolean) o;
                    if (newValue) {
                        prefListenAdr.setEnabled(false);
                    }
                    else {
                        prefListenAdr.setEnabled(true);
                    }
                    return true;
                }
            });

            prefIpndEIDAutoupdate.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference,
                                                  Object o) {
                    boolean newValue = (boolean) o;
                    if (newValue) {
                        prefIpndPropagatedEid.setEnabled(false);
                    }
                    else {
                        prefIpndPropagatedEid.setEnabled(true);
                    }
                    return true;
                }
            });

            prefIpndTcpclPropagation.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference,
                                                  Object o) {
                    boolean newValue = (boolean) o;
                    if (newValue) {
                        prefIpndTcpclPort.setEnabled(true);
                    }
                    else {
                        prefIpndTcpclPort.setEnabled(false);
                    }
                    return true;
                }
            });

            if (NativeAdapter.getStatus() != NativeAdapter.SystemStatus
                    .STOPPED) {
                Toast.makeText(getActivity().getApplicationContext(), "Only editable when ION is " +
                            "stopped",
                    Toast.LENGTH_LONG).show();

                // Trigger onChange listener
                prefIpndSwitch.getOnPreferenceChangeListener()
                        .onPreferenceChange(prefIpndSwitch, false);
                prefIpndSwitch.setEnabled(false);
            }
            else {
                prefIpndSwitch.setEnabled(true);
                // Trigger onChange listener
                prefIpndSwitch.getOnPreferenceChangeListener()
                        .onPreferenceChange(prefIpndSwitch, prefIpndSwitch.isChecked());

                prefIpndIpListenAutoupdate.getOnPreferenceChangeListener()
                        .onPreferenceChange(prefIpndIpListenAutoupdate, prefIpndIpListenAutoupdate.isChecked());
                prefIpndEIDAutoupdate.getOnPreferenceChangeListener()
                        .onPreferenceChange(prefIpndEIDAutoupdate, prefIpndEIDAutoupdate.isChecked());
                prefIpndTcpclPropagation.getOnPreferenceChangeListener()
                        .onPreferenceChange(prefIpndTcpclPropagation, prefIpndTcpclPropagation.isChecked());

                prefIpndSwitch.setEnabled(true);
            }

        }


        @Override
        public void onStart() {
            super.onStart();
        }

        @Override
        public boolean onOptionsItemSelected(MenuItem item) {
            int id = item.getItemId();
            if (id == android.R.id.home) {
                getFragmentManager().popBackStack();
                return true;
            }
            return super.onOptionsItemSelected(item);
        }
    }
}
