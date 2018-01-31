package gov.nasa.jpl.iondtn.types;

import org.junit.Test;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Unit test that tests the type classes DtnTime
 *
 * @author Robert Wiewel
 */

public class DtnTimeTest {

    @Test
    public void checkDateToDate() throws Exception {
        Date dt = new Date();

        DtnTime time = new DtnTime(dt);
        assertEquals(dt, time);
    }

    @Test
    public void checkDateToStr() throws Exception {
        Date dt = new Date(1507135064000L);

        DtnTime time = new DtnTime(dt);
        assertEquals("2017/10/04-16:37:44", time.toString());
    }

    @Test
    public void checkStrToStr() throws Exception {
        DtnTime time = new DtnTime("2017/11/10-16:37:44");
        assertEquals("2017/11/10-16:37:44", time.toString());
    }

    @Test
    public void checkStrToDate() throws Exception {
        SimpleDateFormat format = new SimpleDateFormat("yyyy/MM/dd-HH:mm:ss");
        Date dt = new Date();

        try {
            dt = format.parse("2017/11/10-16:37:44");
        } catch (ParseException e) {
            e.printStackTrace();
        }

        DtnTime time = new DtnTime("2017/11/10-16:37:44");
        assertEquals(format.format(dt), time.toString());
    }

    @Test
    public void checkDateToStrTimezone() throws Exception {
        DtnTime time = new DtnTime("2017/11/10-16:37:44");

        assertEquals("2017/11/10-08:37:44", time.toString(TimeZone
                .getTimeZone(("America/Los_Angeles"))));
    }

    @Test
    public void checkEqual() throws Exception {
        DtnTime time1 = new DtnTime("2017/11/10-16:37:44");
        DtnTime time2 = new DtnTime("2017/11/10-16:37:44");

        assertTrue(time1.equals(time2));
    }

    @Test
    public void checkUnequal() throws Exception {
        DtnTime time1 = new DtnTime("2017/11/10-16:37:44");
        DtnTime time2 = new DtnTime("2017/11/10-16:38:45");

        assertFalse(time1.equals(time2));
    }
}