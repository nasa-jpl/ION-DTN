// BundleService.aidl
package gov.nasa.jpl.iondtn;

import gov.nasa.jpl.iondtn.IBundleReceiverListener;
import gov.nasa.jpl.iondtn.DtnBundle;

interface IBundleService {
    // Open an endpoint object with the specified source eid
    boolean openEndpoint(String src_eid, IBundleReceiverListener listener);

    // Close the previously opened endpoint object
    boolean closeEndpoint();

    boolean sendBundle(in DtnBundle b);
}
