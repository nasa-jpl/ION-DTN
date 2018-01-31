package gov.nasa.jpl.iondtn.gui;

import gov.nasa.jpl.iondtn.backend.NativeAdapter;

/**
 * ION status change listener interface. Allows callback registering to get
 * notified when ION-DTN changes occur
 */
public interface OnIonStatusChangeListener{
    /**
     * Notifies that the ION-DTN status has changed
     * @param status the new status
     */
    void onIonStatusUpdate(NativeAdapter.SystemStatus status);

    /**
     * Notifies that the NodeAdministrationService is now available
     */
    void onServiceAvailable();
}