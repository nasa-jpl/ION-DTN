// INodeAdminListener.aidl
package gov.nasa.jpl.iondtn;

// Declare any non-default types here with import statements

interface INodeAdminListener {
    /**
     * Is called when a bundle assigned to the subscribers eid has been
     * received in ION
     */
    void notifyStatusChanged(String status);
}
