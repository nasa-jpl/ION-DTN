package gov.nasa.jpl.iondtn.gui.Setup;


import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.gui.MainActivity;

/**
 * {@link Fragment} that requests the filesystem read permission from the
 * user during the setup process.
 *
 * @author Robert Wiewel
 */
public class SetupCompleteFragment extends Fragment {
    private static final String TAG = "SetupComplFrag";

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
        View v = inflater.inflate(R.layout.fragment_setup_complete, container,
                false);

        FloatingActionButton completeButton = v.findViewById(R.id
                .floatingActionButton);

        completeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                SharedPreferences sharedPref = getActivity().getApplicationContext()
                        .getSharedPreferences(getString(R.string.preference_file_key)
                                , Context.MODE_PRIVATE);

                StartupActivity act = (StartupActivity)getActivity();

                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putBoolean("setup complete", true);
                editor.putString("startup path", act.getFileManager()
                        .getStartupPath());
                Log.d(TAG, "onClick: Startup Path: " + act.getFileManager()
                        .getStartupPath());
                editor.putString("init path", act.getFileManager()
                        .getInitPath());
                Log.d(TAG, "onClick: Init Path: " + act.getFileManager()
                        .getInitPath());
                editor.apply();

                Intent main_intent = new Intent(getActivity()
                        .getApplicationContext(),
                        MainActivity.class);
                main_intent.setFlags(main_intent.getFlags() | Intent
                        .FLAG_ACTIVITY_CLEAR_TASK | Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(main_intent);
            }
        });

        return v;
    }

}
