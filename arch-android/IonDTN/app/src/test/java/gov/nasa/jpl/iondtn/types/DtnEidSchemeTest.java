package gov.nasa.jpl.iondtn.types;

import org.junit.Test;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Unit test that tests the type class DtnEidScheme
 *
 * @author Robert Wiewel
 */
public class DtnEidSchemeTest {

    @Test
    public void checkConstructor() throws Exception {
        DtnEidScheme eidScheme = new DtnEidScheme("identifier", "forwarder",
                "adminApp", false);

        assertEquals("identifier", eidScheme.getSchemeID());
        assertEquals("forwarder", eidScheme.getForwarderCommand());
        assertEquals("adminApp", eidScheme.getAdminAppCommand());
        assertEquals(false, eidScheme.getStatus());
    }

    @Test
    public void checkEquals() throws Exception {
        DtnEidScheme eidScheme = new DtnEidScheme("identifier", "forwarder",
                "adminApp", false);

        assertTrue(eidScheme.equals(eidScheme));

        DtnEidScheme eidScheme2 = new DtnEidScheme("identifier", "forwarder",
                "adminApp", false);

        assertTrue(eidScheme.equals(eidScheme2));

        DtnEidScheme eidScheme3 = new DtnEidScheme("identifier2", "forwarder",
                "adminApp", false);
        DtnEidScheme eidScheme4 = new DtnEidScheme("identifier", "forwarder2",
                "adminApp", false);
        DtnEidScheme eidScheme5 = new DtnEidScheme("identifier", "forwarder",
                "adminApp2", false);

        assertFalse(eidScheme.equals(eidScheme3));
        assertFalse(eidScheme.equals(eidScheme4));
        assertFalse(eidScheme.equals(eidScheme5));
    }
}
