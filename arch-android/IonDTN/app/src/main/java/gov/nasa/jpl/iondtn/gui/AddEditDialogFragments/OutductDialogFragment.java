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
import gov.nasa.jpl.iondtn.gui.MainFragments.InOutductFragment;
import gov.nasa.jpl.iondtn.types.DtnInOutduct;

/**
 * Dialog fragment that allows adding, removing and editing of an outduct via
 * a GUI dialog.
 *
 * @author Robert Wiewel
 */
public class OutductDialogFragment extends android.support.v4.app
        .DialogFragment {
    private DtnInOutduct outduct = null;

    EditText editIdentifier;
    EditText editProtocol;
    EditText editCommand;
    EditText editMaxPayloadLength;
    Switch switchStatus;


    private enum ActionType {
        ADD,
        MODIFY
    }
    private ActionType opMode;

    /**
     * Factory method that creates a new instance of the dialog fragment
     * @param outduct A particular rule that should be edited/removed
     * @return the fragment object
     */
    public static OutductDialogFragment newInstance(DtnInOutduct outduct) {
        OutductDialogFragment f = new OutductDialogFragment();

        // Supply num input as an argument.
        Bundle args = new Bundle();
        args.putParcelable("inoutduct", outduct);
        f.setArguments(args);

        return f;
    }

    /**
     * Factory method that creates a new instance of the dialog fragment in
     * order to add ION object
     * @return the fragment object
     */
    public static OutductDialogFragment newInstance() {
        return new OutductDialogFragment();
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
            this.outduct = getArguments().getParcelable("inoutduct");
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
        View dialogView = inflater.inflate(R.layout.dialog_add_edit_outduct,
                vg);

        editIdentifier = dialogView.findViewById(R.id.editIdentifier);
        editProtocol = dialogView.findViewById(R.id.editProtocol);
        editCommand = dialogView.findViewById(R.id.editCmd);
        editMaxPayloadLength = dialogView.findViewById(R.id
                .editMaxPayloadLength);
        switchStatus = dialogView.findViewById(R.id.switchStatus);

        switchStatus.setEnabled(false);

        if (this.outduct != null) {

            editIdentifier.setText(outduct.getDuctName());
            editProtocol.setText(String.valueOf(outduct.getProtocolName()));
            editCommand.setText(String.valueOf(outduct.getCmd()));
            editMaxPayloadLength.setText(String.valueOf(outduct.getMaxPayloadLength()));
            switchStatus.setChecked(outduct.getStatus());

            editIdentifier.setEnabled(false);
            editProtocol.setEnabled(false);
            switchStatus.setEnabled(true);
        }

        switchStatus.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {


            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                if (b) {
                    if (!startOutductION(outduct.getProtocolName(), outduct
                            .getDuctName())) {
                        Toast.makeText(getContext(), "Failed to start " +
                                "outduct!", Toast.LENGTH_LONG).show();
                        compoundButton.setChecked(false);
                    }
                }
                else {
                    stopOutductION(outduct.getProtocolName(), outduct
                            .getDuctName());
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
                                    deleteOutduct();
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
                            if (deleteOutduct()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });

                    Button b_save = d.getButton(AlertDialog.BUTTON_POSITIVE);
                    b_save.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            if (addEditOutduct()) {
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
                            if (addEditOutduct()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });
                }
            });
        }

        return d;
    }

    boolean deleteOutduct() {
        if (!deleteOutductION(outduct.getDuctName(), outduct.getProtocolName()
        )) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Failed to delete induct! Switch to log for details!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }
        // Update the list
        InOutductFragment inf = (InOutductFragment)getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        inf.getOutductFragment().update();
        return true;
    }

    boolean addEditOutduct() {

        // no consistency check possible
        String cmdOutput = editCommand.getText().toString();

        if (editCommand.getText().toString().equals("-")) {
            cmdOutput = "";
        }

        if (opMode == ActionType.MODIFY) {
            if (!updateOutductION(editIdentifier.getText().toString(),
                    editProtocol.getText().toString(),
                    cmdOutput,
                    Integer.valueOf(editMaxPayloadLength.getText().toString()
                    ))) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to add induct! Switch to log for details!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }
        else {
            if (!addOutductION(editIdentifier.getText().toString(),
                    editProtocol.getText().toString(),
                    editCommand.getText().toString(),
                    Integer.valueOf(editMaxPayloadLength.getText().toString()
                    ))) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to add scheme! Switch to log for details!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }

        // Update the list
        InOutductFragment inf = (InOutductFragment)getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        inf.getOutductFragment().update();
        return true;
    }

    native boolean deleteOutductION(String identifier, String protocol);

    native boolean startOutductION(String identifier, String protocol);
    native void stopOutductION(String identifier, String protocol);


    native boolean addOutductION(String identifier,
                                String protocol,
                                String cmd,
                                 int maxPayloadLength);

    native boolean updateOutductION(String identifier,
                                   String protocol,
                                   String cmd,
                                    int maxPayloadLength);
}