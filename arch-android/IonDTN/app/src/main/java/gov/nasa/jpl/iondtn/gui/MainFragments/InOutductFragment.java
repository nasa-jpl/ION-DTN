package gov.nasa.jpl.iondtn.gui.MainFragments;


import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import gov.nasa.jpl.iondtn.R;

/**
 * A {@link Fragment} that provides a tab design for both the
 * {@link InductFragment} and the {@link OutductFragment}.
 *
 * @author Robert Wiewel
 */
public class InOutductFragment extends Fragment {
    InOutductPagerAdapter pa;
    ViewPager mViewPager;

    /**
     * Starts fragment and sets title
     */
    @Override
    public void onStart() {
        super.onStart();
        getActivity().setTitle("In-/Outducts");
    }

    /**
     * Populates the layout of the fragment
     * @param inflater The inflater
     * @param container The ViewGroup container
     * @param savedInstanceState An (optional) previously saved instance of
     *                           this fragment
     * @return A populated view
     */
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        // Inflate the layout for this fragment
        View v = inflater.inflate(R.layout.fragment_tab_pager, container,
                false);
        // ViewPager and its adapters use support library
        // fragments, so use getSupportFragmentManager.
        pa =
                new InOutductPagerAdapter(
                        getChildFragmentManager());
        mViewPager = v.findViewById(R.id.pager);
        mViewPager.setAdapter(pa);

        return v;
    }

    /**
     * Creates a new object of this type
     * @param savedInstanceState An (optional) previously saved instance of
     *                           this fragment
     */
    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    /**
     * Returns the active instantiated InductFragment
     * @return the InductFragment
     */
    public InductFragment getInductFragment() {
        InOutductPagerAdapter ad = (InOutductPagerAdapter)mViewPager
                .getAdapter();
        return ad.getInductFragment();
    }

    /**
     * Returns the active instantiated OutductFragment
     * @return the OutductFragment
     */
    public OutductFragment getOutductFragment() {
        InOutductPagerAdapter ad = (InOutductPagerAdapter)mViewPager
                .getAdapter();
        return ad.getOutductFragment();
    }

    private class InOutductPagerAdapter extends FragmentStatePagerAdapter {
        private InductFragment indFrag;
        private OutductFragment outFrag;

        InOutductPagerAdapter(FragmentManager fm) {
            super(fm);
            indFrag = new InductFragment();
            outFrag = new OutductFragment();
        }

        @Override
        public Fragment getItem(int i) {
            if (i==0) {
                return indFrag;
            }
            else {
                return outFrag;
            }
        }

        @Override
        public int getCount() {
            return 2;
        }

        @Override
        public CharSequence getPageTitle(int position) {
            if (position == 0) {
                return "INDUCTS";
            }
            else {
                return "OUTDUCTS";
            }
        }

        private InductFragment getInductFragment() {
            return indFrag;
        }

        private OutductFragment getOutductFragment() {
            return outFrag;
        }

    }
}
