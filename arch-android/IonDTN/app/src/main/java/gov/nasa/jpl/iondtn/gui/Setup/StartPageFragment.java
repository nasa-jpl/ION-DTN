package gov.nasa.jpl.iondtn.gui.Setup;


import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.app.Fragment;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import gov.nasa.jpl.iondtn.R;

/**
 * {@link Fragment} that requests the filesystem read permission from the
 * user during the setup process.
 *
 * @author Robert Wiewel
 */
public class StartPageFragment extends Fragment {

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
        View v = inflater.inflate(R.layout.fragment_start_page, container,
                false);

        FloatingActionButton next = v.findViewById(R.id.floatingActionButton);
        TextView startMsg = v.findViewById(R.id.textViewStartMessage);

        startMsg.setMovementMethod(LinkMovementMethod.getInstance());

        next.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                getActivity().getSupportFragmentManager()
                        .beginTransaction()
                        .setCustomAnimations(android.R
                                .anim.fade_in, android.R.anim.fade_out)
                        .replace(R.id.fragment_container_startup, new
                                SelectNodeFragment()).commit();
            }
        });

        return v;
    }

}
