package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;
import android.os.Parcelable;

import java.text.ParseException;
import java.util.Date;

/**
 * Object that represents a Range in the IonDTN scope
 *
 * @author Robert Wiewel
 */
public class DtnRange implements Parcelable {
    private String fromNode;
    private String toNode;
    private DtnTime fromTime;
    private DtnTime toTime;
    private int owlt;

    /**
     * Constructor that creates a {@link DtnRange} object
     * @param fromNode The source node that the range applies to
     * @param toNode The destination node that the range applies to
     * @param fromTime The start time that the range becomes valid
     * @param toTime The end time after which the range is invalid
     * @param owlt The data transfer delay on this range (one-way light time)
     *             in seconds
     */
    public DtnRange(String fromNode,
                    String toNode,
                    DtnTime fromTime,
                    DtnTime toTime,
                    int owlt) {
        this.fromNode = fromNode;
        this.toNode = toNode;
        this.fromTime = fromTime;
        this.toTime = toTime;
        this.owlt = owlt;
    }

    /**
     * Constructor for reconstructing elements from parcels
     * @param in The parcel containing the object in a "flat" form
     */
    public DtnRange(Parcel in){
        String[] data = new String[4];

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
        this.owlt = in.readInt();
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
     * Returns the source node that the range applies to
     * @return the source node EID as string
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
     * Returns the destination node that the range applies to
     * @return the destination node EID as string
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
     * Returns the time that the range description becomes valid
     * @return the start time as {@link DtnTime} object
     */
    public DtnTime getFromTime() {
        return this.fromTime;
    }

    /**
     * Returns the time that the range description becomes invalid
     * @return the end time as {@link DtnTime} object
     */
    public DtnTime getToTime() {
        return this.toTime;
    }

    /**
     * Returns the One-Way Light Time delay of the range
     * @return the One-Way Light Time (OWLT) in seconds as string
     */
    public int getOwlt() {
        return this.owlt;
    }

    public boolean equals(DtnRange obj) {
        return (this.fromNode.equals(obj.getFromNode()) &&
                this.toNode.equals(obj.getToNode()) &&
                this.fromTime.equals(obj.getFromTime()) &&
                this.toTime.equals(obj.getToTime()) &&
                this.owlt == obj.getOwlt());
    }

    /**
     * Returns a textual representation of the range object. Helpful for
     * logging and debugging
     * @return one string containing all information that the object holds
     */
    public String toString() {
        return "From: " + this.fromNode + " To: " + this.toNode +
                " FromTime: " + this.fromTime +  " ToTime: " +
                this.toTime + " Owlt: " + this.owlt;
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
                this.toTime.toString()});

        parcel.writeInt(this.owlt);
    }
}
