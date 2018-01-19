package gov.nasa.jpl.iondtn.gui.Setup;


import android.Manifest;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.TextInputLayout;
import android.support.v4.app.Fragment;
import android.support.v4.content.ContextCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.ConfigFileManager;

/**
 * {@link Fragment} that lets the user set a node number for the specific ION
 * node
 *
 * @author Robert Wiewel
 */
public class SelectNodeFragment extends Fragment {

    EditText editNodeNbr;
    TextInputLayout layoutNodeNbr;

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
        View v = inflater.inflate(R.layout.fragment_select_node, container,
                false);

        editNodeNbr = v.findViewById(R.id.editNodeNbr);
        layoutNodeNbr = v.findViewById(R.id.NodeNbrLayout);

        FloatingActionButton fb = v.findViewById(R.id.floatingActionButton);

        fb.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                long nodeNbr;

                try {
                    nodeNbr = Long.parseLong(editNodeNbr.getText().toString());
                }
                catch (NumberFormatException e) {
                    layoutNodeNbr.setError("Node Number has to be integer!");
                    return;
                }
                StartupActivity act = (StartupActivity)getActivity();
                act.getFileManager().setNodeNbr(nodeNbr);

                SharedPreferences sharedPref = getActivity().getApplicationContext()
                        .getSharedPreferences(getString(R.string.preference_file_key)
                                , Context.MODE_PRIVATE);

                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putLong("node number", nodeNbr);
                editor.apply();

                act.getFileManager().setType(ConfigFileManager
                        .ConfigFileType.CUSTOM);
                if (ContextCompat.checkSelfPermission(getActivity(),
                        Manifest.permission.READ_EXTERNAL_STORAGE)
                        != PackageManager.PERMISSION_GRANTED) {
                    getActivity().getSupportFragmentManager()
                            .beginTransaction()
                            .setCustomAnimations(android.R
                                    .anim.fade_in, android.R.anim.fade_out)
                            .replace(R.id.fragment_container_startup, new
                                    PermissionsFragment()).commit();
                }
                else {
                    getActivity().getSupportFragmentManager()
                            .beginTransaction()
                            .setCustomAnimations(android.R
                                    .anim.fade_in, android.R.anim
                                    .fade_out)
                            .replace(R.id.fragment_container_startup,
                                    new SelectInitFragment()).commit();
                }
            }
        });

        return v;
    }

}
