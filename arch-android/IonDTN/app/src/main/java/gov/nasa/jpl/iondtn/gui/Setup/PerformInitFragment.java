package gov.nasa.jpl.iondtn.gui.Setup;


import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.ConfigFileManager;

/**
 * {@link Fragment} that performs the actual initialization step of the setup
 * routine
 *
 * @author Robert Wiewel
 */
public class PerformInitFragment extends Fragment implements
        ConfigFileManager.ConfigFileManagerListener {

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
        StartupActivity act = (StartupActivity)getActivity();
        act.getFileManager().setListener(this);
        act.getFileManager().runAsyncTask();
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_perform_init, container, false);
    }

    /**
     * Function that handles the return call when the
     * {@link android.os.AsyncTask} of the setup has finished
     */
    @Override
    public void notifyFinished(boolean status) {
        if (!status) {
            getActivity().getSupportFragmentManager()
                    .beginTransaction()
                    .setCustomAnimations(android.R
                            .anim.fade_in, android.R.anim.fade_out)
                    .replace(R.id.fragment_container_startup, new
                            SelectNodeFragment()).commit();
            Toast.makeText(getContext(),
                    "Initialization failed! Please try again!",
                    Toast.LENGTH_LONG).show();
            return;
        }

        getActivity().getSupportFragmentManager()
                .beginTransaction()
                .setCustomAnimations(android.R
                        .anim.fade_in, android.R.anim.fade_out)
                .replace(R.id.fragment_container_startup, new
                        SetupCompleteFragment()).commit();
    }
}
