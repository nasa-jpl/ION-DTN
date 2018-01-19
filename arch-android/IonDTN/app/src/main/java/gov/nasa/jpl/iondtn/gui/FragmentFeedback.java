package gov.nasa.jpl.iondtn.gui;

/**
 * Interface that allows fragments to signal back to the main activity
 */
public interface FragmentFeedback {
    /**
     * Requests the deactivation of the drawer menu in the main activity
     */
    void deactivateDrawerMenu();

    /**
     * Requests the activation of the drawer menu in the main activity
     * @param partial Specifies if everything should be activated or only the
     *                items feasible when ion is not running
     */
    void activateDrawerMenu(boolean partial);
}