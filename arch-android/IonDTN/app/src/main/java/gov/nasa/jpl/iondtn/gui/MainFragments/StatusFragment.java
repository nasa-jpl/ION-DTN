package gov.nasa.jpl.iondtn.gui.MainFragments;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.support.constraint.ConstraintLayout;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.ScrollView;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import org.apache.commons.io.input.ReversedLinesFileReader;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;
import java.nio.charset.Charset;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;
import gov.nasa.jpl.iondtn.gui.MainActivity;


/**
 * Fragment that handles the lifecycle of ION
 *
 * Allows starting and stopping and has a log output element to review the
 * ION operations
 *
 * @author Robert Wiewel
 */
public class StatusFragment extends ConfigurationFragment {
    private TextView textViewLog;
    private TextView textViewStatus;
    private Switch switchION;
    private ScrollView scrollLog;
    ConstraintLayout cl;
    Thread logUpdateThread;
    private View v = null;
    private static final String TAG = "StatusFragment";

    /**
     * Factory method to create a new fragment instance
     * @return a new instance of the StatusFragment
     */
    public static StatusFragment newInstance() {
        return new StatusFragment();
    }

    /**
     * Populates the layout of the fragment
     * @param inflater The inflater
     * @param container The ViewGroup container
     * @param savedInstanceState An (optional) previously saved instance of
     *                           this fragment
     * @return A populated view
     */
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        v = inflater.inflate(R.layout.fragment_status, container, false);

        textViewLog = v.findViewById(R.id.textViewLog);
        textViewStatus = v.findViewById(R.id.textViewStatus);
        switchION = v.findViewById(R.id.switchION);

        cl = v.findViewById(R.id.status_layout);
        switchION = v.findViewById(R.id.switchION);
        scrollLog = v.findViewById(R.id.scrollLog);


        setStatus(NativeAdapter.getStatus());

        cl.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                switchION.performClick();
            }
        });

        TextView clearLog = v.findViewById(R.id.textViewClearLog);

        clearLog.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // Create target directory
                File filesDir = getContext().getFilesDir();
                File logfile = new File(filesDir, "ion.log");

                // Delete existing config files if present
                try {
                    PrintWriter writer = new PrintWriter(logfile);
                    writer.print("");
                    writer.close();

                }
                catch (FileNotFoundException e) {
                    Log.d(TAG, "Logfile not found! ION probably never " +
                            "started!");
                }
            }
        });



        textViewLog.setMovementMethod(new ScrollingMovementMethod());

        switchION.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                if (b && (NativeAdapter.getStatus() == NativeAdapter.SystemStatus
                        .STOPPED)) {
                    if (getActivity() instanceof MainActivity) {
                        try {
                            if (getActivity() != null &&
                                    ((MainActivity) getActivity())
                                            .getAdminService() != null) {
                                ((MainActivity) getActivity()).getAdminService()
                                        .startION();
                            }
                        }
                        catch (RemoteException e){
                            Toast.makeText(getContext(), getString(R.string
                                    .errorStartIonMessage), Toast
                                    .LENGTH_LONG).show();
                        }
                    }
                    else {
                        Toast.makeText(getContext(), getString(R.string
                                .errorStartIonMessage), Toast
                                .LENGTH_LONG).show();
                    }
                } else if (!b && NativeAdapter.getStatus()  == NativeAdapter.SystemStatus.STARTED) {
                    if (getActivity() instanceof MainActivity) {
                        try {
                            ((MainActivity) getActivity()).getAdminService()
                                    .stopION();
                        }
                        catch (RemoteException e){
                            Toast.makeText(getContext(), getString(R.string
                                    .errorStopIonMessage), Toast
                                    .LENGTH_LONG).show();
                        }
                    }
                    else {
                        Toast.makeText(getContext(), getString(R.string
                                .errorStopIonMessage), Toast
                                .LENGTH_LONG).show();
                    }
                }
            }
        });

        return v;
    }

    /**
     * Handles the graphical representation of ION status changes
     * @param status The new ION status
     */
    @SuppressWarnings("deprecation")
    public void setStatus(NativeAdapter.SystemStatus status) {
        if (v == null) {
            return;
        }

        switch (status) {
            case STARTING:
                textViewStatus.setText(R.string.label_status_starting);
                textViewStatus.setTextColor(getResources().getColor(R
                        .color.PendingOperationOrange));
                switchION.setEnabled(false);
                cl.setClickable(false);
                cl.setEnabled(false);
                switchION.setChecked(true);
                break;

            case STARTED:
                textViewStatus.setText(R.string.label_status_started);
                textViewStatus.setTextColor(getResources().getColor(R
                        .color.StartedGreen));
                switchION.setEnabled(true);
                switchION.setChecked(true);
                cl.setClickable(true);
                cl.setEnabled(true);
                switchION.setChecked(true);
                break;

            case STOPPING:
                textViewStatus.setText(R.string.label_status_stopping);
                textViewStatus.setTextColor(getResources().getColor(R
                        .color.PendingOperationOrange));
                switchION.setEnabled(false);
                cl.setClickable(false);
                cl.setEnabled(false);
                switchION.setChecked(false);
                break;

            case STOPPED:
                textViewStatus.setText(R.string.label_status_stopped);
                textViewStatus.setTextColor(getResources().getColor(R
                        .color.StoppedRed));
                switchION.setEnabled(true);
                switchION.setChecked(false);
                cl.setClickable(true);
                cl.setEnabled(true);

                if (switchION.isChecked()) {
                    Toast.makeText(getContext(), "Starting ION failed! Check " +
                                    "configuration!",
                            Toast.LENGTH_LONG).show();
                    switchION.setChecked(false);
                }
                break;
        }

    }

    /**
     * Handler function for ION status changes
     * @param status The changed status
     */
    @Override
    public void onIonStatusUpdate(NativeAdapter.SystemStatus status) {
        setStatus(status);
    }

    /**
     * Starts fragment and sets title
     */
    @Override
    public void onStart() {
        super.onStart();
        setStatus(NativeAdapter.getStatus());

        logUpdateThread = new Thread() {
            StringBuilder log;
            int length;
            @Override
            public void run() {
                int msleepValue;
                int log_tail_lines;

                SharedPreferences sharedPref = PreferenceManager
                        .getDefaultSharedPreferences(getActivity()
                                .getApplicationContext());
                msleepValue = Integer.parseInt(sharedPref.getString
                        (getString(R.string.refresh_rate_log),
                        "1000"));
                log_tail_lines = Integer.parseInt(sharedPref.getString
                        (getString(R.string.log_tail_lines),
                                "200"));

                try {
                    while (!isInterrupted()) {
                        log = new StringBuilder();
                        try {
                            String logPath = getContext().getFilesDir() +
                                    "/ion.log";

                            File logFile = new File(logPath);

                            ReversedLinesFileReader reader = new
                                    ReversedLinesFileReader(logFile, Charset.defaultCharset());

                            String line;
                            int line_count = 0;
                            while ((line = reader.readLine()) != null &&
                                    line_count < log_tail_lines) {
                                log.insert(0, line);
                                log.insert(line.length(), '\n');
                                line_count++;
                            }
                            reader.close();

                        } catch (IOException e) {
                            Log.d(TAG, "Log file couldn't be read! Probably " +
                                    "not created yet!");
                        } catch (IllegalStateException e) {
                            Log.d(TAG, "Log file was cleared while reading! " +
                                    "Try again later");
                        }
                        if (getActivity() == null) {
                            break;
                        }
                        getActivity().runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                textViewLog.setText(log.toString());
                                if (log.toString().length() != length) {
                                    scrollLog.fullScroll(View.FOCUS_DOWN);
                                    length = log.toString().length();
                                }
                            }
                        });
                        Thread.sleep(msleepValue);
                    }
                } catch (InterruptedException e) {
                    Log.d(TAG, "run: Interrupted thread!");
                }
            }
        };

        logUpdateThread.start();
        getActivity().setTitle("IonDTN");
    }

    /**
     * Callback function when the NodeAdministrationService becomes available
     */
    @Override
    public void onServiceAvailable() {
        // do nothing!
    }

    /**
     * Handles the teardown of the StatusFragment
     */
    @Override
    public void onDestroyView() {
        logUpdateThread.interrupt();
        super.onDestroyView();
    }

    /**
     * Handles the shutdown of the StatusFragment
     */
    @Override
    public void onStop() {
        logUpdateThread.interrupt();
        super.onStop();
    }
}
