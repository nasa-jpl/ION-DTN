package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Created by rwiewel on 11/13/17.
 */

public class DtnBabRuleTest {

    @Test
    public void checkConstructor() throws Exception {
        DtnBabRule bab = new DtnBabRule("sender", "receiver", "cipher",
                "key");

        assertEquals("sender", bab.getSenderEID());
        assertEquals("receiver", bab.getReceiverEID());
        assertEquals("cipher", bab.getCiphersuiteName());
        assertEquals("key", bab.getKeyName());
    }

    @Test
    public void checkEquals() throws Exception {
        DtnBabRule bab = new DtnBabRule("sender", "receiver", "cipher",
                "key");

        assertTrue(bab.equals(bab));

        DtnBabRule bab2 = new DtnBabRule("sender", "receiver", "cipher",
                "key");

        assertTrue(bab.equals(bab2));

        DtnBabRule bab3 = new DtnBabRule("sender2", "receiver", "cipher",
                "key");
        DtnBabRule bab4 = new DtnBabRule("sender", "receiver2", "cipher",
                "key");
        DtnBabRule bab5 = new DtnBabRule("sender", "receiver", "cipher2",
                "key");
        DtnBabRule bab6 = new DtnBabRule("sender", "receiver", "cipher",
                "key2");

        assertFalse(bab.equals(bab3));
        assertFalse(bab.equals(bab4));
        assertFalse(bab.equals(bab5));
        assertFalse(bab.equals(bab6));
    }
}
