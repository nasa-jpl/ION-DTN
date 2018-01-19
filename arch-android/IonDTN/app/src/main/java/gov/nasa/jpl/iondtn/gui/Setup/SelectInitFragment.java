package gov.nasa.jpl.iondtn.gui.Setup;


import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioButton;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.ConfigFileManager;

/**
 * {@link Fragment} that lets the user choose one type of initialization,
 * either empty or custom
 *
 * @author Robert Wiewel
 */
public class SelectInitFragment extends Fragment {

    FloatingActionButton nextButton;
    RadioButton customInitButton;
    RadioButton defaultInitButton;

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
        View v = inflater.inflate(R.layout.fragment_select_init, container,
                false);

        nextButton = v.findViewById(R.id.floatingActionButton);
        customInitButton = v.findViewById(R.id.radioButtonCustom);
        defaultInitButton = v.findViewById(R.id.radioButtonStandard);

        nextButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                StartupActivity act = (StartupActivity)getActivity();
                if (customInitButton.isChecked()) {
                        getActivity().getSupportFragmentManager()
                                .beginTransaction()
                                .setCustomAnimations(android.R
                                        .anim.fade_in, android.R.anim
                                        .fade_out)
                                .replace(R.id.fragment_container_startup,
                                        new SelectPathFragment()).commit();
                }
                else if (defaultInitButton.isChecked()) {
                    act.getFileManager().setType(ConfigFileManager
                            .ConfigFileType.EMPTY);
                    getActivity().getSupportFragmentManager()
                            .beginTransaction()
                            .setCustomAnimations(android.R
                                    .anim.fade_in, android.R.anim.fade_out)
                            .replace(R.id.fragment_container_startup, new
                                    PerformInitFragment()).commit();
                }
            }
        });

        return v;
    }

}
