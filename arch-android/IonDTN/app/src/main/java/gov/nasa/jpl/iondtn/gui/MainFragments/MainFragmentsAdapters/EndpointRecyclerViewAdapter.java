package gov.nasa.jpl.iondtn.gui.MainFragments.MainFragmentsAdapters;

import android.support.v4.app.FragmentManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Scanner;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.gui.AddEditDialogFragments.EidDialogFragment;
import gov.nasa.jpl.iondtn.types.DtnEndpointIdentifier;

/**
 * This Recycler adapter generates views for
 * {@link gov.nasa.jpl.iondtn.gui.MainFragments.EndpointFragment}.
 *
 * @author Robert Wiewel
 */
public class EndpointRecyclerViewAdapter extends RecyclerView
        .Adapter<EndpointRecyclerViewAdapter.CustomViewHolder>{
    private ArrayList<DtnEndpointIdentifier> dataset;
    private static final String TAG = "EidAdapter";
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
    public EndpointRecyclerViewAdapter(String input, FragmentManager fr) {
        Log.d(TAG, "EndpointRecyclerViewAdapter: got text: " + input);
        dataset = new ArrayList<>();
        parseContent(input);
        this.fragmentMgr = fr;
    }

    private void parseContent(String input) {
        Scanner linescanner = new Scanner(input);

        while(linescanner.hasNextLine()) {
            String delims = "[ ]+";
            String[] tokens = linescanner.nextLine().split(delims);
            DtnEndpointIdentifier.ReceivingBehavior r;

            if (tokens[1].equals("q")) {
                r = DtnEndpointIdentifier.ReceivingBehavior.QUEUE;
            }
            else {
                r = DtnEndpointIdentifier.ReceivingBehavior.DISCARD;
            }

            DtnEndpointIdentifier s = new DtnEndpointIdentifier(tokens[0], r,
                    tokens[2]);
            dataset.add(s);
            Log.d(TAG, "new DtnEndpointIdentifier object: " + s.toString());
        }
    }

    /**
     * Creates new view objects
     * @param parent The parent view group of the new item view object
     * @param viewType Not used but required
     * @return A new view object
     */
    @Override
    public EndpointRecyclerViewAdapter.CustomViewHolder onCreateViewHolder
    (final ViewGroup parent, int viewType) {
        // create a new view
        View v = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.recycler_eid_view_item_layout, parent,
                        false);
        // set the view's size, margins, paddings and layout parameters
        return new CustomViewHolder(v, parent);
    }

    /**
     * Replaces an view's content with new content
     * @param holder The view object
     * @param position The position of the object within the dataset
     */
    @Override
    public void onBindViewHolder(CustomViewHolder holder, int position) {
        // - get element from your dataset at this position
        // - replace the contents of the view with that element

        final int pos = position;

        holder.itemView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                android.support.v4.app.DialogFragment newFragment =
                        EidDialogFragment.newInstance(dataset
                                .get(pos));
                newFragment.show(fragmentMgr, "ef");
            }
        });

        TextView eid = holder.itemView.findViewById(R.id
                .textViewEid);
        TextView behavior = holder.itemView.findViewById(R.id
                .textViewBehavior);
        TextView script = holder.itemView.findViewById(R.id
                .textViewScript);

        eid.setText(dataset.get(position).getIdentifier());

        if (dataset.get(position).getBehavior() == DtnEndpointIdentifier
                .ReceivingBehavior.QUEUE) {
            behavior.setText(R.string.eid_option_queue);
        }
        else {
            behavior.setText(R.string.eid_option_discard);
        }

        script.setText(dataset.get(position).getScript());
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
