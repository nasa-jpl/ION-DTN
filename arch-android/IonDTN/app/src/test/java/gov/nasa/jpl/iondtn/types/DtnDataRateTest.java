package gov.nasa.jpl.iondtn.types;

import org.junit.Test;

import java.util.Date;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Unit test that tests the type class DtnDataRate
 *
 * @author Robert Wiewel
 */

public class DtnDataRateTest {

    @Test
    public void checkIntToInt() throws Exception {
        DtnDataRate rate = new DtnDataRate(1500);

        assertEquals(1500, rate.toInt());
    }

    @Test
    public void checkIntToString() throws Exception {
        DtnDataRate rate = new DtnDataRate(3500);

        assertEquals("3500", rate.toString());
    }

    @Test
    public void checkString() throws Exception {
        DtnDataRate rate = new DtnDataRate("1482");

        assertEquals(1482, rate.toInt());
    }

    @Test
    public void checkEqual() throws Exception {
        DtnDataRate rate1 = new DtnDataRate("1482");
        DtnDataRate rate2 = new DtnDataRate("1482");

        assertTrue(rate1.equals(rate2));
    }

    @Test
    public void checkUnequal() throws Exception {
        DtnDataRate rate1 = new DtnDataRate("1482");
        DtnDataRate rate2 = new DtnDataRate("123471347");

        assertFalse(rate1.equals(rate2));
    }
}