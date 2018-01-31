package gov.nasa.jpl.iondtn.gui.MainFragments;


import android.os.AsyncTask;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v4.widget.SwipeRefreshLayout;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;
import gov.nasa.jpl.iondtn.gui.MainActivity;

/**
 * Fragment class that uses an RecyclerView to provide an configuration
 * option for a certain ION object. Usually used as superclass.
 */
public abstract class ConfigurationListFragment extends ConfigurationFragment {
    private RecyclerView mRecyclerView;
    protected RecyclerView.Adapter mAdapter;
    private static final String TAG = "ConfigListFragment";
    protected boolean abort = false;
    private boolean waitForService = false;
    private View localView;


    /**
     * Default empty public constructor of fragment object
     */
    public ConfigurationListFragment() {
        // Required empty public constructor
    }

    /**
     * Creates the fragment (including internal objects), also logs this
     * operation
     * @param savedInstanceState A previously saved state as Bundle
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
        onIonStatusUpdate(NativeAdapter.getStatus());
    }

    /**
     * Creates the actual ConfigurationFragment view
     * @param inflater The inflater for the view
     * @param container The ViewGroup where the fragment should be populated
     * @param savedInstanceState An previous instance of the object
     * @return A populated view of the ConfigurationFragment
     */
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {


        // Inflate the layout for this fragment
        View view = inflater.inflate(R.layout.fragment_configuration_list, container, false);
        this.localView = view;

        if (!abort) {
            FloatingActionButton fab = view.findViewById(R.id.fab);
            fab.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    launchAddAction();
                }
            });

            final SwipeRefreshLayout mSwipeRefreshLayout =
                    view.findViewById(R.id.swipe_container);
            mSwipeRefreshLayout.setOnRefreshListener(new SwipeRefreshLayout.OnRefreshListener() {
                @Override
                public void onRefresh() {
                    update();
                }
            });

            mRecyclerView = view.findViewById(R.id.recycler_view);


            // use a linear layout manager
            RecyclerView.LayoutManager mLayoutManager = new
                    LinearLayoutManager(getContext());
            mRecyclerView.setLayoutManager(mLayoutManager);

            // Set empty adapter to allow asynchronous data retrieval
            // Adapter will later be replaced by actual adapter
            setEmptyAdapter();

            if (getActivity() instanceof MainActivity) {
                if (((MainActivity) getActivity()).getAdminService() ==
                        null) {
                    waitForService = true;
                }
                else {
                    new ConfigurationListFragment.createRecyclerView().execute();
                }
            }
        }

        return view;
    }

    protected abstract void launchAddAction();

    protected abstract void setAdapter();

    protected void setEmptyAdapter(){
        // Set empty adapter to allow asynchronous data retrieval
        // (also used when ION is not available)
        mAdapter = new RecyclerView.Adapter() {
            @Override
            public RecyclerView.ViewHolder onCreateViewHolder
                    (ViewGroup parent, int viewType) {
                return null;
            }

            @Override
            public void onBindViewHolder
                    (RecyclerView.ViewHolder holder, int position) {

            }

            @Override
            public int getItemCount() {
                return 0;
            }
        };

        mRecyclerView.setAdapter(mAdapter);
        mRecyclerView.setVisibility(View.INVISIBLE);
    }

    private class createRecyclerView extends AsyncTask< String, Void,
            Integer> {

        createRecyclerView() {}
        SwipeRefreshLayout mSwipeRefreshLayout = (SwipeRefreshLayout)
                localView.findViewById(R.id.swipe_container);

        @Override
        protected void onPreExecute() {
            mSwipeRefreshLayout.setRefreshing(true);
            super.onPreExecute();
        }

        @Override
        protected Integer doInBackground(String... strings) {
            setAdapter();
            return 0;
        }

        @Override
        protected void onPostExecute(Integer integer) {
            mRecyclerView.setAdapter(mAdapter);

            // use this setting to improve performance if you know that changes
            // in content do not change the layout size of the RecyclerView
            mRecyclerView.setHasFixedSize(false);

            mRecyclerView.invalidate();
            mRecyclerView.setVisibility(View.VISIBLE);

            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mSwipeRefreshLayout.setRefreshing(false);
                }
            });
            Log.d(TAG, "onPostExecute: In post execute, set refreshing to " +
                    "false");

            super.onPostExecute(integer);
        }
    }

    private class updateRecyclerView extends AsyncTask<String, Void,
            Integer> {

        updateRecyclerView() {}
        SwipeRefreshLayout mSwipeRefreshLayout = (SwipeRefreshLayout)
                localView.findViewById(R.id.swipe_container);

        @Override
        protected void onPreExecute() {
            mSwipeRefreshLayout.setRefreshing(true);
            super.onPreExecute();
        }

        @Override
        protected Integer doInBackground(String... strings) {
            setAdapter();
            return 0;
        }

        @Override
        protected void onPostExecute(Integer integer) {
            Log.d(TAG, "onPostExecute: In post executed");
            mRecyclerView.setAdapter(mAdapter);
            mRecyclerView.invalidate();
            mRecyclerView.setVisibility(View.VISIBLE);
            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mSwipeRefreshLayout.setRefreshing(false);
                }
            });
            Log.d(TAG, "onPostExecute: In post execute, set refreshing to " +
                    "false");

            super.onPostExecute(integer);
        }
    }

    /**
     * Inflates the options menu with items when the fragment is visible
     * @param menu The menu object to inflate
     * @param inflater The menu inflater
     */
    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        inflater.inflate(R.menu.main, menu);
    }

    /**
     * Responds to clicks on an options menu item, in this case only the
     * refresh item exists, thus it updates the list
     * @param item The tapped MenuItem
     * @return true if event was handled, false otherwise
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_refresh) {
            new ConfigurationListFragment.updateRecyclerView().execute();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    /**
     * Defines behavior on when the NodeAdministrationService becomes available
     */
    @Override
    public void onServiceAvailable() {
        if (waitForService) {
            new ConfigurationListFragment.createRecyclerView().execute();
            waitForService = false;
        }
    }

    /**
     * Update the fragment
     */
    public void update() {
        new updateRecyclerView().execute();
    }
}
