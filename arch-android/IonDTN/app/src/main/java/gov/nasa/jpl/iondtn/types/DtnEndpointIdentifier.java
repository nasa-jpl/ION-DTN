package gov.nasa.jpl.iondtn.types;

import android.os.Parcel;
import android.os.Parcelable;

/**
 *  The DtnEndpointIdentifier class represents a specific unique identifier
 *  of a DTN endpoint. These identifiers are used to identify parties that
 *  are somewhat involved in the processing or transmission of a bundle.
 *
 *  An endpoint identifier is always composed of a scheme part and an unique
 *  identifier within that scheme.
 *
 *  An example for a scheme and what the structure of a resulting EID is can
 *  be found in <a href="https://tools.ietf.org/html/draft-irtf-
 *  dtnrg-dtn-uri-scheme-00">draft-irtf-dtnrg-dtn-uri-scheme-00</a>
 *
 *  @author Robert Wiewel
 */

public class DtnEndpointIdentifier implements Parcelable {
    private String identifier;
    private String script = "";
    private ReceivingBehavior behavior;

    public enum ReceivingBehavior {DISCARD, QUEUE}

    /**
     * Constructor of an DtnEndpoint
     * @param identifier The local identifier for the Endpoint
     * @param behavior The behaviour (Either discarding or queuing incoming
     *                 and not delivered bundles)
     * @param script The script that is used when bundles are received
     */
    public DtnEndpointIdentifier(String identifier, ReceivingBehavior
            behavior, String script) {
        this.behavior = behavior;
        this.identifier = identifier;
        this.script = script;
    }

    /**
     * Constructor for reconstructing elements from parcels
     * @param in The parcel containing the object in a "flat" form
     */
    public DtnEndpointIdentifier(Parcel in){
        String[] data = new String[3];

        in.readStringArray(data);
        // the order needs to be the same as in writeToParcel() method
        this.identifier = data[0];
        this.behavior = ReceivingBehavior.valueOf(data[1]);
        this.script = data[2];
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
     * Returns the scheme object of the EID
     * @return scheme object
     */
    public ReceivingBehavior getBehavior() {
        return this.behavior;
    }

    /**
     * Returns the identifier of the EID
     * @return identfifier as a String
     */
    public String getIdentifier() {
        return this.identifier;
    }

    /**
     * Returns the identifier of the EID
     * @return identfifier as a String
     */
    public String getScript() {
        return this.script;
    }

    /**
     * Provides the full URI of the EID as a String
     * @return URI consisting of -scheme-://-identifier-
     */
    @Override
    public String toString() {
        return identifier;
    }

    /**
     * Checks if this object is logically equal to another object
     * @param obj The other DtnEndpointIdentifier object
     * @return boolean value if the two objects are logically equal
     */
    public boolean equals(DtnEndpointIdentifier obj) {
        return (this.identifier.equals(obj.getIdentifier()) &&
                this.behavior.equals(obj.getBehavior()) &&
                this.script.equals(obj.getScript()));
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
        parcel.writeString(this.getBehavior().toString());
        parcel.writeString(this.getScript());
    }
}
