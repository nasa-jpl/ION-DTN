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
import gov.nasa.jpl.iondtn.gui.MainFragments.RangeFragment;
import gov.nasa.jpl.iondtn.types.DtnRange;

/**
 * Dialog fragment that allows adding, removing and editing of a range via
 * a GUI dialog.
 *
 * @author Robert Wiewel
 */
public class RangeDialogFragment extends android.support.v4.app.DialogFragment {
    private DtnRange range = null;
    private static final String TAG = "RangDiaFragm";

    EditText nodeFrom;
    EditText nodeTo;
    EditText dateFrom;
    EditText dateTo;
    EditText timeFrom;
    EditText timeTo;
    EditText owlt;
    TextInputLayout layoutNodeFrom;
    TextInputLayout layoutNodeTo;
    TextInputLayout layoutTimeFrom;
    TextInputLayout layoutTimeTo;
    TextInputLayout layoutDateFrom;
    TextInputLayout layoutDateTo;
    TextInputLayout layoutOwlt;

    private enum ActionType {
        ADD,
        MODIFY
    }
    private ActionType opMode;

    /**
     * Factory method that creates a new instance of the dialog fragment
     * @param range A particular rule that should be edited/removed
     * @return the fragment object
     */
    public static RangeDialogFragment newInstance(DtnRange range) {
        RangeDialogFragment f = new RangeDialogFragment();

        // Supply num input as an argument.
        Bundle args = new Bundle();
        args.putParcelable("range", range);
        f.setArguments(args);

        return f;
    }

    /**
     * Factory method that creates a new instance of the dialog fragment in
     * order to add ION object
     * @return the fragment object
     */
    public static RangeDialogFragment newInstance() {
        return new RangeDialogFragment();
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
            this.range = getArguments().getParcelable("range");
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
        View dialogView = inflater.inflate(R.layout.dialog_add_edit_range,
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
        layoutOwlt = dialogView.findViewById(R.id
                .OwltLayout);

        nodeFrom = dialogView.findViewById(R.id.editNodeFrom);
        nodeTo = dialogView.findViewById(R.id.editNodeTo);
        dateFrom = dialogView.findViewById(R.id.editDateFrom);
        dateTo = dialogView.findViewById(R.id.editDateTo);
        timeFrom = dialogView.findViewById(R.id.editTimeFrom);
        timeTo = dialogView.findViewById(R.id.editTimeTo);
        owlt = dialogView.findViewById(R.id.editOwlt);



        if (this.range == null) {
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

            dateFrom.setText(range.getFromTime()
                    .getDateString());
            dateTo.setText(range.getToTime()
                    .getDateString());
            timeFrom.setText(range.getFromTime()
                    .getTimeString());
            timeTo.setText(range.getToTime()
                    .getTimeString());

            nodeFrom.setText(range.getFromNode());
            nodeTo.setText(range.getToNode());

            owlt.setText(String.format(Locale.US, "%d", range.getOwlt()));

            timeFrom.setEnabled(false);
            dateFrom.setEnabled(false);
            nodeFrom.setEnabled(false);
            nodeTo.setEnabled(false);
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
                                    deleteRange();
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
                            if (deleteRange()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });

                    Button b_save = d.getButton(AlertDialog.BUTTON_POSITIVE);
                    b_save.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            if (addEditRange()) {
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
                            if (addEditRange()) {
                                dialogInterface.dismiss();
                            }
                        }
                    });
                }
            });
        }

        return d;
    }

    boolean deleteRange() {
        Log.d(TAG, "deleteContact");
        if (!deleteRangeION(range.getFromTime().toString(),
                range.getFromNode().replace("ipn:", ""),
                range.getToNode().replace("ipn:", ""))) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Failed to delete contact!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }
        RangeFragment rf = (RangeFragment)getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        rf.update();
        return true;
    }

    boolean addEditRange() {
        boolean abort = false;
        Log.d(TAG, "edit");

        // Perform consistency checks
        layoutNodeFrom.setError(null);
        layoutNodeTo.setError(null);
        layoutTimeFrom.setError(null);
        layoutTimeTo.setError(null);
        layoutDateFrom.setError(null);
        layoutDateTo.setError(null);
        layoutOwlt.setError(null);



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
        int owltVal = -1;
        try {
            owltVal = Integer.parseInt(owlt.getText().toString());
        } catch (NumberFormatException e) {
            layoutOwlt.setError("Owlt has to be a positive integer!");
            abort = true;
        }

        if (owltVal <= 0 ){
            layoutOwlt.setError("Owlt has to be a positive integer!");
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
            if (!deleteRangeION(range.getFromTime().toString(),
                    range.getFromNode().replace("ipn:", ""), range.getToNode()
                            .replace("ipn:", ""))) {
                Toast.makeText(getActivity().getApplicationContext(),
                        "Failed to edit range!",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }

        if (!addRangeION(nodeFrom.getText().toString().replace("ipn:", ""),
                nodeTo.getText().toString().replace("ipn:", ""),
                dateFrom.getText().toString() + "-" + timeFrom
                        .getText().toString(),
                dateTo.getText().toString() + "-" + timeTo
                        .getText().toString(),
                String.valueOf(owlt.getText()))) {
            Toast.makeText(getActivity().getApplicationContext(),
                    "Failed to add range!",
                    Toast.LENGTH_SHORT).show();
            return false;
        }

        // Update the list
        RangeFragment rf = (RangeFragment)getActivity()
                .getSupportFragmentManager()
                .findFragmentById(R.id.fragment_container);
        rf.update();
        return true;
    }


    native boolean deleteRangeION(String timeFrom, String nodeFrom, String
            nodeTo);

    native boolean addRangeION(String nodeFrom, String
            nodeTo, String timeFrom, String timeTo, String owlt);
}