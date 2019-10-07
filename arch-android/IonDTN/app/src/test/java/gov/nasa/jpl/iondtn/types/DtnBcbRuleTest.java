package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Created by rwiewel on 11/13/17.
 */

public class DtnBcbRuleTest {

    @Test
    public void checkConstructor() throws Exception {
        DtnBcbRule bcb = new DtnBcbRule("sender", "receiver", "cipher",
                "key", "block");

        assertEquals("sender", bcb.getSenderEID());
        assertEquals("receiver", bcb.getReceiverEID());
        assertEquals("cipher", bcb.getCiphersuiteName());
        assertEquals("key", bcb.getKeyName());
        assertEquals("block", bcb.getBlockTypeNumber());
    }

    @Test
    public void checkEquals() throws Exception {
        DtnBcbRule bcb = new DtnBcbRule("sender", "receiver", "cipher",
                "key", "block");

        assertTrue(bcb.equals(bcb));

        DtnBcbRule bcb2 = new DtnBcbRule("sender", "receiver", "cipher",
                "key", "block");

        assertTrue(bcb.equals(bcb2));

        DtnBcbRule bcb3 = new DtnBcbRule("sender2", "receiver", "cipher",
                "key", "block");
        DtnBcbRule bcb4 = new DtnBcbRule("sender", "receiver2", "cipher",
                "key", "block");
        DtnBcbRule bcb5 = new DtnBcbRule("sender", "receiver", "cipher2",
                "key", "block");
        DtnBcbRule bcb6 = new DtnBcbRule("sender", "receiver", "cipher",
                "key2", "block");
        DtnBcbRule bcb7 = new DtnBcbRule("sender", "receiver", "cipher",
                "key", "block2");

        assertFalse(bcb.equals(bcb3));
        assertFalse(bcb.equals(bcb4));
        assertFalse(bcb.equals(bcb5));
        assertFalse(bcb.equals(bcb6));
        assertFalse(bcb.equals(bcb7));
    }
}
