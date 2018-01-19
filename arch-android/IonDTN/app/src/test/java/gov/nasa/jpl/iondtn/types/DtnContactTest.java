package gov.nasa.jpl.iondtn.types;

import org.junit.Test;

import java.util.Date;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Unit test that tests the type class DtnContact
 *
 * @author Robert Wiewel
 */

public class DtnContactTest {

    @Test
    public void checkStringConstructor() throws Exception {
        DtnContact contact = new DtnContact("fromNode",
                "toNode",
                "2017/11/13-12:00:01",
                "2017/11/13-13:10:01",
                "100001",
                "0.93");

        assertEquals(contact.getFromNode(), "fromNode");
        assertEquals(contact.getToNode(), "toNode");
        assertEquals(contact.getFromTime().toString(), "2017/11/13-12:00:01");
        assertEquals(contact.getToTime().toString(), "2017/11/13-13:10:01");
        assertEquals(contact.getXmitRate().toInt(), 100001);
        assertEquals(contact.getConfidence().toInt(), 93);
    }

    @Test
    public void checkPartStringConstructor() throws Exception {
        DtnTime time = new DtnTime(new Date());
        DtnDataRate rate = new DtnDataRate(32);
        DtnConfidence conf = new DtnConfidence(45);

        DtnContact contact = new DtnContact("a", "b",
                time, time, rate, conf);

        assertEquals(contact.getFromTime(), time);
        assertEquals(contact.getToTime(), time);
        assertEquals(contact.getXmitRate(), rate);
        assertEquals(contact.getConfidence(), conf);
    }

    @Test
    public void checkEquals() throws Exception {
        DtnContact contact = new DtnContact("fromNode",
                "toNode",
                "2017/11/13-12:00:01",
                "2017/11/13-13:10:01",
                "100001",
                "93");

        DtnContact contact2 = new DtnContact("fromNode",
                "toNode",
                "2017/11/13-12:00:01",
                "2017/11/13-13:10:01",
                "100001",
                "93");

        assertTrue(contact.equals(contact2));

        DtnContact contact3 = new DtnContact("fromNode2",
                "toNode",
                "2017/11/13-12:00:01",
                "2017/11/13-13:10:01",
                "100001",
                "93");

        assertFalse(contact.equals(contact3));
    }
}