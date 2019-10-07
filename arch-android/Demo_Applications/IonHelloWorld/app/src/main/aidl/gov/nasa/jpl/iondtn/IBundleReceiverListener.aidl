// IBundleReceiverListener.aidl
package gov.nasa.jpl.iondtn;

import gov.nasa.jpl.iondtn.DtnBundle;

interface IBundleReceiverListener {
    /**
     * Is called when a bundle assigned to the subscribers eid has been
     * received in ION
     */
    int notifyBundleReceived(in DtnBundle b);
}
