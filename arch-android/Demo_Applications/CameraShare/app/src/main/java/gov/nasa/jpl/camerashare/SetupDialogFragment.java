package gov.nasa.jpl.camerashare;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.RadioButton;

/**
 * Dialog fragment that allows adding, removing and editing of an induct via
 * a GUI dialog.
 *
 * @author Robert Wiewel
 */
public class SetupDialogFragment extends android.support.v4.app
        .DialogFragment {
    EditText editOwnShareEID;
    EditText editRemoteShareEID;
    RadioButton radioOpenOwnShare;
    RadioButton radioOpenRemoteShare;

    /**
     * Factory method that creates a new instance of the dialog fragment in
     * order to add ION object
     * @return the fragment object
     */
    public static SetupDialogFragment newInstance() {
        return new SetupDialogFragment();
    }

    /**
     * Determines the type of operation and (if applicable) extracts object
     * data from bundle
     * @param savedInstanceState A bundle containing a saved instance of this
     *                           fragment if available
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    /**
     * Populates the dialog fragment and registers button listeners
     * @param savedInstanceState A bundle containing a saved instance of this
     *                           fragment if available
     */
    @NonNull
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        // Get the layout inflater
        LayoutInflater inflater = getActivity().getLayoutInflater();

        final ViewGroup vg = null;
        View dialogView = inflater.inflate(R.layout.dialog_start,
                vg);

        editOwnShareEID = dialogView.findViewById(R.id.editsink);
        editRemoteShareEID = dialogView.findViewById(R.id.editjoineid);
        radioOpenOwnShare = dialogView.findViewById(R.id.radioButton2);
        radioOpenRemoteShare = dialogView.findViewById(R.id.radioButton);

        SharedPreferences sharedPref = getActivity().getApplicationContext()
                .getSharedPreferences(getString(R.string.preference_file_key)
                        , Context.MODE_PRIVATE);

        if (sharedPref.contains("own_sink_eid")) {
            editOwnShareEID.setText(sharedPref.getString("own_sink_eid",
                    "ipn:1.1"));
        }
        if (sharedPref.contains("last_remote_eid")) {
            editRemoteShareEID.setText(sharedPref.getString("last_remote_eid",
                    "ipn:1.1"));
        }

        editRemoteShareEID.setEnabled(false);
        radioOpenOwnShare.setChecked(true);

        radioOpenOwnShare.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                if (b) {
                    editRemoteShareEID.setEnabled(false);
                }
                else {
                    editRemoteShareEID.setEnabled(true);
                }
            }
        });

        // Inflate and set the layout for the dialog
        // Pass null as the parent view because its going in the dialog layout
        builder.setView(dialogView);

        final AlertDialog d = builder.create();

        d.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(final DialogInterface dialogInterface) {
                Button b_save = d.getButton(AlertDialog.BUTTON_POSITIVE);
                b_save.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        if (updateConfig()) {
                            dialogInterface.dismiss();
                        }
                    }
                });
            }
        });

        return d;
    }

    boolean updateConfig() {
        SharedPreferences sharedPref = getActivity().getApplicationContext()
                .getSharedPreferences(getString(R.string.preference_file_key)
                        , Context.MODE_PRIVATE);

        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString("own_sink_eid", editOwnShareEID.getText().toString());
        editor.putString("editRemoteShareEID", editRemoteShareEID.getText().toString());
        editor.apply();
        return true;
    }
}