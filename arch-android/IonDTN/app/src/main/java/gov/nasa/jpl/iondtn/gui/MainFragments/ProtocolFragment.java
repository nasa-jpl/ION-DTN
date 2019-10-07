package gov.nasa.jpl.iondtn.gui.MainFragments;

import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;
import gov.nasa.jpl.iondtn.gui.AddEditDialogFragments.ProtocolDialogFragment;
import gov.nasa.jpl.iondtn.gui.MainActivity;
import gov.nasa.jpl.iondtn.gui.MainFragments.MainFragmentsAdapters
        .ProtocolRecyclerViewAdapter;

/**
 * List Configuration Fragment that uses {@link ProtocolRecyclerViewAdapter}
 * to display all configured protocols
 *
 * @author Robert Wiewel
 */
public class ProtocolFragment extends ConfigurationListFragment {
    private static final String TAG = "ProtocolFrag";

    /**
     * Starts fragment and sets title
     */
    @Override
    public void onStart() {
        super.onStart();
        getActivity().setTitle("Protocols");
    }

    @Override
    protected void launchAddAction() {
        android.support.v4.app.DialogFragment newFragment =
                ProtocolDialogFragment.newInstance();
        newFragment.show(this.getActivity().getSupportFragmentManager(), "pf");
    }

    @Override
    protected void setAdapter() {
        String protocol_list = "";

        if (getActivity() instanceof MainActivity) {
            try {
                protocol_list = ((MainActivity) getActivity()).getAdminService()
                        .getProtocolListString();
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
        super.mAdapter = new ProtocolRecyclerViewAdapter(
                protocol_list, this.getActivity().getSupportFragmentManager());
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
