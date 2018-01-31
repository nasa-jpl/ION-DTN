package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Object that represents one cryptographic key (does only contain metadata
 * and not the actual key)
 *
 * @author Robert Wiewel
 */
public class DtnKey implements Parcelable {
    private String keyName;
    private String keyLength;

    /**
     * Constructor of a {@link DtnKey} object
     * @param keyName The name of the key
     * @param keyLength The length of the actual key
     */
    public DtnKey(String keyName, String keyLength) {
        this.keyName = keyName;
        this.keyLength = keyLength;
    }

    /**
     * Constructor for reconstructing elements from parcels
     * @param in The parcel containing the object in a "flat" form
     */
    public DtnKey(Parcel in){
        String[] data = new String[2];

        in.readStringArray(data);
        // the order needs to be the same as in writeToParcel() method
        this.keyName = data[0];
        this.keyLength = data[1];
    }

    /**
     * Factory method for reconstruction after parcelling
     */
    public static final Creator CREATOR = new Creator() {
        public DtnKey createFromParcel(Parcel in) {
            return new DtnKey(in);
        }

        public DtnKey[] newArray(int size) {
            return new DtnKey[size];
        }
    };

    /**
     * Returns the length of the key
     * @return the length of the key as String (in integer representation)
     */
    public String getKeyLength() {
        return this.keyLength;
    }

    /**
     * Returns the name of the key
     * @return the name of the key as String
     */
    public String getKeyName() {
        return this.keyName;
    }

    /**
     * Compares two {@link DtnKey} objects and checks for logical equality
     * @param obj the second {@link DtnKey} that should be compared to
     * @return true if logically equal, false otherwise
     */
    public boolean equals(DtnKey obj) {
        return (this.keyLength.equals(obj.getKeyLength()) &&
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
        parcel.writeStringArray(new String[] {this.getKeyName(),
                this.getKeyLength()});

    }
}
