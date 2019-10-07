package gov.nasa.jpl.iondtn.types;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Created by rwiewel on 11/13/17.
 */

public class DtnRangeTest {

    @Test
    public void checkConstructor() throws Exception {
        DtnTime startTime = new DtnTime("2017/11/13-12:00:01");
        DtnTime endTime = new DtnTime("2017/11/13-13:10:01");

        DtnRange range = new DtnRange("fromNode",
                "toNode",
                startTime,
                endTime,
                42);

        assertEquals("fromNode", range.getFromNode());
        assertEquals("toNode", range.getToNode());
        assertEquals(startTime, range.getFromTime());
        assertEquals(endTime, range.getToTime());
        assertEquals(42, range.getOwlt());
    }

    @Test
    public void checkEquals() throws Exception {
        DtnTime startTime = new DtnTime("2017/11/13-12:00:01");
        DtnTime endTime = new DtnTime("2017/11/13-13:10:01");

        DtnRange range = new DtnRange("fromNode",
                "toNode",
                startTime,
                endTime,
                42);

        assertTrue(range.equals(range));

        DtnTime startTime2 = new DtnTime("2017/11/13-12:00:01");
        DtnTime endTime2 = new DtnTime("2017/11/13-13:10:01");

        DtnRange range2 = new DtnRange("fromNode",
                "toNode",
                startTime,
                endTime,
                42);

        assertTrue(range.equals(range2));

        DtnRange range3 = new DtnRange("fromNode2",
                "toNode",
                startTime,
                endTime,
                42);
        DtnRange range4 = new DtnRange("fromNode",
                "toNode2",
                startTime,
                endTime,
                42);
        DtnRange range5 = new DtnRange("fromNode",
                "toNode",
                endTime,
                endTime,
                42);
        DtnRange range6 = new DtnRange("fromNode",
                "toNode",
                startTime,
                startTime,
                42);
        DtnRange range7 = new DtnRange("fromNode",
                "toNode",
                endTime,
                startTime,
                42);
        DtnRange range8 = new DtnRange("fromNode",
                "toNode",
                endTime,
                startTime,
                43);

        assertFalse(range.equals(range3));
        assertFalse(range.equals(range4));
        assertFalse(range.equals(range5));
        assertFalse(range.equals(range6));
        assertFalse(range.equals(range7));
        assertFalse(range.equals(range8));
    }
}
