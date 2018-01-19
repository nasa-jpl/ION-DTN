package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;
import android.os.Parcelable;

import java.text.ParseException;

/**
 * Object that represents one Security BAB Rule
 *
 * @author Robert Wiewel
 */
public class DtnBabRule implements Parcelable {
    private String senderEID;
    private String receiverEID;
    private String ciphersuiteName;
    private String keyName;

    /**
     * Constructor that takes only strings
     * @param senderEID The sender EID that this rule shall be applied to
     * @param receiverEID The receiver EID that this rules shall be applied to
     * @param ciphersuiteName The name of the used Ciphersuite
     * @param keyName The name of the (already registered) cryptographic key
     */
    public DtnBabRule(String senderEID, String receiverEID, String
            ciphersuiteName, String keyName) {
        this.senderEID = senderEID;
        this.receiverEID = receiverEID;
        this.ciphersuiteName = ciphersuiteName;
        this.keyName = keyName;

        // replace all "~" with "*" in srcEID and dstEID
        this.senderEID = this.senderEID.replace("~","*");
        this.receiverEID = this.receiverEID.replace("~","*");
    }

    /**
     * Constructor for reconstructing element from parcel
     * @param in The parcel containing the object in a "flat" form
     */
    public DtnBabRule(Parcel in){
        String[] data = new String[4];

        in.readStringArray(data);
        // the order needs to be the same as in writeToParcel() method
        this.senderEID = data[0];
        this.receiverEID = data[1];
        this.ciphersuiteName = data[2];
        this.keyName = data[3];
    }

    /**
     * Factory method for reconstruction after parcelling
     */
    public static final Parcelable.Creator CREATOR = new Parcelable.Creator() {
        public DtnBabRule createFromParcel(Parcel in) {
            return new DtnBabRule(in);
        }

        public DtnBabRule[] newArray(int size) {
            return new DtnBabRule[size];
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
     * Compares two {@link DtnBabRule} objects and checks for logical equality
     * @param obj the second {@link DtnBabRule} that should be compared to
     * @return true if logically equal, false otherwise
     */
    public boolean equals(DtnBabRule obj) {
        return (this.senderEID.equals(obj.getSenderEID()) &&
                this.receiverEID.equals(obj.getReceiverEID()) &&
                this.ciphersuiteName.equals(obj.getCiphersuiteName()) &&
                this.keyName.equals(obj.getKeyName()));
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
                this.getKeyName()});

    }
}
