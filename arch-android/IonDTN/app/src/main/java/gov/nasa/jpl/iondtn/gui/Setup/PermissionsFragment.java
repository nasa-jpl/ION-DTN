package gov.nasa.jpl.iondtn.gui.Setup;


import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.Fragment;
import android.support.v4.content.ContextCompat;
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
public class PermissionsFragment extends Fragment {

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
        View v = inflater.inflate(R.layout.fragment_permissions, container,
                false);

        FloatingActionButton next = v.findViewById(R.id.floatingActionButton);

        next.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                    ActivityCompat.requestPermissions(getActivity(),
                            new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                            MainActivity.REQUEST_READ_PERMISSION);
            }
        });

        return v;
    }

    /**
     * Function that handles the return when permission has been granted or
     * dismissed.
     */
    @Override
    public void onResume() {
        super.onResume();
        if (ContextCompat.checkSelfPermission(getActivity(),
                Manifest.permission.READ_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED) {
            getActivity().getSupportFragmentManager()
                    .beginTransaction()
                    .setCustomAnimations(android.R
                            .anim.fade_in, android.R.anim.fade_out)
                    .replace(R.id.fragment_container_startup, new
                            SelectInitFragment()).commit();
        }
    }
}
