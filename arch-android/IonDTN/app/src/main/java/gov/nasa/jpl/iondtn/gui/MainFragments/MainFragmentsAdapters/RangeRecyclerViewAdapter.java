package gov.nasa.jpl.iondtn.gui.MainFragments.MainFragmentsAdapters;

import android.support.v4.app.FragmentManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.text.ParseException;
import java.util.ArrayList;
import java.util.Locale;
import java.util.Scanner;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.gui.AddEditDialogFragments.RangeDialogFragment;
import gov.nasa.jpl.iondtn.types.DtnRange;
import gov.nasa.jpl.iondtn.types.DtnTime;

/**
 * This Recycler adapter generates views for
 * {@link gov.nasa.jpl.iondtn.gui.MainFragments.RangeFragment}.
 *
 * @author Robert Wiewel
 */
public class RangeRecyclerViewAdapter extends RecyclerView.Adapter<RangeRecyclerViewAdapter.CustomViewHolder>{
    private ArrayList<DtnRange> dataset;
    private static final String TAG = "RangeAdapter";
    private FragmentManager fragmentMgr;

    /**
     * Constructor used for initializing the view generation process
     */
    public static class CustomViewHolder extends RecyclerView.ViewHolder{
        public View my_view;
        public ViewGroup parent;
        public CustomViewHolder(View v, ViewGroup parent) {
            super(v);
            my_view = v;
            this.parent = parent;
        }
    }

    /**
     * Constructor used for initializing the view generation process
     * @param input The formatted string from ION that contains all relevant
     *              information for populating the views
     * @param fr The FragmentManager that the views have to be added to
     */
    public RangeRecyclerViewAdapter(String input, FragmentManager fr) {
        Log.d(TAG, "RangeRecyclerViewAdapter: got text: " + input);
        dataset = new ArrayList<>();
        parseContent(input);
        this.fragmentMgr = fr;
    }

    private void parseContent(String input) {
        Scanner linescanner = new Scanner(input);

        while(linescanner.hasNextLine()) {
            String delims = "[ ]+";
            String[] tokens = linescanner.nextLine().split(delims);

            try {
                DtnRange r = new DtnRange(tokens[8],
                        tokens[11],
                        new DtnTime(tokens[1]),
                        new DtnTime(tokens[3]),
                        Integer.parseInt(tokens[13]));
                dataset.add(r);
                Log.d(TAG, "new DtnRange object: " + r.toString());
            } catch (ParseException e) {
                Log.e(TAG, "parseContent: Couldn\'t parse line. Ignore.");
            }
        }
    }

    /**
     * Creates new view objects
     * @param parent The parent view group of the new item view object
     * @param viewType Not used but required
     * @return A new view object
     */
    @Override
    public RangeRecyclerViewAdapter.CustomViewHolder onCreateViewHolder(final ViewGroup parent,
                                                                          int viewType) {
        // create a new view
        View v = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.recycler_range_view_item_layout, parent,
                        false);
        // set the view's size, margins, paddings and layout parameters
        return new CustomViewHolder(v, parent);
    }

    // Replace the contents of a view (invoked by the layout manager)
    @Override
    public void onBindViewHolder(CustomViewHolder holder, int position) {
        // - get element from your dataset at this position
        // - replace the contents of the view with that element

        final int pos = position;

        holder.itemView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                android.support.v4.app.DialogFragment newFragment =
                        RangeDialogFragment.newInstance(dataset
                        .get(pos));
                newFragment.show(fragmentMgr, "rf");
            }
        });

        TextView fromNode = holder.itemView.findViewById(R.id
                .textViewNodeFrom);
        TextView toNode = holder.itemView.findViewById(R.id
                .textViewNodeTo);
        TextView fromTime = holder.itemView.findViewById(R.id
                .textViewTimeFrom);
        TextView toTime = holder.itemView.findViewById(R.id
                .textViewTimeTo);
        TextView owlt = holder.itemView.findViewById(R.id
                .textViewOwlt);

        fromNode.setText(dataset.get(position).getFromNode());
        toNode.setText(dataset.get(position).getToNode());
        fromTime.setText(dataset.get(position).getFromTime().toString());
        toTime.setText(dataset.get(position).getToTime().toString());
        owlt.setText(String.format(Locale.US, "%d", dataset.get(position)
                .getOwlt()));
    }

    /**
     * Get the number of items in the dataset
     * @return The number of items as integer
     */
    @Override
    public int getItemCount() {
        return dataset.size();
    }


}
