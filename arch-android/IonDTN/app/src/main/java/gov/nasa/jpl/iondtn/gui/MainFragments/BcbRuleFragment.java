package gov.nasa.jpl.iondtn.gui.MainFragments;

import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;
import gov.nasa.jpl.iondtn.gui.AddEditDialogFragments.BcbRuleDialogFragment;
import gov.nasa.jpl.iondtn.gui.MainActivity;
import gov.nasa.jpl.iondtn.gui.MainFragments.MainFragmentsAdapters
        .BcbRuleRecyclerViewAdapter;


/**
 * List Configuration Fragment that uses {@link BcbRuleRecyclerViewAdapter}
 * to display all configured BcbRules
 *
 * @author Robert Wiewel
 */
public class BcbRuleFragment extends ConfigurationListFragment {
    private static final String TAG = "BcbFrag";

    /**
     * Starts fragment and sets title
     */
    @Override
    public void onStart() {
        super.onStart();
        getActivity().setTitle("Security");
    }

    @Override
    protected void launchAddAction() {
        android.support.v4.app.DialogFragment newFragment =
                BcbRuleDialogFragment.newInstance();
        newFragment.show(this.getActivity().getSupportFragmentManager(),
                "bcbdf");
    }

    @Override
    protected void setAdapter() {
        String bcbrule_list = "";

        if (getActivity() instanceof MainActivity) {
            try {
                bcbrule_list = ((MainActivity) getActivity()).getAdminService()
                        .getBcbRuleListString();
            }
            catch (RemoteException e){
                Toast.makeText(getContext(), getString(R.string
                        .errorRetrieveData), Toast
                        .LENGTH_LONG).show();
            }
        }
        else {
            Log.e(TAG, "setAdapter: Instantiated from wrong activity!");
        }

        // specify an adapter (see also next example)
        super.mAdapter = new BcbRuleRecyclerViewAdapter(
                bcbrule_list, this.getActivity().getSupportFragmentManager());
    }

    /**
     * Handles ION status changes when fragment is active/visible
     * @param status The changed status
     */
    @Override
    public void onIonStatusUpdate(NativeAdapter.SystemStatus status) {
        if (status != NativeAdapter.SystemStatus.STARTED) {
            Toast.makeText(getContext(), getString(R.string
                    .errorIonNotAvailable), Toast
                    .LENGTH_LONG).show();
            getActivity().getSupportFragmentManager().popBackStack();
            abort = true;
        }
    }
}
