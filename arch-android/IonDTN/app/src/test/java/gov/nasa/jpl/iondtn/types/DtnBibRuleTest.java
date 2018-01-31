package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Created by rwiewel on 11/13/17.
 */

public class DtnBibRuleTest {

    @Test
    public void checkConstructor() throws Exception {
        DtnBibRule bib = new DtnBibRule("sender", "receiver", "cipher",
                "key", "block");

        assertEquals("sender", bib.getSenderEID());
        assertEquals("receiver", bib.getReceiverEID());
        assertEquals("cipher", bib.getCiphersuiteName());
        assertEquals("key", bib.getKeyName());
        assertEquals("block", bib.getBlockTypeNumber());
    }

    @Test
    public void checkEquals() throws Exception {
        DtnBibRule bib = new DtnBibRule("sender", "receiver", "cipher",
                "key", "block");

        assertTrue(bib.equals(bib));

        DtnBibRule bib2 = new DtnBibRule("sender", "receiver", "cipher",
                "key", "block");

        assertTrue(bib.equals(bib2));

        DtnBibRule bib3 = new DtnBibRule("sender2", "receiver", "cipher",
                "key", "block");
        DtnBibRule bib4 = new DtnBibRule("sender", "receiver2", "cipher",
                "key", "block");
        DtnBibRule bib5 = new DtnBibRule("sender", "receiver", "cipher2",
                "key", "block");
        DtnBibRule bib6 = new DtnBibRule("sender", "receiver", "cipher",
                "key2", "block");
        DtnBibRule bib7 = new DtnBibRule("sender", "receiver", "cipher",
                "key", "block2");

        assertFalse(bib.equals(bib3));
        assertFalse(bib.equals(bib4));
        assertFalse(bib.equals(bib5));
        assertFalse(bib.equals(bib6));
        assertFalse(bib.equals(bib7));
    }
}
