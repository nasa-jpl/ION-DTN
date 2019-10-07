// INodeAdminService.aidl
package gov.nasa.jpl.iondtn;

// Declare any non-default types here with import statements

import gov.nasa.jpl.iondtn.INodeAdminListener;

interface INodeAdminService {

    // Register a listener that is notified whenever a status change of the
    // underlying ION instance occurs
    int registerStatusChangeListener(INodeAdminListener lst);

    // Register a listener that is notified whenever a status change of the
    // underlying ION instance occurs
    int unregisterStatusChangeListener(INodeAdminListener lst);

    // Start the underlying ION instance
    void startION();

    // Stop the unerlying ION instance
    void stopION();

    // Provides a list of all contacts in ION as a String
    String getContactListString();

    // Provides a list of all ranges in ION as a String
    String getRangeListString();

    // Provides a list of all schemes in ION as a String
    String getSchemeListString();

    // Provides a list of all registered endpoints in ION as a String
    String getEndpointListString();

    // Provides a list of all registered protocols in ION as a String
    String getProtocolListString();

    // Provides a list of all registered inducts in ION as a String
    String getInductListString();

    // Provides a list of all registered outducts in ION as a String
    String getOutductListString();

    // Provides a list of all registered bab rules in ION as a String
    String getBabRuleListString();

    // Provides a list of all registered bib rules in ION as a String
    String getBibRuleListString();

    // Provides a list of all registered bib rules in ION as a String
    String getBcbRuleListString();

    // Provides a list of all registered keys in ION as a String
    String getKeyListString();
}
