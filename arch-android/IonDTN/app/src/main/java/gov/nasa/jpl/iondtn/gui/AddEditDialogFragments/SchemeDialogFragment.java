package gov.nasa.jpl.iondtn.gui.AddEditDialogFragments;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Switch;
import android.widget.Toast;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.gui.MainFragments.SchemeFragment;
import gov.nasa.jpl.iondtn.types.DtnEidScheme;

/**
 * Dialog fragment that allows adding, removing and editing of a scheme via
 * a GUI dialog.
 *
 * @author Robert Wiewel
 */
public class SchemeDialogFragment extends android.support.v4.app.DialogFragment {
    private DtnEidScheme scheme = null;

    EditText editScheme;
    EditText editFwdCmd;
    EditText editAdmAppCmd;
    Switch switchStatus;

    private enum ActionType {
        ADD,
        MODIFY
    }
    private ActionType opMode;

    /**
     * Factory method that creates a new instance of the dialog fragment
     * @param scheme A particular rule that should be edited/removed
     * @return the fragment object
     */
    public static SchemeDialogFragment newInstance(DtnEidScheme scheme) {
        SchemeDialogFragment f = new SchemeDialogFragment();

        // Supply num input as an argument.
        Bundle args = new Bundle();
        args.putParcelable("scheme", scheme);
        f.setArguments(args);

        return f;
    }

    /**
     * Factory method that creates a new instance of the dialog fragment in
     * order to add ION object
     * @return the fragment object
     */
    public static SchemeDialogFragment newInstance() {
        return new SchemeDialogFragment();
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
        if (getArguments() != null) {
            this.scheme = getArguments().getParcelable("scheme");
            opMode = ActionType.MODIFY;
        }
        else {
            opMode = ActionType.ADD;
        }
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
        View dialogView = inflater.inflate(R.layout.dialog_add_edit_scheme,
                vg);

        editScheme = dialogView.findViewById(R.id.editScheme);
        editFwdCmd = dialogView.findViewById(R.id.editFwdCmd);
        editAdmAppCmd = dialogView.findViewById(R.id.editAdmAppCmd);
        switchStatus = dialogView.findViewById(R.id.switchStatus);

        switchStatus.setEnabled(false);

        if (this.scheme != null) {

            editScheme.setText(scheme.getSchemeID());
            editFwdCmd.setText(scheme.getForwarderCommand());
            editAdmAppCmd.setText(scheme.getAdminAppCommand());
            switchStatus.setChecked(scheme.getStatus());

            editScheme.setEnabled(false);
            switchStatus.setEnabled(true);
        }

        switchStatus.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {


            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                if (b) {
                    if (!startSchemeION(scheme.getSchemeID())) {
                        Toast.makeText(getContext(), "Failed to start " +
                                "scheme!", Toast.LENGTH_LONG).show();
                        compoundButton.setChecked(false);
                    }
                }
                else {
                    stopSchemeION(scheme.getSchemeID());
                }
            }
        });

        if (opMode == ActionType.MODIFY) {
            // Inflate and set the layout for the dialog
            // Pass null as the parent view because its going in the dialog layout
            builder.setView(dialogView)
                    // Add action buttons
                    .setPositiveButton(R.string.dialogButtonSave, new
                            DialogInterface
                                    .OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int id) {
                                    Toast.makeText(getActivity()
                                            .getApplicationContext(), "Save " +
                                            "Action", Toast
                                            .LENGTH_LONG).show();
                                }
                            })
                    .setNegativeButton(R.string.dialogButtonCancel, new
                            DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int
                                        id) {
                                    dialog.dismiss();
                                }
                            })
                    .setNeutralButton(R.string.dialogButtonDelete, new
                            DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int
                                        id) {
                                    deleteScheme();
                                }
                            });
        }
        else {
            // Inflate and set the layout for the dialog
            // Pass null as the parent view because its going in the dialog layout
            builder.setView(dialogView)
                    // Add action buttons
                    .setPositiveButton(R.string.dialogButtonAdd, new
                            DialogInterface
                                    .OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int id) {
                                    Toast.makeText(getActivity()
                                            .getApplicationContext(), "Save " +
                                            "Action", Toast
                                            .LENGTH_LONG).show();
                                }
                            })
                    .setNegativeButton(R.string.dialogButtonCancel, new
                            DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int
                                        id) {
                                    dialog.dismiss();
                                }
                            }).setCancelable(false);
        }

        final AlertDialog d = builder.create();

        if (opMode == ActionType.MODIFY) {
            d.setOnShowListener(new DialogInterface.OnShowListener() {
                @Override
                public void onShow(final DialogInterface dialogInterface) {
                    Button b_delete = d.getButton(AlertDialog.BUTTON_NEUTRAL);
                    b_delete.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            if (deleteScheme()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });

                    Button b_save = d.getButton(AlertDialog.BUTTON_POSITIVE);
                    b_save.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            if (addEditScheme()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });
                }
            });
        }
        else {
            d.setOnShowListener(new DialogInterface.OnShowListener() {
                @Override
                public void onShow(final DialogInterface dialogInterface) {

                    Button b_save = d.getButton(AlertDialog.BUTTON_POSITIVE);
                    b_save.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            if (addEditScheme()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });
                }
            });
        }

        return d;
    }

   boolean deleteScheme() {
        if (!deleteSchemeION(scheme.getSchemeID())) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Failed to delete scheme! Switch to log for details!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }
       // Update the list
       SchemeFragment sf = (SchemeFragment)getActivity()
               .getSupportFragmentManager()
               .findFragmentById(R.id.fragment_container);
       sf.update();
        return true;
    }

    boolean addEditScheme() {

        // no consistency check possible

        if (opMode == ActionType.MODIFY) {
            if (!updateSchemeION(editScheme.getText().toString(), editFwdCmd.getText
                    ().toString(), editAdmAppCmd.getText().toString())) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to add scheme! Switch to log for details!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }
        else {
            if (!addSchemeION(editScheme.getText().toString(), editFwdCmd.getText
                    ().toString(), editAdmAppCmd.getText().toString())) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to add scheme! Switch to log for details!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }

        // Update the list
        SchemeFragment sf = (SchemeFragment)getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        sf.update();
        return true;
    }


    native boolean deleteSchemeION(String scheme);

    native boolean addSchemeION(String name, String
            fwdCmd, String admAppCmd);

    native boolean updateSchemeION(String name, String
            fwdCmd, String admAppCmd);

    native boolean startSchemeION(String identifier);
    native void stopSchemeION(String identifier);
}