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
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.Toast;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.gui.MainFragments.EndpointFragment;
import gov.nasa.jpl.iondtn.types.DtnEndpointIdentifier;

/**
 * Dialog fragment that allows adding, removing and editing of an EID via
 * a GUI dialog.
 *
 * @author Robert Wiewel
 */
public class EidDialogFragment extends android.support.v4.app.DialogFragment {
    private DtnEndpointIdentifier eid = null;

    EditText editEID;
    RadioButton discardButton;
    RadioButton queueButton;


    private enum ActionType {
        ADD,
        MODIFY
    }
    private ActionType opMode;

    /**
     * Factory method that creates a new instance of the dialog fragment
     * @param eid A particular rule that should be edited/removed
     * @return the fragment object
     */
    public static EidDialogFragment newInstance(DtnEndpointIdentifier eid) {
        EidDialogFragment f = new EidDialogFragment();

        // Supply num input as an argument.
        Bundle args = new Bundle();
        args.putParcelable("eid", eid);
        f.setArguments(args);

        return f;
    }

    /**
     * Factory method that creates a new instance of the dialog fragment in
     * order to add ION object
     * @return the fragment object
     */
    public static EidDialogFragment newInstance() {
        return new EidDialogFragment();
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
            this.eid = getArguments().getParcelable("eid");
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
        View dialogView = inflater.inflate(R.layout.dialog_add_edit_eid,
                vg);

        editEID = dialogView.findViewById(R.id.editEID);
        discardButton = dialogView.findViewById(R.id.radioButtonDiscard);
        queueButton = dialogView.findViewById(R.id.radioButtonQueue);


        if (this.eid != null) {

            editEID.setText(eid.getIdentifier());
            discardButton.setChecked((eid.getBehavior() ==
                    DtnEndpointIdentifier.ReceivingBehavior.DISCARD));
            queueButton.setChecked((eid.getBehavior() ==
                    DtnEndpointIdentifier.ReceivingBehavior.QUEUE));

            editEID.setEnabled(false);
        }

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
                                    //deleteRange();
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
                            if (deleteEid()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });

                    Button b_save = d.getButton(AlertDialog.BUTTON_POSITIVE);
                    b_save.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            if (addEditEid()) {
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
                            if (addEditEid()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });
                }
            });
        }

        return d;
    }

    boolean deleteEid() {
        if (!deleteEidION(eid.getIdentifier())) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Failed to delete EID! Switch to log for details!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }
        // Update the list
        EndpointFragment sf = (EndpointFragment) getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        sf.update();
        return true;
    }

    boolean addEditEid() {

        // no consistency check possible

        if (opMode == ActionType.MODIFY) {
            if (!updateEidION(editEID.getText().toString(),
                    discardButton.isChecked())) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to add scheme! Switch to log for details!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }
        else {
            if (!addEidION(editEID.getText().toString(),
                    discardButton.isChecked())) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to add scheme! Switch to log for details!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }

        // Update the list
        EndpointFragment sf = (EndpointFragment) getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        sf.update();
        return true;
    }


    native boolean deleteEidION(String eid);

    native boolean addEidION(String eid, boolean behavior);

    native boolean updateEidION(String eid, boolean behavior);
}