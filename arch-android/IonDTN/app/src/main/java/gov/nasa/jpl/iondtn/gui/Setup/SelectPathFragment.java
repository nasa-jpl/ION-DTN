package gov.nasa.jpl.iondtn.gui.Setup;


import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;


import gov.nasa.jpl.iondtn.R;

/**
 * {@link Fragment} that requests the user to select a path to the custom
 * configuration file.
 *
 * @author Robert Wiewel
 */
public class SelectPathFragment extends Fragment {

    private static final int RESULT_CODE_FILE_SELECT = 142;
    Button selectFileButton;
    EditText pathEditText;
    Uri dataUri = null;

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
        View v = inflater.inflate(R.layout.fragment_select_path, container,
                false);

        selectFileButton = v.findViewById(R.id.selectFileButton);
        pathEditText = v.findViewById(R.id.editTextFilePath);

        selectFileButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                intent.setType("*/*");
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                startActivityForResult(intent, RESULT_CODE_FILE_SELECT);
            }
        });

        FloatingActionButton fb = v.findViewById(R.id.floatingActionButton);

        fb.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (dataUri == null) {
                    Toast.makeText(getContext(), "Please select file!", Toast
                            .LENGTH_LONG)
                            .show();
                }
                else {
                    StartupActivity act = (StartupActivity) getActivity();
                    act.getFileManager().setCustomConfig(dataUri);

                    act.getSupportFragmentManager()
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

    /**
     * Handles the return of the path selection dialogue
     * @param requestCode The code that we have set before to identify the
     *                    request
     * @param resultCode The result code that was assigned to the request
     * @param data The return URI of the selected file (or null on error)
     */
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == RESULT_CODE_FILE_SELECT &&
                resultCode == Activity.RESULT_OK
                && data != null) {

            dataUri = data.getData();
            pathEditText.setText(data.getData().toString());
        }
    }
}

