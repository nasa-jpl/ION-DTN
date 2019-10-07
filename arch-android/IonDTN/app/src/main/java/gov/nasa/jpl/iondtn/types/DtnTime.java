package gov.nasa.jpl.iondtn.types;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

/**
 *  The DtnTime class represents a time(stamp) object in the DTN space. The RFC
 *  5050 states that a timestamp is represented as the number of seconds
 *  between the particular point in time and 00:00:00 of 01/01/2000.
 *
 *  Due to the size of the int type (2^32) only points in time till 02/07/2136
 *  6:28:16 can be represented with this class
 *
 *  @see <a href="https://tools.ietf.org/html/rfc5050">RFC 5050</a>
 *
 *  @author Robert Wiewel
 */

public class DtnTime extends Date {
    private Date dt;

    private final long OFFSET_EPOCH_UNIX_DTN = 946684800000l;  // offset in
    // seconds

    /**
     * Constructor that allows the instantiation with a string that
     * represents a seconds value
     * @param str String representing the time in the
     *            format "yyyy/MM/dd-HH:mm:ss"
     */
    public DtnTime(String str) throws ParseException {
        SimpleDateFormat format = new SimpleDateFormat("yyyy/MM/dd-HH:mm:ss");
        format.setTimeZone(TimeZone.getTimeZone("UTC"));

        this.dt = format.parse(str);
    }

    /**
     * Constructor that allows the instantiation with a Java Date object
     * @param date Date object that represents the specific point in time
     */
    public DtnTime(Date date) {
        this.dt = date;
    }

    /**
     * Returns the contained point in time as a String
     * @return String that represents the time in a human-readable format (in
     * the UTC timezone)
     */
    @Override
    public String toString() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy/MM/dd-HH:mm:ss",
                Locale.US);
        format.setTimeZone(TimeZone.getTimeZone("UTC"));
        return format.format(dt);
    }

    // FIXME add description
    public String toString(TimeZone tz) {
        SimpleDateFormat format = new SimpleDateFormat("yyyy/MM/dd-HH:mm:ss",
                Locale.US);
        format.setTimeZone(tz);
        return format.format(dt);
    }

    public String getTimeString() {
        SimpleDateFormat format = new SimpleDateFormat("HH:mm:ss",
                Locale.US);
        format.setTimeZone(TimeZone.getTimeZone("UTC"));
        return format.format(dt);
    }

    public String getTimeString(TimeZone tz) {
        SimpleDateFormat format = new SimpleDateFormat("HH:mm:ss",
                Locale.US);
        format.setTimeZone(tz);
        return format.format(dt);
    }

    public String getDateString() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy/MM/dd",
                Locale.US);
        format.setTimeZone(TimeZone.getTimeZone("UTC"));
        return format.format(dt);
    }

    public String getDateString(TimeZone tz) {
        SimpleDateFormat format = new SimpleDateFormat("yyyy/MM/dd",
                Locale.US);
        format.setTimeZone(tz);
        return format.format(dt);
    }


    /**
     * Returns the contained point in time as an Int
     * @return Int that represents the number of seconds since 01/01/2000
     * 00:00:00
     */
    public long toLong() {
        return (long)((dt.getTime()/1000)-OFFSET_EPOCH_UNIX_DTN);
    }

    /**
     * Equals function to check if another point in time is equal to this one
     * @param obj The DtnTime object that this object should be compared to
     * @return If both points in time are equal
     */
    public boolean equals(DtnTime obj) {
        return (this.toLong() == obj.toLong());
    }
}
