package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;
import android.os.Parcelable;

import java.text.ParseException;

/**
 * Object that represents one protocol
 *
 * @author Robert Wiewel
 */
public class DtnProtocol implements Parcelable {
    private String identifier;
    private int payloadFrameSize;
    private int overheadFrameSize;
    private int protocolClass;

    /**
     * Constructor of a {@link DtnProtocol} object
     * @param identifier The identifier of the object
     * @param payloadFrameSize The frame size of the protocol (in bytes)
     * @param overheadFrameSize The overhead size of the protocol (in bytes)
     * @param protocolClass The protocol class (see ION-DTN documentation for
     *                      details, most of the time 8)
     */
    public DtnProtocol(String identifier, String payloadFrameSize, String
            overheadFrameSize, String protocolClass) {
        this.identifier = identifier;
        this.payloadFrameSize = Integer.parseInt(payloadFrameSize);
        this.overheadFrameSize = Integer.parseInt(overheadFrameSize);
        this.protocolClass = Integer.parseInt(protocolClass);
    }

    /**
     * Constructor for reconstructing elements from parcels
     * @param in The parcel containing the object in a "flat" form
     */
    public DtnProtocol(Parcel in){
        String[] data = new String[3];

        in.readStringArray(data);
        // the order needs to be the same as in writeToParcel() method
        this.identifier = in.readString();
        this.payloadFrameSize = in.readInt();
        this.overheadFrameSize = in.readInt();
        this.protocolClass = in.readInt();
    }

    /**
     * Factory method for reconstruction after parcelling
     */
    public static final Parcelable.Creator CREATOR = new Parcelable.Creator() {
        public DtnProtocol createFromParcel(Parcel in) {
            return new DtnProtocol(in);
        }

        public DtnProtocol[] newArray(int size) {
            return new DtnProtocol[size];
        }
    };

    /**
     * Provides the identifier of a particular protocol object
     * @return the protocol identifier as a String
     */
    public String getIdentifier() {
        return this.identifier;
    }

    /**
     * Returns the payload frame size
     * @return the payload frame size in bytes as integer
     */
    public int getPayloadFrameSize() {
        return this.payloadFrameSize;
    }

    /**
     * Returns the overhead frame size
     * @return the overhead frame size in bytes as integer
     */
    public int getOverheadFrameSize() {
        return this.overheadFrameSize;
    }

    /**
     * Returns the protocol class
     * @return the protocol class as integer (see ION-DTN documentation for
     * details on the protocol classes)
     */
    public int getProtocolClass() {
        return this.protocolClass;
    }

    /**
     * Compares two {@link DtnProtocol} objects and checks for logical equality
     * @param obj the second {@link DtnProtocol} that should be compared to
     * @return true if logically equal, false otherwise
     */
    public boolean equals(DtnProtocol obj) {
        return (this.identifier == obj.getIdentifier() &&
                this.overheadFrameSize == obj.getOverheadFrameSize() &&
                this.payloadFrameSize == obj.getPayloadFrameSize() &&
                this.protocolClass == obj.getProtocolClass());
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
        parcel.writeString(this.getIdentifier());
        parcel.writeInt(this.getPayloadFrameSize());
        parcel.writeInt(this.getOverheadFrameSize());
        parcel.writeInt(this.getProtocolClass());
    }
}
