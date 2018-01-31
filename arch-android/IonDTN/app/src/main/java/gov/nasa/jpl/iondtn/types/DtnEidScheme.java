package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Object that represents one EID scheme
 *
 * @author Robert Wiewel
 */
public class DtnEidScheme implements Parcelable {
    private String identifier;
    private String forwarderCommand;
    private String adminAppCommand;
    private boolean mStatus;

    /**
     * Constructor that allows the instantiation of a DtnEidScheme object
     * @param identifier String that identifies the scheme
     * @param forwarderCmd Command that is used for forwarding of this scheme
     * @param adminAppCmd Command that is used for administration of this scheme
     */
    public DtnEidScheme(String identifier, String forwarderCmd, String
            adminAppCmd, boolean status) {
        this.identifier = identifier;
        this.forwarderCommand = forwarderCmd;
        this.adminAppCommand = adminAppCmd;
        this.mStatus = status;
    }

    /**
     * Constructor for reconstructing elements from parcels
     * @param in The parcel containing the object in a "flat" form
     */
     DtnEidScheme(Parcel in){
        String[] data = new String[3];

        in.readStringArray(data);
        // the order needs to be the same as in writeToParcel() method
        this.identifier = data[0];
        this.forwarderCommand = data[1];
        this.adminAppCommand = data[2];
        this.mStatus = in.readInt() != 0;
     }

    /**
     * Factory method for reconstruction after parcelling
     */
    public static final Parcelable.Creator CREATOR = new Parcelable.Creator() {
        public DtnEidScheme createFromParcel(Parcel in) {
            return new DtnEidScheme(in);
        }

        public DtnEidScheme[] newArray(int size) {
            return new DtnEidScheme[size];
        }
    };

    /**
     * Provides the identifier of a particular scheme object
     * @return the scheme identifier as a String
     */
    public String getSchemeID() {
        return this.identifier;
    }

    /**
     * Provides the forwarding command of a particular scheme object
     * @return the forwarding command as a String
     */
    public String getForwarderCommand() {
        return this.forwarderCommand;
    }

    /**
     * Provides the administration application command of a particular scheme
     * object
     * @return the administration application command as a String
     */
    public String getAdminAppCommand() {
        return this.adminAppCommand;
    }

    /**
     * Returns the status of the EID scheme
     * @return true if EID scheme is started, false otherwise
     */
    public boolean getStatus() {
        return this.mStatus;
    }

    /**
     * Checks if this object is logically equal to another object
     * @param obj The other DtnEidScheme object
     * @return boolean value if the two objects are logically equal
     */
    public boolean equals(DtnEidScheme obj) {
        return (this.identifier.equals(obj.getSchemeID()) &&
                this.forwarderCommand.equals(obj.getForwarderCommand()) &&
                this.adminAppCommand.equals(obj.getAdminAppCommand()));
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
        parcel.writeStringArray(new String[] {this.getSchemeID(),
                this.getForwarderCommand(),
                this.getAdminAppCommand()});
        parcel.writeInt(this.mStatus?1:0);

    }
}
