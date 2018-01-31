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
import java.util.Scanner;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.gui.AddEditDialogFragments.ContactDialogFragment;
import gov.nasa.jpl.iondtn.types.DtnContact;
import gov.nasa.jpl.iondtn.types.DtnTime;

/**
 * This Recycler adapter generates views for
 * {@link gov.nasa.jpl.iondtn.gui.MainFragments.ContactFragment}.
 *
 * @author Robert Wiewel
 */
public class ContactRecyclerViewAdapter extends RecyclerView.Adapter<ContactRecyclerViewAdapter.CustomViewHolder>{
    private ArrayList<DtnContact> dataset;
    private static final String TAG = "ContactAdapter";
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
    public ContactRecyclerViewAdapter(String input, FragmentManager fr) {
        Log.d(TAG, "ContactRecyclerViewAdapter: got text: " + input);
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
                DtnContact c = new DtnContact(tokens[9],
                        tokens[12],
                        new DtnTime(tokens[1]),
                        new DtnTime(tokens[3]),
                        tokens[14],
                        tokens[17].substring(0, tokens[17].length() - 1));
                dataset.add(c);
                Log.d(TAG, "new DtnContact object: " + c.toString());
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
    public ContactRecyclerViewAdapter.CustomViewHolder onCreateViewHolder(final ViewGroup parent,
                                                                          int viewType) {
        // create a new view
        View v = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.recycler_contact_view_item_layout, parent, false);
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
                android.support.v4.app.DialogFragment newFragment = ContactDialogFragment.newInstance(dataset
                        .get(pos));
                newFragment.show(fragmentMgr, "ctf");
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
        TextView confidence = holder.itemView.findViewById(R.id
                .textViewConfidence);
        TextView rate = holder.itemView.findViewById(R.id
                .textViewRate);

        fromNode.setText(dataset.get(position).getFromNode());
        toNode.setText(dataset.get(position).getToNode());
        fromTime.setText(dataset.get(position).getFromTime().toString());
        toTime.setText(dataset.get(position).getToTime().toString());
        confidence.setText(dataset.get(position).getConfidence().toString());
        rate.setText(dataset.get(position).getXmitRate().toString());

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
