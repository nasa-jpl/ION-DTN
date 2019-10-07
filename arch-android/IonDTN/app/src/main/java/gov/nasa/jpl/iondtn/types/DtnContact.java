package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;
import android.os.Parcelable;

import java.text.ParseException;

/**
 * Object that represents one contact element in the IonDTN application
 *
 * @author Robert Wiewel
 */
public class DtnContact implements Parcelable {
    private String fromNode;
    private String toNode;
    private DtnTime fromTime;
    private DtnTime toTime;
    private DtnDataRate xmitRate;
    private DtnConfidence confidence;

    /**
     * Constructor that only takes Strings as parameters and handles
     * conversion to custom data types internally
     * @param fromNode The source node of the contact
     * @param toNode The destination node of the contact
     * @param fromTime Start time as string representing the time in the
     *                  format "yyyy/MM/dd-HH:mm:ss"
     * @param toTime End time as string representing the time in the
     *                  format "yyyy/MM/dd-HH:mm:ss"
     * @param xmitRate The data rate of the contact in bits/sec
     * @param confidence The confidence as float representation between "0.0"
     *                   and "1.0"
     * @throws ParseException When the parsing of data fails
     */
    public DtnContact(String fromNode,
                      String toNode,
                      String fromTime,
                      String toTime,
                      String xmitRate,
                      String confidence) throws ParseException{
        this.fromNode = fromNode;
        this.toNode = toNode;
        this.fromTime = new DtnTime(fromTime);
        this.toTime = new DtnTime(toTime);
        this.xmitRate = new DtnDataRate(xmitRate);
        this.confidence = new DtnConfidence(confidence);
    }

    /**
     * Constructor that only takes Strings as parameters and handles
     * conversion to custom data types internally
     * @param fromNode The source node of the contact
     * @param toNode The destination node of the contact
     * @param fromTime Start time as {@link DtnTime} object
     * @param toTime End time as {@link DtnTime} object
     * @param xmitRate The data rate of the contact in bits/sec
     * @param confidence The confidence as float representation between "0.0"
     *                   and "1.0"
     */
    public DtnContact(String fromNode,
                      String toNode,
                      DtnTime fromTime,
                      DtnTime toTime,
                      String xmitRate,
                      String confidence) {
        this.fromNode = fromNode;
        this.toNode = toNode;
        this.fromTime = fromTime;
        this.toTime = toTime;
        this.xmitRate = new DtnDataRate(xmitRate);
        this.confidence = new DtnConfidence(confidence);
    }

    /**
     * Constructor that only takes Strings as parameters and handles
     * conversion to custom data types internally
     * @param fromNode The source node of the contact
     * @param toNode The destination node of the contact
     * @param fromTime Start time as {@link DtnTime} object
     * @param toTime End time as {@link DtnTime} object
     * @param xmitRate The data rate as {@link DtnDataRate} object
     * @param confidence The confidence as {@link DtnConfidence} object
     */
    public DtnContact(String fromNode,
                      String toNode,
                      DtnTime fromTime,
                      DtnTime toTime,
                      DtnDataRate xmitRate,
                      DtnConfidence confidence) {
        this.fromNode = fromNode;
        this.toNode = toNode;
        this.fromTime = fromTime;
        this.toTime = toTime;
        this.xmitRate = xmitRate;
        this.confidence = confidence;
    }

    /**
     * Constructor for reconstructing elements from parcels
     * @param in The parcel containing the object in a "flat" form
     */
    public DtnContact(Parcel in){
        String[] data = new String[6];

        in.readStringArray(data);
        // the order needs to be the same as in writeToParcel() method
        this.fromNode = data[0];
        this.toNode = data[1];
        try {
            this.fromTime = new DtnTime(data[2]);
            this.toTime = new DtnTime(data[3]);
        }
        catch (ParseException e) {
            e.printStackTrace();
        }
        this.xmitRate = new DtnDataRate(data[4]);
        this.confidence = new DtnConfidence(data[5]);
    }

    /**
     * Factory method for reconstruction after parcelling
     */
    public static final Parcelable.Creator CREATOR = new Parcelable.Creator() {
        public DtnContact createFromParcel(Parcel in) {
            return new DtnContact(in);
        }

        public DtnContact[] newArray(int size) {
            return new DtnContact[size];
        }
    };

    /**
     * Returns the Source node EID of the contact
     * @return the source EID as String
     */
    public String getFromNode() {
        if (android.text.TextUtils.isDigitsOnly(this.fromNode)) {
            return "ipn:" + this.fromNode;
        }
        else {
            return this.fromNode;
        }
    }

    /**
     * Returns the destination EID of the contact
     * @return the destination EId as String
     */
    public String getToNode() {
        if (android.text.TextUtils.isDigitsOnly(this.toNode)) {
            return "ipn:" + this.toNode;
        }
        else {
            return this.toNode;
        }
    }

    /**
     * Returns the start time of the contact as {@link DtnTime} object
     * @return the start time as {@link DtnTime} object
     */
    public DtnTime getFromTime() {
        return this.fromTime;
    }

    /**
     * Returns the end time of the contact as {@link DtnTime} object
     * @return the end time as {@link DtnTime} object
     */
    public DtnTime getToTime() {
        return this.toTime;
    }

    /**
     * Returns the data rate as {@link DtnDataRate} object
     * @return the data rate as {@link DtnDataRate} object
     */
    public DtnDataRate getXmitRate() {
        return this.xmitRate;
    }

    /**
     * Returns the confidence as {@link DtnConfidence} object
     * @return the confidence as {@link DtnConfidence} object
     */
    public DtnConfidence getConfidence() {
        return this.confidence;
    }

    public boolean equals(DtnContact obj) {
        return (this.fromNode.equals(obj.getFromNode()) &&
                this.toNode.equals(obj.getToNode()) &&
                this.fromTime.equals(obj.getFromTime()) &&
                this.toTime.equals(obj.getToTime()) &&
                this.xmitRate.equals(obj.getXmitRate()) &&
                this.confidence.equals(obj.getConfidence()));
    }

    /**
     * Returns a textual representation of the object. Helpful for logging
     * and debugging.
     * @return All contents of the object as one combine String
     */
    public String toString() {
        return "From: " + this.fromNode + " To: " + this.toNode +
                            " FromTime: " + this.fromTime +  " ToTime: " +
                            this.toTime + " Rate: " + this.xmitRate + " " +
                            "Confidence: " + this.confidence;
    }

    /**
     * Description of the object contents, required for parcelling
     * @return 0 as identifier (not used in this context)
     */
    @Override
    public int describeContents() {
        return 0;
    }

    /**
     * Writes the objects state to a provided parcel
     * @param parcel The object that the state shall be stored in
     * @param i Flags
     */
    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeStringArray(new String[] {this.fromNode,
                this.toNode,
                this.fromTime.toString(),
                this.toTime.toString(),
                this.xmitRate.toString(),
                this.confidence.toString()});
    }
}
