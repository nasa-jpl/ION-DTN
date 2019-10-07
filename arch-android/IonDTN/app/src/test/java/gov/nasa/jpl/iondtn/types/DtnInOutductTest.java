package gov.nasa.jpl.iondtn.types;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Created by rwiewel on 11/13/17.
 */

public class DtnInOutductTest {

    @Test
    public void checkConstructor() throws Exception {
        DtnInOutduct inoutduct = new DtnInOutduct(DtnInOutduct.IOType.INDUCT,
                "protName", "ductName", "cmd", false);

        assertEquals(DtnInOutduct.IOType.INDUCT, inoutduct.getType());
        assertEquals("protName", inoutduct.getProtocolName());
        assertEquals("ductName", inoutduct.getDuctName());
        assertEquals("cmd", inoutduct.getCmd());
        assertEquals(false, inoutduct.getStatus());
    }

    @Test
    public void checkEquals() throws Exception {
        DtnInOutduct inoutduct = new DtnInOutduct(DtnInOutduct.IOType.INDUCT,
                "protName", "ductName", "cmd", false);

        assertTrue(inoutduct.equals(inoutduct));

        DtnInOutduct inoutduct2 = new DtnInOutduct(DtnInOutduct.IOType.INDUCT,
                "protName", "ductName", "cmd", false);

        assertTrue(inoutduct.equals(inoutduct2));

        DtnInOutduct inoutduct3 = new DtnInOutduct(DtnInOutduct.IOType.OUTDUCT,
                "protName", "ductName", "cmd", false);
        DtnInOutduct inoutduct4 = new DtnInOutduct(DtnInOutduct.IOType.INDUCT,
                "protName2", "ductName", "cmd", false);
        DtnInOutduct inoutduct5 = new DtnInOutduct(DtnInOutduct.IOType.INDUCT,
                "protName", "ductName2", "cmd", false);
        DtnInOutduct inoutduct6 = new DtnInOutduct(DtnInOutduct.IOType.INDUCT,
                "protName", "ductName", "cmd2", false);
        DtnInOutduct inoutduct7 = new DtnInOutduct(DtnInOutduct.IOType.INDUCT,
                "protName", "ductName", "cmd", true);

        assertFalse(inoutduct.equals(inoutduct3));
        assertFalse(inoutduct.equals(inoutduct4));
        assertFalse(inoutduct.equals(inoutduct5));
        assertFalse(inoutduct.equals(inoutduct6));
        assertFalse(inoutduct.equals(inoutduct7));
    }
}
