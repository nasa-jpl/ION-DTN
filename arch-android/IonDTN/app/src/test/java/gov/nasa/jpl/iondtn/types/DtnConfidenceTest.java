package gov.nasa.jpl.iondtn.types;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Unit test that tests the type class DtnConfidence
 *
 * @author Robert Wiewel
 */

public class DtnConfidenceTest {

    @Test
    public void checkIntToInt() throws Exception {
        DtnConfidence conf = new DtnConfidence(95);

        assertEquals(95, conf.toInt());
    }

    @Test
    public void checkStrToInt() throws Exception {
        DtnConfidence conf = new DtnConfidence("0.82");

        assertEquals(82, conf.toInt());
    }

    @Test
    public void checkFloatToInt() throws Exception {
        DtnConfidence conf = new DtnConfidence(0.43f);

        assertEquals(43, conf.toInt());
    }

    @Test
    public void checkToFloat() throws Exception {
        DtnConfidence conf = new DtnConfidence(32);

        assertEquals(0.32f, conf.toFloat(), 1e-15);
    }

    @Test
    public void checkToString() throws Exception {
        DtnConfidence conf = new DtnConfidence(33);

        assertEquals("33", conf.toString());
    }

    @Test
    public void checkTooLow() throws Exception {
        DtnConfidence conf1 = new DtnConfidence(-4);
        DtnConfidence conf2 = new DtnConfidence("-28");
        DtnConfidence conf3 = new DtnConfidence(-0.36f);

        assertEquals(0, conf1.toInt());
        assertEquals(0, conf2.toInt());
        assertEquals(0, conf3.toInt());
    }

    @Test
    public void checkTooHigh() throws Exception {
        DtnConfidence conf1 = new DtnConfidence(235);
        DtnConfidence conf2 = new DtnConfidence("4256");
        DtnConfidence conf3 = new DtnConfidence(12.0f);

        assertEquals(100, conf1.toInt());
        assertEquals(100, conf2.toInt());
        assertEquals(100, conf3.toInt());
    }

    @Test
    public void checkEqual() throws Exception {
        DtnConfidence conf1 = new DtnConfidence(12);
        DtnConfidence conf2 = new DtnConfidence(12);

        assertTrue(conf1.equals(conf2));
    }

    @Test
    public void checkUnequal() throws Exception {
        DtnConfidence conf1 = new DtnConfidence(12);
        DtnConfidence conf2 = new DtnConfidence(14);

        assertFalse(conf1.equals(conf2));
    }
}