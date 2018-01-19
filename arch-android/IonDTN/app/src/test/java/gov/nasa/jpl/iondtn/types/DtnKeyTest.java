package gov.nasa.jpl.iondtn.types;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Created by rwiewel on 11/13/17.
 */

public class DtnKeyTest {

    @Test
    public void checkConstructor() throws Exception {
        DtnKey key = new DtnKey("name", "length");

        assertEquals("name", key.getKeyName());
        assertEquals("length", key.getKeyLength());
    }

    @Test
    public void checkEquals() throws Exception {
        DtnKey key = new DtnKey("name", "length");

        assertTrue(key.equals(key));

        DtnKey key2 = new DtnKey("name", "length");


        assertTrue(key.equals(key2));

        DtnKey key3 = new DtnKey("name1", "length");
        DtnKey key4 = new DtnKey("name", "length2");

        assertFalse(key.equals(key3));
        assertFalse(key.equals(key4));
    }
}
