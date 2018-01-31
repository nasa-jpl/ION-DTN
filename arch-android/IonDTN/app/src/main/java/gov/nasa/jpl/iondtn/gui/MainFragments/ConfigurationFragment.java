package gov.nasa.jpl.iondtn.gui.MainFragments;


import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;
import gov.nasa.jpl.iondtn.gui.OnIonStatusChangeListener;

/**
 * Base class for all configuration/status fragments used in IonDTN
 *
 * Modifies behaviour including update routines when the ION status changed
 *
 * @author Robert Wiewel
 */
public abstract class ConfigurationFragment
        extends Fragment
        implements OnIonStatusChangeListener{

    protected static final String TAG = "ConfigurationFragment";

    /**
     * Creates the fragment (including internal objects), also logs this
     * operation
     * @param savedInstanceState A previously saved state as Bundle
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.v(TAG, "onCreate: Fragment created");
        super.onCreate(savedInstanceState);
    }

    /**
     * Destroys an ConfigurationFragment object, also logs this operation
     */
    @Override
    public void onDestroy() {
        Log.v(TAG, "onDestroy: Fragment destroyed");
        super.onDestroy();
    }

    /**
     * Creates the actual ConfigurationFragment view
     * @param inflater The inflater for the view
     * @param container The ViewGroup where the fragment should be populated
     * @param savedInstanceState An previous instance of the object
     * @return A populated view of the ConfigurationFragment
     */
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        TextView textView = new TextView(getActivity());
        onIonStatusUpdate(NativeAdapter.getStatus());
        getActivity().setTitle("ConfigurationFragment");
        return textView;
    }

    /**
     * Handles ION status changes when fragment is active/visible
     * @param status The changed status
     */
    public abstract void onIonStatusUpdate(NativeAdapter.SystemStatus status);

    /**
     * Provides callback functionality when the NodeAdministrationService
     * becomes available
     */
    public abstract void onServiceAvailable();
}
