package gov.nasa.jpl.iondtn.gui.MainFragments;

import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;
import gov.nasa.jpl.iondtn.gui.AddEditDialogFragments.SchemeDialogFragment;
import gov.nasa.jpl.iondtn.gui.MainActivity;
import gov.nasa.jpl.iondtn.gui.MainFragments.MainFragmentsAdapters
        .SchemeRecyclerViewAdapter;

/**
 * List Configuration Fragment that uses {@link SchemeRecyclerViewAdapter}
 * to display all configured schemes
 *
 * @author Robert Wiewel
 */
public class SchemeFragment extends ConfigurationListFragment {
    private static final String TAG = "SchemeFragment";

    /**
     * Starts fragment and sets title
     */
    @Override
    public void onStart() {
        super.onStart();
        getActivity().setTitle("Schemes");
    }

    @Override
    protected void launchAddAction() {
        android.support.v4.app.DialogFragment newFragment = SchemeDialogFragment
                .newInstance();
        newFragment.show(this.getActivity().getSupportFragmentManager(), "sf");
    }

    @Override
    protected void setAdapter() {
        String scheme_list = "";

        if (getActivity() instanceof MainActivity) {
            try {
                scheme_list = ((MainActivity) getActivity()).getAdminService()
                        .getSchemeListString();
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
        super.mAdapter = new SchemeRecyclerViewAdapter(
                scheme_list, this.getActivity().getSupportFragmentManager());
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
