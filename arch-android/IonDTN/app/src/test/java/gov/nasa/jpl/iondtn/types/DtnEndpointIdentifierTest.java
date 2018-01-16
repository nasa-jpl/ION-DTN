package gov.nasa.jpl.iondtn.types;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Unit test that tests the type classes DtnEndpointIdentifierTest
 *
 * @author Robert Wiewel
 */

public class DtnEndpointIdentifierTest {

    @Test
    public void checkConstructor() throws Exception {
        DtnEndpointIdentifier eid = new DtnEndpointIdentifier("identifier",
                DtnEndpointIdentifier.ReceivingBehavior.QUEUE, "script");

        assertEquals("identifier", eid.getIdentifier());
        assertEquals(DtnEndpointIdentifier.ReceivingBehavior.QUEUE, eid.getBehavior());
        assertEquals("script", eid.getScript());
    }

    @Test
    public void checkEquals() throws Exception {
        DtnEndpointIdentifier eid = new DtnEndpointIdentifier("identifier",
                DtnEndpointIdentifier.ReceivingBehavior.QUEUE, "script");

        assertTrue(eid.equals(eid));

        DtnEndpointIdentifier eid2 = new DtnEndpointIdentifier("identifier",
                DtnEndpointIdentifier.ReceivingBehavior.QUEUE, "script");

        assertTrue(eid.equals(eid2));

        DtnEndpointIdentifier eid3 = new DtnEndpointIdentifier("identifier1",
                DtnEndpointIdentifier.ReceivingBehavior.QUEUE, "script");
        DtnEndpointIdentifier eid4 = new DtnEndpointIdentifier("identifier",
                DtnEndpointIdentifier.ReceivingBehavior.DISCARD, "script");
        DtnEndpointIdentifier eid5 = new DtnEndpointIdentifier("identifier",
                DtnEndpointIdentifier.ReceivingBehavior.QUEUE, "script2");

        assertFalse(eid.equals(eid3));
        assertFalse(eid.equals(eid4));
        assertFalse(eid.equals(eid5));
    }
}