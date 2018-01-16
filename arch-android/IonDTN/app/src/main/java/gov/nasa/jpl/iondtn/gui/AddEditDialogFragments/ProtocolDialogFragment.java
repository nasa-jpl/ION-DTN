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
import android.widget.Toast;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.gui.MainFragments.ProtocolFragment;
import gov.nasa.jpl.iondtn.types.DtnProtocol;

/**
 * Dialog fragment that allows adding, removing and editing of a protocol via
 * a GUI dialog.
 *
 * @author Robert Wiewel
 */
public class ProtocolDialogFragment extends android.support.v4.app
        .DialogFragment {
    private DtnProtocol protocol = null;

    EditText editIdentifier;
    EditText editPayload;
    EditText editOverhead;
    EditText editProtocolClass;

    private enum ActionType {
        ADD,
        MODIFY
    }
    private ActionType opMode;

    /**
     * Factory method that creates a new instance of the dialog fragment
     * @param protocol A particular rule that should be edited/removed
     * @return the fragment object
     */
    public static ProtocolDialogFragment newInstance(DtnProtocol protocol) {
        ProtocolDialogFragment f = new ProtocolDialogFragment();

        // Supply num input as an argument.
        Bundle args = new Bundle();
        args.putParcelable("protocol", protocol);
        f.setArguments(args);

        return f;
    }

    /**
     * Factory method that creates a new instance of the dialog fragment in
     * order to add ION object
     * @return the fragment object
     */
    public static ProtocolDialogFragment newInstance() {
        return new ProtocolDialogFragment();
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
            this.protocol = getArguments().getParcelable("protocol");
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
        View dialogView = inflater.inflate(R.layout.dialog_add_edit_protocol,
                vg);

        editIdentifier = dialogView.findViewById(R.id.editIdentifier);
        editOverhead = dialogView.findViewById(R.id.editOverhead);
        editPayload = dialogView.findViewById(R.id.editPayload);
        editProtocolClass = dialogView.findViewById(R.id.editProtocolClass);

        if (this.protocol != null) {

            editIdentifier.setText(protocol.getIdentifier());
            editPayload.setText(String.valueOf(protocol.getPayloadFrameSize()));
            editOverhead.setText(String.valueOf(protocol.getOverheadFrameSize()));
            editProtocolClass.setText(String.valueOf(protocol.getProtocolClass()));

            editIdentifier.setEnabled(false);
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
                                    deleteProtocol();
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
                            if (deleteProtocol()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });

                    Button b_save = d.getButton(AlertDialog.BUTTON_POSITIVE);
                    b_save.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            if (addEditProtocol()) {
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
                            if (addEditProtocol()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });
                }
            });
        }

        return d;
    }

    boolean deleteProtocol() {
        if (!deleteProtocolION(protocol.getIdentifier())) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Failed to delete scheme! Switch to log for details!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }
        // Update the list
        ProtocolFragment pf = (ProtocolFragment)getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        pf.update();
        return true;
    }

    boolean addEditProtocol() {

        // no consistency check possible
        if (editProtocolClass.getText().toString().isEmpty()) {
            editProtocolClass.setText(editOverhead.getText());
        }

        if (opMode == ActionType.MODIFY) {
            if (!updateProtocolION(editIdentifier.getText().toString(),
                    Integer.parseInt(editPayload.getText().toString()),
                    Integer.parseInt(editOverhead.getText().toString()),
                    Integer.parseInt(editProtocolClass.getText().toString()))) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to add scheme! Switch to log for details!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }
        else {
            if (!addProtocolION(editIdentifier.getText().toString(),
                    Integer.parseInt(editPayload.getText().toString()),
                    Integer.parseInt(editOverhead.getText().toString()),
                    Integer.parseInt(editProtocolClass.getText().toString()))) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to add scheme! Switch to log for details!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }

        // Update the list
        ProtocolFragment pf = (ProtocolFragment)getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        pf.update();
        return true;
    }


    native boolean deleteProtocolION(String identifier);

    native boolean addProtocolION(String identifier, int payload, int overhead,
                                int protocolClass);

    native boolean updateProtocolION(String identifier, int payload, int overhead,
                                   int protocolClass);
}