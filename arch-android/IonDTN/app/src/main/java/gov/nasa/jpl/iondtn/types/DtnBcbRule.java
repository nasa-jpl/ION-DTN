package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Object that represents one Security BCB Rule
 *
 * @author Robert Wiewel
 */
public class DtnBcbRule implements Parcelable {
    private String senderEID;
    private String receiverEID;
    private String ciphersuiteName;
    private String keyName;
    private String blockTypeNumber;

    /**
     * Constructor that takes only Strings as parameters
     * @param senderEID The sender EID that this rule shall be applied to
     * @param receiverEID The receiver EID that this rule shall be applied to
     * @param ciphersuiteName The name of the used Ciphersuite
     * @param keyName The name of the (already registered) cryptographic key
     * @param blockTypeNumber The blocktype number that this rule shall be
     *                        applied to
     */
    public DtnBcbRule(String senderEID, String receiverEID, String
            ciphersuiteName, String keyName, String blockTypeNumber) {
        this.senderEID = senderEID;
        this.receiverEID = receiverEID;
        this.ciphersuiteName = ciphersuiteName;
        this.keyName = keyName;
        this.blockTypeNumber = blockTypeNumber;

        // replace all "~" with "*" in srcEID and dstEID
        this.senderEID = this.senderEID.replace("~","*");
        this.receiverEID = this.receiverEID.replace("~","*");
    }

    /**
     * Constructor for reconstructing elements from parcels
     * @param in The parcel containing the object in a "flat" form
     */
    public DtnBcbRule(Parcel in){
        String[] data = new String[5];

        in.readStringArray(data);
        // the order needs to be the same as in writeToParcel() method
        this.senderEID = data[0];
        this.receiverEID = data[1];
        this.ciphersuiteName = data[2];
        this.keyName = data[3];
        this.blockTypeNumber = data[4];
    }

    /**
     * Factory method for reconstruction after parcelling
     */
    public static final Creator CREATOR = new Creator() {
        public DtnBcbRule createFromParcel(Parcel in) {
            return new DtnBcbRule(in);
        }

        public DtnBcbRule[] newArray(int size) {
            return new DtnBcbRule[size];
        }
    };

    /**
     * Returns the Sender EID that this rule applies to
     * @return the Sender EID as String
     */
    public String getSenderEID() {
        return this.senderEID;
    }

    /**
     * Returns the Receiver EID that this rule applies to
     * @return the Receiver EID as String
     */
    public String getReceiverEID() {
        return this.receiverEID;
    }

    /**
     * Returns the name of the used Ciphersuite
     * @return the Ciphersuite name as String
     */
    public String getCiphersuiteName() {
        return this.ciphersuiteName;
    }

    /**
     * Returns the name of the key
     * @return the name of the key as a string
     */
    public String getKeyName() {
        return this.keyName;
    }

    /**
     * Returns the blockType number
     * @return the blockType number as string
     */
    public String getBlockTypeNumber() {
        return this.blockTypeNumber;
    }

    /**
     * Compares two {@link DtnBcbRule} objects and checks for logical equality
     * @param obj the second {@link DtnBcbRule} that should be compared to
     * @return true if logically equal, false otherwise
     */
    public boolean equals(DtnBcbRule obj) {
        return (this.senderEID.equals(obj.getSenderEID()) &&
                this.receiverEID.equals(obj.getReceiverEID()) &&
                this.ciphersuiteName.equals(obj.getCiphersuiteName()) &&
                this.keyName.equals(obj.getKeyName()) &&
                this.blockTypeNumber.equals(obj.getBlockTypeNumber()));
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
        parcel.writeStringArray(new String[] {this.getSenderEID(),
                this.getReceiverEID(),
                this.getCiphersuiteName(),
                this.getKeyName(),
                this.getBlockTypeNumber()});

    }
}
