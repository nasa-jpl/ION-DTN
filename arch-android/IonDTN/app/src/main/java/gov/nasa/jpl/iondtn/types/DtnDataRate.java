package gov.nasa.jpl.iondtn.types;

/**
 *  The DtnDataRate class represents a data rate in the DTN space. It is
 *  internally represented as an int value in bytes per second
 *
 *  @author Robert Wiewel
 */

public class DtnDataRate {
    private int dataRate;

    /**
     * Constructor that allows instantiation from a numeric string
     * @param str String representing the data rate in bytes per second
     */
    DtnDataRate(String str) {
        this.dataRate = Integer.parseInt(str);
    }

    /**
     * Constructor that allows the instantiation from an integer value
     * @param value Integer representing the data rate in bytes per second
     */
    DtnDataRate(int value) {
        this.dataRate = value;
    }

    /**
     * Provides the contained data rate as string
     * @return String representing the data rate in bytes per second
     */
    public String toString(){
        return Integer.toString(this.dataRate);
    }

    /**
     * Provides the contained data rate as integer
     * @return Integer representing the data rate in bytes per second
     */
    public int toInt(){
        return this.dataRate;
    }

    /**
     * Equals function to check if another data rate is equal to this one
     * @param obj The DtnDataRate object that this object should be compared to
     * @return If both data rates are equal
     */
    public boolean equals(DtnDataRate obj) {
        return (obj.toInt() == this.dataRate);
    }
}
