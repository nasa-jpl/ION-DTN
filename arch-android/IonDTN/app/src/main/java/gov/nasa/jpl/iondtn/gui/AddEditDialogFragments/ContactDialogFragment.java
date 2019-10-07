package gov.nasa.jpl.iondtn.gui.AddEditDialogFragments;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.TextInputLayout;
import android.support.v7.app.AlertDialog;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.gui.MainFragments.ContactFragment;
import gov.nasa.jpl.iondtn.types.DtnContact;

/**
 * Dialog fragment that allows adding, removing and editing of a contact via
 * a GUI dialog.
 *
 * @author Robert Wiewel
 */
public class ContactDialogFragment extends android.support.v4.app.DialogFragment {
    private DtnContact contact = null;
    private static final String TAG = "ContDiaFragm";

    EditText nodeFrom;
    EditText nodeTo;
    EditText dateFrom;
    EditText dateTo;
    EditText timeFrom;
    EditText timeTo;
    EditText rate;
    EditText confidence;
    TextInputLayout layoutNodeFrom;
    TextInputLayout layoutNodeTo;
    TextInputLayout layoutTimeFrom;
    TextInputLayout layoutTimeTo;
    TextInputLayout layoutDateFrom;
    TextInputLayout layoutDateTo;
    TextInputLayout layoutRate;
    TextInputLayout layoutConfidence;

    private enum ActionType {
        ADD,
        MODIFY
    }
    private ActionType opMode;

    /**
     * Factory method that creates a new instance of the dialog fragment
     * @param contact A particular rule that should be edited/removed
     * @return the fragment object
     */
    public static ContactDialogFragment newInstance(DtnContact contact) {
        ContactDialogFragment f = new ContactDialogFragment();

        // Supply num input as an argument.
        Bundle args = new Bundle();
        args.putParcelable("contact", contact);
        f.setArguments(args);

        return f;
    }

    /**
     * Factory method that creates a new instance of the dialog fragment in
     * order to add ION object
     * @return the fragment object
     */
    public static ContactDialogFragment newInstance() {
        return new ContactDialogFragment();
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
            this.contact = getArguments().getParcelable("contact");
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
        View dialogView = inflater.inflate(R.layout.dialog_add_edit_contact,
                vg);

        layoutNodeFrom = dialogView.findViewById(R.id
                .NodeFromLayout);
        layoutNodeTo = dialogView.findViewById(R.id
                .NodeToLayout);
        layoutTimeFrom = dialogView.findViewById(R.id
                .TimeFromLayout);
        layoutTimeTo = dialogView.findViewById(R.id
                .TimeToLayout);
        layoutDateFrom = dialogView.findViewById(R.id
                .DateFromLayout);
        layoutDateTo = dialogView.findViewById(R.id
                .DateToLayout);
        layoutRate = dialogView.findViewById(R.id
                .DataRateLayout);
        layoutConfidence = dialogView.findViewById(R.id
                .ConfidenceLayout);

        nodeFrom = dialogView.findViewById(R.id.editNodeFrom);
        nodeTo = dialogView.findViewById(R.id.editNodeTo);
        dateFrom = dialogView.findViewById(R.id.editDateFrom);
        dateTo = dialogView.findViewById(R.id.editDateTo);
        timeFrom = dialogView.findViewById(R.id.editTimeFrom);
        timeTo = dialogView.findViewById(R.id.editTimeTo);
        rate = dialogView.findViewById(R.id.editDataRate);
        confidence = dialogView.findViewById(R.id.editConfidence);



        if (this.contact == null) {
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy/MM/dd", Locale.US);
            SimpleDateFormat timeFormat = new SimpleDateFormat("HH:mm:ss", Locale.US);
            dateFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
            timeFormat.setTimeZone(TimeZone.getTimeZone("UTC"));

            dateFrom.setText(dateFormat.format(new Date()));
            dateTo.setText(dateFormat.format(new Date()));

            Date time = new Date();
            timeFrom.setText(timeFormat.format(time));
            Calendar cal = Calendar.getInstance();
            cal.setTime(time);
            cal.add(Calendar.HOUR, 1);
            Date time2 = cal.getTime();
            timeTo.setText(timeFormat.format(time2));
        }
        else {

            dateFrom.setText(contact.getFromTime()
                    .getDateString());
            dateTo.setText(contact.getToTime()
                    .getDateString());
            timeFrom.setText(contact.getFromTime()
                    .getTimeString());
            timeTo.setText(contact.getToTime()
                    .getTimeString());

            nodeFrom.setText(contact.getFromNode());
            nodeTo.setText(contact.getToNode());

            confidence.setText(contact.getConfidence().toString());
            rate.setText(contact.getXmitRate().toString());

            nodeTo.setEnabled(false);
            nodeFrom.setEnabled(false);
            timeFrom.setEnabled(false);
            dateFrom.setEnabled(false);
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
                                    deleteContact();
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
                            if (deleteContact()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });

                    Button b_save = d.getButton(AlertDialog.BUTTON_POSITIVE);
                    b_save.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            if (addEditContact()) {
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
                            if (addEditContact()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });
                }
            });
        }

        return d;
    }

    boolean deleteContact() {
        Log.d(TAG, "deleteContact");
        if (!deleteContactION(contact.getFromTime().toString(),
                contact.getFromNode().replace("ipn:", ""),
                contact.getToNode().replace("ipn:", ""))) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Failed to delete contact!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }
        ContactFragment cf = (ContactFragment)getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        cf.update();
        return true;
    }

    boolean addEditContact() {
        boolean abort = false;
        Log.d(TAG, "edit");

        // Perform consistency checks
        layoutNodeFrom.setError(null);
        layoutNodeTo.setError(null);
        layoutTimeFrom.setError(null);
        layoutTimeTo.setError(null);
        layoutDateFrom.setError(null);
        layoutDateTo.setError(null);
        layoutRate.setError(null);
        layoutConfidence.setError(null);



        // FromNode
        if (!android.text.TextUtils.isDigitsOnly(nodeFrom.getText().toString().replace
                ("ipn:", ""))) {
            layoutNodeFrom.setError("Invalid format!");
            abort = true;
        }
        // ToNode
        if (!android.text.TextUtils.isDigitsOnly(nodeTo.getText().toString()
                .replace
                ("ipn:", ""))) {
            layoutNodeTo.setError("Invalid format!");
            abort = true;
        }

        // Confidence
        int confVal = -1;
        try {
            confVal = Integer.parseInt(confidence.getText().toString());
        } catch (NumberFormatException e) {
            layoutConfidence.setError("Confidence has to be an integer between 0 and 100!");
            abort = true;
        }
        if (confVal > 100 || confVal < 0) {
            layoutConfidence.setError("Confidence has to be an integer between 0 and 100!");
            abort = true;
        }

        // Data Rate
        if (!android.text.TextUtils.isDigitsOnly(rate.getText().toString())) {
            layoutRate.setError("Data rate has to be an integer!");
            abort = true;
        }

        // FromTime
        SimpleDateFormat dateFormat = new SimpleDateFormat
                ("yyyy/MM/dd", Locale.US);
        SimpleDateFormat timeFormat = new SimpleDateFormat
                ("HH:mm:ss", Locale.US);
        SimpleDateFormat format = new SimpleDateFormat("yyyy/MM/dd-HH:mm:ss",
                Locale.US);
        dateFormat.setLenient(false);
        timeFormat.setLenient(false);
        Date from;
        Date to;

        try {
            from = format.parse(dateFrom.getText().toString() + "-" + timeFrom
                    .getText().toString());
            to = format.parse(dateTo.getText().toString() + "-" + timeTo
                    .getText().toString());
        }
        catch (ParseException e) {
            e.printStackTrace();
            return false;
        }

        if (to.before(from)) {
            layoutDateTo.setError("End has to be after the Start");
            layoutTimeTo.setError("");
            abort = true;
        }

        try {
            dateFormat.parse(dateFrom.getText().toString());
        }
        catch (ParseException e) {
            layoutDateFrom.setError("Date format: \'yyyy" +
                    "/MM/dd\"");
            abort = true;
        }

        try {
            dateFormat.parse(dateTo.getText().toString());
        }
        catch (ParseException e) {
            layoutDateTo.setError("Date format: \'yyyy" +
                    "/MM/dd\"");
            abort = true;
        }

        try {
            timeFormat.parse(timeFrom.getText().toString());
        }
        catch (ParseException e) {
            layoutTimeFrom.setError("Time format: \'HH:" +
                    "mm:ss\"");
            abort = true;
        }

        try {
            timeFormat.parse(timeTo.getText().toString());
        }
        catch (ParseException e) {
            layoutTimeTo.setError("Time format: \'HH:" +
                    "mm:ss\"");
            abort = true;
        }

        if (abort) {
            return false;
        }

        if (opMode == ActionType.MODIFY) {
            if (!deleteContactION(contact.getFromTime().toString(),
                    contact.getFromNode().replace("ipn:", ""), contact.getToNode().replace("ipn:", ""))) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to edit contact!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }

        int percentInt = Integer.parseInt(confidence.getText().toString());
        float percentage = (float)percentInt/100.0f;

        if (!addContactION(nodeFrom.getText().toString().replace("ipn:", ""),
                nodeTo.getText().toString().replace("ipn:", ""),
                dateFrom.getText().toString() + "-" + timeFrom
                        .getText().toString(),
                dateTo.getText().toString() + "-" + timeTo
                        .getText().toString(),
                String.valueOf(percentage),
                rate.getText().toString())) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Failed to add contact!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }

        // Update the list
        ContactFragment cf = (ContactFragment)getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        cf.update();
        return true;
    }


    native boolean deleteContactION(String timeFrom, String nodeFrom, String
            nodeTo);

    native boolean addContactION(String nodeFrom, String
            nodeTo, String timeFrom, String timeTo, String confidence, String
            rate);
}