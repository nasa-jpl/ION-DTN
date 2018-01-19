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
import gov.nasa.jpl.iondtn.gui.MainFragments.SecurityFragment;
import gov.nasa.jpl.iondtn.types.DtnBibRule;

/**
 * Dialog fragment that allows adding, removing and editing of a bib rule via
 * a GUI dialog.
 *
 * @author Robert Wiewel
 */
public class BibRuleDialogFragment extends android.support.v4.app
        .DialogFragment {
    private DtnBibRule rule = null;

    EditText senderEID;
    EditText destEID;
    EditText ciphersuiteName;
    EditText keyName;
    EditText nodeTypeNumber;

    private enum ActionType {
        ADD,
        MODIFY
    }
    private ActionType opMode;

    /**
     * Factory method that creates a new instance of the dialog fragment
     * @param rule A particular rule that should be edited/removed
     * @return the fragment object
     */
    public static BibRuleDialogFragment newInstance(DtnBibRule rule) {
        BibRuleDialogFragment f = new BibRuleDialogFragment();

        // Supply num input as an argument.
        Bundle args = new Bundle();
        args.putParcelable("Bibrule", rule);
        f.setArguments(args);

        return f;
    }

    /**
     * Factory method that creates a new instance of the dialog fragment in
     * order to add ION object
     * @return the fragment object
     */
    public static BibRuleDialogFragment newInstance() {
        return new BibRuleDialogFragment();
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
            this.rule = getArguments().getParcelable("Bibrule");
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
        View dialogView = inflater.inflate(R.layout.dialog_add_edit_bibrule,
                vg);

        senderEID = dialogView.findViewById(R.id.editSourceEID);
        destEID = dialogView.findViewById(R.id.editDestEID);
        ciphersuiteName = dialogView.findViewById(R.id.editCipherSuite);
        keyName = dialogView.findViewById(R.id.editKeyIdentifier);
        nodeTypeNumber = dialogView.findViewById(R.id.editNodeTypeNumber);

        if (this.rule != null) {
            senderEID.setText(rule.getSenderEID());
            destEID.setText(rule.getReceiverEID());
            ciphersuiteName.setText(rule.getCiphersuiteName());
            keyName.setText(rule.getKeyName());
            nodeTypeNumber.setText(rule.getBlockTypeNumber());

            senderEID.setEnabled(false);
            destEID.setEnabled(false);
            ciphersuiteName.setEnabled(false);
            keyName.setEnabled(false);
            nodeTypeNumber.setEnabled(false);
        }

        if (opMode == ActionType.MODIFY) {
            // Inflate and set the layout for the dialog
            // Pass null as the parent view because its going in the dialog layout
            builder.setView(dialogView)
                    // Add action buttons
                    .setPositiveButton(R.string.dialogButtonCancel, new
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
                    .setNeutralButton(R.string.dialogButtonDelete, new
                            DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int
                                        id) {
                                    deleteBibRule();
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
                            if (deleteBibRule()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });

                    Button b_save = d.getButton(AlertDialog.BUTTON_POSITIVE);
                    b_save.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            dialogInterface.dismiss();
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
                            if (addEditBibRule()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });
                }
            });
        }

        return d;
    }

    boolean deleteBibRule() {
        if (!deleteBibRuleION(rule.getSenderEID(), rule.getReceiverEID(),
                Integer.valueOf(rule.getBlockTypeNumber()))) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Failed to delete Bib rule! Switch to log for details!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }
        // Update the list
        SecurityFragment sec = (SecurityFragment) getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        sec.getBibRuleFragment().update();
        return true;
    }

    boolean addEditBibRule() {

        // no consistency check possible

        if (opMode == ActionType.MODIFY) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Operation not allowed!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }
        else {
            if (!addBibRuleION(senderEID.getText().toString(),
                    destEID.getText().toString(),
                    ciphersuiteName.getText().toString(),
                    keyName.getText().toString(),
                    Integer.valueOf(nodeTypeNumber.getText().toString()))) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to add Bib rule! Switch to log for details!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }

        // Update the list
        SecurityFragment sec = (SecurityFragment) getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        sec.getBibRuleFragment().update();
        return true;
    }

    native boolean deleteBibRuleION(String srcEID,
                                    String destEID,
                                    int nodeTypeNumber);

    native boolean addBibRuleION(String srcEID,
                                 String destEID,
                                 String ciphersuiteName,
                                 String keyName,
                                 int nodeTypeNumber);
}