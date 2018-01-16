package gov.nasa.jpl.iondtn.types;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Created by rwiewel on 11/13/17.
 */

public class DtnProtocolTest {

    @Test
    public void checkConstructor() throws Exception {
        DtnProtocol proto = new DtnProtocol("identifier", "123",
                "432",
                "2");

        assertEquals("identifier", proto.getIdentifier());
        assertEquals(123, proto.getPayloadFrameSize());
        assertEquals(432, proto.getOverheadFrameSize());
        assertEquals(2, proto.getProtocolClass());
    }

    @Test
    public void checkEquals() throws Exception {
        DtnProtocol proto = new DtnProtocol("identifier", "123",
                "432",
                "2");

        assertTrue(proto.equals(proto));

        DtnProtocol proto2 = new DtnProtocol("identifier", "123",
                "432", "2");

        assertTrue(proto.equals(proto2));

        DtnProtocol proto3 = new DtnProtocol("identifier2", "123",
                "432", "2");
        DtnProtocol proto4 = new DtnProtocol("identifier", "1234",
                "432", "2");
        DtnProtocol proto5 = new DtnProtocol("identifier", "123",
                "4324", "2");
        DtnProtocol proto6 = new DtnProtocol("identifier", "123",
                "432", "4");

        assertFalse(proto.equals(proto3));
        assertFalse(proto.equals(proto4));
        assertFalse(proto.equals(proto5));
        assertFalse(proto.equals(proto6));
    }
}
