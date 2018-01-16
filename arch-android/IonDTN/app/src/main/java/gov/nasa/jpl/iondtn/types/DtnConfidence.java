package gov.nasa.jpl.iondtn.types;

/**
 *  The DtnConfidence class represents a confidence as percentage, i.e. 0-100
 *  percent. Although intialization can be done with more precise values,
 *  output values are always rounded to the next percent
 *
 *  @author Robert Wiewel
 */

public class DtnConfidence {
    private int confidence;

    /**
     * Constructor that allows the instantiation with a float string
     *
     * This constructor converts all values above 1.0 as 1.0 and all values
     * below 0.0 as 0.0.
     *
     * @param str string value that represents the confidence as a float in the
     *            range between 0 and 1
     */
    public DtnConfidence(String str) {
        float tempVal = Float.parseFloat(str);

        if (tempVal < 0.0) {
            this.confidence = 0;
        }
        else if (tempVal > 1.0) {
            this.confidence = 100;
        }
        else {
            this.confidence = Math.round(tempVal*100);
        }
    }

    /**
     * Constructor that allows the instantiation with a float
     *
     * This constructor converts all values above 1.0 as 1.0 and all values
     * below 0.0 as 0.0.
     *
     * @param value float value that represents the confidence in the range
     *              between 0 and 100
     */
    public DtnConfidence(float value) {
        if (value < 0.0) {
            this.confidence = 0;
        }
        else if (value > 1.0) {
            this.confidence = 100;
        }
        else {
            this.confidence = Math.round(value*100);
        }
    }

    /**
     * Constructor that allows the instantiation with an int
     *
     * This constructor converts all values above 100 as 100 and all values
     * below 0 as 0.
     *
     * @param value int value that represents the confidence in the range
     *              between 0 and 100
     */
    public DtnConfidence(int value) {
        if (value < 0) {
            this.confidence = 0;
        }
        else if (value > 100) {
            this.confidence = 100;
        }
        else {
            this.confidence = value;
        }
    }

    /**
     * Returns the confidence value as String
     * @return Confidence value as String (range: 0 - 100)
     */
    public String toString() {
        return Integer.toString(this.confidence);
    }

    /**
     * Returns the confidence value as float
     * @return Confidence value as float (range: 0.0 - 1.0)
     */
    public float toFloat() {
        return (float) (this.confidence/100.0);
    }

    /**
     * Returns the confidence value as int
     * @return Confidence value as int (range: 0 - 100)
     */
    public int toInt() {
        return this.confidence;
    }

    /**
     * Checks if this object is logically equal to another object
     * @param obj The other DtnConfidence object
     * @return boolean value if the two objects are logically equal
     */
    public boolean equals(DtnConfidence obj) {
        return (this.confidence == obj.toInt());
    }
}
