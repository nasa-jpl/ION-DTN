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
 * A {@link Fragment} that provides a tab design for the
 * {@link BabRuleFragment}, the {@link BibRuleFragment}, the
 * {@link BcbRuleFragment} and the {@link KeyFragment}.
 *
 * @author Robert Wiewel
 */
public class SecurityFragment extends Fragment {
    SecurityPagerAdapter pa;
    ViewPager mViewPager;

    /**
     * Starts fragment and sets title
     */
    @Override
    public void onStart() {
        super.onStart();
        getActivity().setTitle("Security");
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
                new SecurityPagerAdapter(
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
     * Returns the active instantiated BabRuleFragment
     * @return the BabRuleFragment
     */
    public BabRuleFragment getBabRuleFragment() {
        SecurityPagerAdapter ad = (SecurityPagerAdapter) mViewPager
                .getAdapter();
        return ad.getBabRuleFragment();
    }

    /**
     * Returns the active instantiated BibRuleFragment
     * @return the BibRuleFragment
     */
    public BibRuleFragment getBibRuleFragment() {
        SecurityPagerAdapter ad = (SecurityPagerAdapter) mViewPager
                .getAdapter();
        return ad.getBibRuleFragment();
    }

    /**
     * Returns the active instantiated BcbRuleFragment
     * @return the BcbRuleFragment
     */
    public BcbRuleFragment getBcbRuleFragment() {
        SecurityPagerAdapter ad = (SecurityPagerAdapter) mViewPager
                .getAdapter();
        return ad.getBcbRuleFragment();
    }

    /**
     * Returns the active instantiated KeyFragment
     * @return the KeyFragment
     */
    public KeyFragment getKeyFragment() {
        SecurityPagerAdapter ad = (SecurityPagerAdapter) mViewPager
                .getAdapter();
        return ad.getKeyFragment();
    }

    private class SecurityPagerAdapter extends FragmentStatePagerAdapter {
        private BabRuleFragment babFrag;
        private BibRuleFragment bibFrag;
        private BcbRuleFragment bcbFrag;
        private KeyFragment keyFrag;

        SecurityPagerAdapter(FragmentManager fm) {
            super(fm);
            babFrag = new BabRuleFragment();
            bibFrag = new BibRuleFragment();
            bcbFrag = new BcbRuleFragment();
            keyFrag = new KeyFragment();
        }

        @Override
        public Fragment getItem(int i) {
            switch(i) {
                case 0:
                    return babFrag;
                case 1:
                    return bibFrag;
                case 2:
                    return bcbFrag;
                case 3:
                    return keyFrag;
                default:
                    return null;
            }
        }

        @Override
        public int getCount() {
            return 4;
        }

        @Override
        public CharSequence getPageTitle(int position) {
            switch(position) {
                case 0:
                    return "BABRULE";
                case 1:
                    return "BIBRULE";
                case 2:
                    return "BCBRULE";
                case 3:
                    return "KEY";
                default:
                    return "ERROR";
            }
        }

        BabRuleFragment getBabRuleFragment() {
            return babFrag;
        }
        BibRuleFragment getBibRuleFragment() {
            return bibFrag;
        }
        BcbRuleFragment getBcbRuleFragment() {
            return bcbFrag;
        }
        KeyFragment getKeyFragment() {
            return keyFrag;
        }
    }
}
