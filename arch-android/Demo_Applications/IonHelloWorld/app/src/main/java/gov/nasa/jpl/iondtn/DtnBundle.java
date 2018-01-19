package gov.nasa.jpl.iondtn;

import android.os.Parcel;
import android.os.ParcelFileDescriptor;
import android.os.Parcelable;

/**
 * This class represents one DtnBundle that has been received by ION-DTN and
 *   that has to be handed over to the client application
 * @author Robert Wiewel
 */

public class DtnBundle implements Parcelable {
    // Represents the source EID that the DtnBundle was received from (when
    // returned from the provider application) or the destination EID (when
    // the DtnBundle object is handed over to the provider application)
    private String eid;

    // The time that the DtnBundle was created at the source
    private long creation_time;

    // The time to live counter to evaluate the route that a DtnBundle has taken
    private int time_to_live;

    // The type that the payload is stored in
    public enum payload_type {BYTE_ARRAY, FILE_DESCRIPTOR}
    private payload_type type;

    // A file descriptor object that can hold a pointer to the payload data
    private ParcelFileDescriptor payload_fd = null;

    // A byte array that can hold the payload data
    private byte payload_byte_array[] = null;

    // The priority class of the (sending) bundle, invalid for received bundles
    public enum Priority {BULK, STANDARD, EXPEDITED, INVALID}
    private Priority prio;

    protected DtnBundle(Parcel in) {
        eid = in.readString();
        creation_time = in.readLong();
        time_to_live = in.readInt();
        type = payload_type.valueOf(in.readString());
        prio = Priority.valueOf(in.readString());

        if (type == payload_type.FILE_DESCRIPTOR) {
            payload_fd = in.readParcelable(ParcelFileDescriptor.class
                    .getClassLoader());
        }
        else {
            payload_byte_array = in.createByteArray();
        }
    }

    /**
     * The creator operation in order to create parcelable DtnBundles
     */
    public static final Creator<DtnBundle> CREATOR = new Creator<DtnBundle>() {
        @Override
        public DtnBundle createFromParcel(Parcel in) {
            return new DtnBundle(in);
        }

        @Override
        public DtnBundle[] newArray(int size) {
            return new DtnBundle[size];
        }
    };

    /**
     * Describes the contents of the object, can be omitted
     * @return The contents of the object, is simply ignored
     */
    @Override
    public int describeContents() {
        return 0;
    }

    /**
     * Write the contents of the object to a parcel that can be transferred
     * to other applications
     * @param parcel The transferable object
     * @param i The flags
     */
    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeString(this.eid);
        parcel.writeLong(creation_time);
        parcel.writeInt(time_to_live);
        parcel.writeString(type.toString());
        parcel.writeString(prio.toString());
        if (type == payload_type.FILE_DESCRIPTOR) {
            parcel.writeParcelable(payload_fd, i);
        }
        else {
            parcel.writeByteArray(payload_byte_array);
        }
    }

    /**
     * Empty constructor, required for filling by JNI
     */
    DtnBundle() {
        this.prio = Priority.INVALID;
    }

    /**
     * Constructor that allows the instantiation of a DtnBundle with a
     * {@link ParcelFileDescriptor}.
     * @param eid The EID
     * @param creation_time The time that the DtnBundle was created. Is ignored
     *                      when DtnBundle is sent.
     * @param time_to_live The time-to-live value that the DtnBundle had when
     *                     being received by ION-DTN.
     * @param priority The priority class of the DtnBundle (Bulk, Standard,
     *                 Expedited)
     * @param payload_fd The {@link ParcelFileDescriptor} pointing to the
     *                   payload of the DtnBundle.
     */
    public DtnBundle(String eid,
                     long creation_time,
                     int time_to_live,
                     Priority priority,
                     ParcelFileDescriptor payload_fd) {
        // Assign all internal values according to the constructor values
        this.eid = eid;
        this.creation_time = creation_time;
        this.time_to_live = time_to_live;
        this.type = payload_type.FILE_DESCRIPTOR;
        this.prio = priority;
        this.payload_fd = payload_fd;
    }

    /**
     * Constructor that allows the instantiation of a DtnBundle with a ByteArray.
     * @param eid The source EID
     * @param creation_time The time that the DtnBundle was created. Is ignored
     *                      when DtnBundle is sent.
     * @param time_to_live The time-to-live value that the DtnBundle had when
     *                     being received by ION-DTN
     * @param priority The priority class of the DtnBundle (Bulk, Standard,
     *                 Expedited)
     * @param payload_byte_array The raw payload as byte array.
     */
    public DtnBundle(String eid,
                     long creation_time,
                     int time_to_live,
                     Priority priority,
                     byte[] payload_byte_array) {
        // Assign all internal values according to the constructor values
        this.eid = eid;
        this.creation_time = creation_time;
        this.time_to_live = time_to_live;
        this.type = payload_type.BYTE_ARRAY;
        this.prio = priority;
        this.payload_byte_array = payload_byte_array;
    }

    /**
     * Get function of the EID
     * @return The source EID of the DtnBundle
     */
    public String getEID() {
        return eid;
    }

    /**
     * Get the creation time of the DtnBundle
     * @return the creation time of the DtnBundle measured in seconds since the
     * DTN epoch
     */
    public long getCreationTime() {
        return creation_time;
    }

    /**
     * Get the time-to-live hop count of a received DtnBundle
     * @return the time-to-live count measured in taken hops
     */
    public int getTimeToLive() {
        return time_to_live;
    }

    /**
     * Get the type of the payload (i.e. either file descriptor or byte array)
     * @return the type of the payload as enum of type {@link payload_type}
     */
    public payload_type getPayloadType() {
        return type;
    }

    /**
     * Returns a FileDescriptor pointing to the payload of the DtnBundle. If the
     * {@link payload_type} of the DtnBundle is byte array, this function
     * returns null.
     * @return the ParcelFileDescriptor pointing to the payload (might be null).
     */
    public ParcelFileDescriptor getPayloadFD() {
        return payload_fd;
    }

    /**
     * Returns the payload of the DtnBundle as byte array. If the
     * {@link payload_type} of the DtnBundle is file descriptor, this function
     * returns null.
     * @return the Byte Array (might be null).
     */
    public byte[] getPayloadByteArray(){
        return payload_byte_array;
    }

    /**
     * Returns the priority of the DtnBundles, invalid for received bundles
     * @return the priority of the DtnBundle (Bulk, Standard, Expedited)
     */
    public Priority getPriority() {
        return this.prio;
    }

    /**
     * Sets the source/destination EID of the DtnBundle
     * @param eid The EID of the DtnBundle
     */
    public void setEID(String eid) {
        this.eid = eid;
    }

    /**
     * Sets the creation time of a DtnBundle
     * @param creation_time The creation time as seconds since the DTN epoch
     */
    public void setCreationTime(long creation_time) {
        this.creation_time = creation_time;
    }

    /**
     * Sets the hop count of the DtnBundle
     * @param time_to_live the hop count
     */
    public void setTimeToLive(int time_to_live) {
        this.time_to_live = time_to_live;
    }

    /**
     * Assigns a byte array payload to the DtnBundle. Also updates the type of
     * the DtnBundle and invalidates the old payload type.
     * @param payload_byte_array The newly assigned byte array payload
     */
    public void setPayloadByteArray(byte[] payload_byte_array) {
        this.type = payload_type.BYTE_ARRAY;
        this.payload_fd = null;
        this.payload_byte_array = payload_byte_array;
    }

    /**
     * Assigns a new priority class value to the DtnBundle
     * @param priority The priority class (Bulk, Standard, Expedited)
     */
    public void setPriority(Priority priority) {
        this.prio = priority;
    }

    /**
     * Assigns a file descriptor as payload to the DtnBundle. Also updates the
     * type of the DtnBundle and invalidates the old payload type.
     * @param payload_fd The newly assigned {@link ParcelFileDescriptor}
     *                   payload.
     */
    public void setPayloadFD(ParcelFileDescriptor payload_fd) {
        this.type = payload_type.FILE_DESCRIPTOR;
        this.payload_byte_array = null;
        this.payload_fd = payload_fd;
    }

    /**
     * Assigns a file descriptor as payload to the DtnBundle. Also updates the
     * type of the DtnBundle and invalidates the old payload type.
     */
    public void setPayloadFDType() {
        this.type = payload_type.FILE_DESCRIPTOR;
        this.payload_byte_array = null;
    }
}
