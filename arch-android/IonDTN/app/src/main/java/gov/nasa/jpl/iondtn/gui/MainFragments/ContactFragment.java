package gov.nasa.jpl.iondtn.gui.MainFragments;

import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;

import gov.nasa.jpl.iondtn.R;
import gov.nasa.jpl.iondtn.backend.NativeAdapter;
import gov.nasa.jpl.iondtn.gui.AddEditDialogFragments.ContactDialogFragment;
import gov.nasa.jpl.iondtn.gui.MainActivity;
import gov.nasa.jpl.iondtn.gui.MainFragments.MainFragmentsAdapters
        .ContactRecyclerViewAdapter;

/**
 * List Configuration Fragment that uses {@link ContactRecyclerViewAdapter}
 * to display all configured contacts
 *
 * @author Robert Wiewel
 */
public class ContactFragment extends ConfigurationListFragment {
    private static final String TAG = "ContactFragment";

    /**
     * Starts fragment and sets title
     */
    @Override
    public void onStart() {
        super.onStart();
        getActivity().setTitle("Contacts");
    }

    @Override
    protected void launchAddAction() {
        android.support.v4.app.DialogFragment newFragment = ContactDialogFragment.newInstance();
        newFragment.show(this.getActivity().getSupportFragmentManager(), "ctf");
    }

    @Override
    protected void setAdapter() {
        String contact_list = "";

        if (getActivity() instanceof MainActivity) {
            try {
                contact_list = ((MainActivity) getActivity()).getAdminService()
                        .getContactListString();
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
        super.mAdapter = new ContactRecyclerViewAdapter(
                contact_list, this.getActivity().getSupportFragmentManager());
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
