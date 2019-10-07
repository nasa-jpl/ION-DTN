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
import gov.nasa.jpl.iondtn.gui.AddEditDialogFragments.BcbRuleDialogFragment;
import gov.nasa.jpl.iondtn.types.DtnBcbRule;

/**
 * This Recycler adapter generates views for
 * {@link gov.nasa.jpl.iondtn.gui.MainFragments.BcbRuleFragment}.
 *
 * @author Robert Wiewel
 */
public class BcbRuleRecyclerViewAdapter extends RecyclerView
        .Adapter<BcbRuleRecyclerViewAdapter.CustomViewHolder>{
    private ArrayList<DtnBcbRule> dataset;
    private static final String TAG = "BcbRuleAdapter";
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
    public BcbRuleRecyclerViewAdapter(String input, FragmentManager fr) {
        Log.d(TAG, "BcbRuleRecyclerViewAdapter: got text: " + input);
        dataset = new ArrayList<>();
        parseContent(input);
        this.fragmentMgr = fr;
    }

    private void parseContent(String input) {
        Scanner linescanner = new Scanner(input);

        while(linescanner.hasNextLine()) {
            String delims = "[ ]+";
            String[] tokens = linescanner.nextLine().split(delims);

            DtnBcbRule s = new DtnBcbRule(tokens[0],
                    tokens[1],
                    tokens[2],
                    tokens[3],
                    tokens[4]);
            dataset.add(s);
            Log.d(TAG, "new DtnBcbRule object: " + s.toString());
        }
    }

    /**
     * Creates new view objects
     * @param parent The parent view group of the new item view object
     * @param viewType Not used but required
     * @return A new view object
     */
    @Override
    public BcbRuleRecyclerViewAdapter.CustomViewHolder onCreateViewHolder(final
                                                                          ViewGroup parent,
                                                                          int viewType) {
        // create a new view
        View v = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.recycler_bcbrule_view_item_layout, parent,
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
                        BcbRuleDialogFragment.newInstance(dataset
                                .get(pos));
                newFragment.show(fragmentMgr, "Bcbdf");
            }
        });

        TextView senderReceiver = holder.itemView.findViewById(R.id
                .textViewSenderReceiverName);
        TextView ciphersuite = holder.itemView.findViewById(R.id
                .textViewCiphersuiteName);
                TextView key = holder.itemView.findViewById(R.id
                .textViewKeyName);
        TextView blocktypenbr = holder.itemView.findViewById(R.id
                .textViewBlockId);



        senderReceiver.setText(dataset.get(position).getSenderEID() + " -> " +
                dataset.get(position).getReceiverEID());

        ciphersuite.setText(dataset.get(position).getCiphersuiteName());
        key.setText(dataset.get(position).getKeyName());
        blocktypenbr.setText(dataset.get(position).getBlockTypeNumber());
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
