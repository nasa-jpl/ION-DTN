package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Object that represents one DTN induct or outduct
 *
 * @author Robert Wiewel
 */
public class DtnInOutduct implements Parcelable {
    public enum IOType {INDUCT, OUTDUCT};

    private IOType mType;
    private String mProtocolName;
    private String mDuctName;
    private String mCmd;
    private int maxPayloadLength;
    private boolean mStatus;

    /**
     * Constructor of one DtnInOutduct object
     * @param type The type of the object (can either be induct or outduct)
     * @param protocolName The name of the protocol that is used for this in-
     *                     or outduct
     * @param ductName The name of the inoutduct
     * @param cmd The command used for the inoutduct
     * @param status The status of the inoutduct (active/started of
     *               inactive/stopped) as boolean
     */
    public DtnInOutduct(IOType type, String protocolName, String ductName,
                        String cmd, boolean status) {
        mType = type;
        mProtocolName = protocolName;
        mDuctName = ductName;
        mCmd = cmd;
        maxPayloadLength = 0;
        mStatus = status;
    }

    /**
     * Constructor of one DtnInOutduct object
     * @param type The type of the object (can either be induct or outduct)
     * @param protocolName The name of the protocol that is used for this in-
     *                     or outduct
     * @param ductName The name of the inoutduct
     * @param cmd The command used for the inoutduct
     * @param maxPayloadLength The maximum acceptable payload length (when
     *                         used as outduct)
     * @param status The status of the inoutduct (active/started of
     *               inactive/stopped) as boolean
     */
    public DtnInOutduct(IOType type, String protocolName, String ductName,
                        String cmd, int maxPayloadLength, boolean status) {
        mType = type;
        mProtocolName = protocolName;
        mDuctName = ductName;
        mCmd = cmd;
        this.maxPayloadLength = maxPayloadLength;
        mStatus = status;
    }

    /**
     * Constructor for reconstructing elements from parcels
     * @param in The parcel containing the object in a "flat" form
     */
    public DtnInOutduct(Parcel in){
        String[] data = new String[3];

        in.readStringArray(data);
        // the order needs to be the same as in writeToParcel() method
        this.mType = IOType.valueOf(in.readString());
        this.mDuctName = in.readString();
        this.mProtocolName = in.readString();
        this.mCmd = in.readString();
        this.maxPayloadLength = in.readInt();
        this.mStatus = in.readInt() != 0;
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
     * Returns the Type of the InOutduct object
     * @return The type as {@link IOType} object
     */
    public IOType getType() {
        return mType;
    }

    /**
     * Returns the protocol name
     * @return the name of the Protocol as string
     */
    public String getProtocolName() {
        return mProtocolName;
    }

    /**
     * Returns the name of the In-/Outduct
     * @return the name as String
     */
    public String getDuctName() {
        return mDuctName;
    }

    /**
     * Returns the command of the In-/Outduct
     * @return the commmand as String
     */
    public String getCmd() {
        return mCmd;
    }

    /**
     * Returns the maximum payload length for an Outduct
     * @return the maximum payload length as Integer
     */
    public int getMaxPayloadLength() {
        return maxPayloadLength;
    }

    /**
     * Returns the current status of the In/Outduct object
     * @return the Status as boolean (true if started, false otherwise)
     */
    public boolean getStatus() {
        return mStatus;
    }

    /**
     * Compares two {@link DtnInOutduct} objects and checks for logical equality
     * @param obj the second {@link DtnInOutduct} that should be compared to
     * @return true if logically equal, false otherwise
     */
    public boolean equals(DtnInOutduct obj) {
        return this.mType.equals(obj.getType()) &&
                this.mProtocolName.equals(obj.getProtocolName()) &&
                this.mDuctName.equals(obj.getDuctName()) &&
                this.mCmd.equals(obj.getCmd()) &&
                this.maxPayloadLength == obj.getMaxPayloadLength() &&
                this.mStatus == obj.getStatus();
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
        parcel.writeString(this.mType.toString());
        parcel.writeString(this.getDuctName());
        parcel.writeString(this.getProtocolName());
        parcel.writeString(this.getCmd());
        parcel.writeInt(this.maxPayloadLength);
        parcel.writeInt(this.mStatus?1:0);
    }
}
