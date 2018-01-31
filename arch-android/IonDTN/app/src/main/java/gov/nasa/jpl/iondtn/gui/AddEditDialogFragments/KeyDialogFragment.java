package gov.nasa.jpl.iondtn.gui.AddEditDialogFragments;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.TextInputLayout;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import java.io.File;
import java.net.URI;
import java.net.URISyntaxException;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.gui.MainFragments.SecurityFragment;
import gov.nasa.jpl.iondtn.types.DtnKey;

/**
 * Dialog fragment that allows adding, removing and editing of a key via
 * a GUI dialog.
 *
 * @author Robert Wiewel
 */
public class KeyDialogFragment extends android.support.v4.app
        .DialogFragment {
    private DtnKey key = null;
    private static final int RESULT_CODE_FILE_SELECT = 143;

    EditText name;
    EditText pathLength;
    Button buttonSelect;

    private enum ActionType {
        ADD,
        MODIFY
    }
    private ActionType opMode;

    /**
     * Factory method that creates a new instance of the dialog fragment
     * @param key A particular rule that should be edited/removed
     * @return the fragment object
     */
    public static KeyDialogFragment newInstance(DtnKey key) {
        KeyDialogFragment f = new KeyDialogFragment();

        // Supply num input as an argument.
        Bundle args = new Bundle();
        args.putParcelable("key", key);
        f.setArguments(args);

        return f;
    }

    /**
     * Factory method that creates a new instance of the dialog fragment in
     * order to add ION object
     * @return the fragment object
     */
    public static KeyDialogFragment newInstance() {
        return new KeyDialogFragment();
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
            this.key = getArguments().getParcelable("key");
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
        View dialogView = inflater.inflate(R.layout.dialog_add_edit_key,
                vg);

        name = dialogView.findViewById(R.id.editKeyName);
        pathLength = dialogView.findViewById(R.id.editKeyPathLength);
        buttonSelect = dialogView.findViewById(R.id.buttonSelect);

        TextInputLayout l = dialogView.findViewById(R.id.layoutFwdCmd);

        if (this.key != null) {
            name.setText(key.getKeyName());
            pathLength.setText(key.getKeyLength());
            l.setHint("Length");

            name.setEnabled(false);
            pathLength.setEnabled(false);
            buttonSelect.setVisibility(View.INVISIBLE);
        }
        else {
            l.setHint("Path");
            buttonSelect.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                    intent.setType("file/*");
                    startActivityForResult(intent, RESULT_CODE_FILE_SELECT);
                }
            });
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
                                    deleteKey();
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
                            if (deleteKey()) {
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
                            if (addKey()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });
                }
            });
        }

        return d;
    }

    boolean deleteKey() {
        if (!deleteKeyION(name.getText().toString())) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Failed to delete key! Switch to log for details!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }
        // Update the list
        SecurityFragment sec = (SecurityFragment) getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        sec.getKeyFragment().update();
        return true;
    }

    boolean addKey() {

        // no consistency check possible

        if (opMode == ActionType.MODIFY) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Operation not allowed!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }
        else {
            if (!addKeyION(name.getText().toString(),
                    pathLength.getText().toString().replace("file://", ""))) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to add key! Switch to log for details!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }

        // Update the list
        SecurityFragment sec = (SecurityFragment) getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        sec.getKeyFragment().update();
        return true;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        URI uri;

        if (requestCode == RESULT_CODE_FILE_SELECT &&
                resultCode == Activity.RESULT_OK
                && data != null) {

            try {
                uri = new URI(data.getData().toString());
                if (!uri.getScheme().equals("file")) {
                    Toast.makeText(getContext(), "Invalid file path!", Toast
                            .LENGTH_LONG).show();
                    return;
                }
                File f = new File(uri);
                pathLength.setText(f.getAbsolutePath());
            }
            catch (URISyntaxException e) {
                Toast.makeText(getContext(), "Parsing URI failed! Try again!",
                        Toast.LENGTH_LONG).show();
            }
        }
    }

    native boolean deleteKeyION(String name);

    native boolean addKeyION(String name,
                                 String path);
}