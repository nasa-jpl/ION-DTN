package it.unibo.dtn.JAL;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.time.LocalDateTime;
import java.time.ZoneOffset;
import java.time.ZonedDateTime;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;

/** 
 * <p>
 * This class represents a Bundle.
 * </p>
 * 
 * <p>
 * The bundles are used to implement a connectionless bundle delivery services according to RFC 4838. Multiple bundles sent from one machine to another might be routed differently, and might arrive in any order. Bundle delivery is not guaranteed.
 * </p>
 * 
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public final class Bundle {

	private static final ZonedDateTime baseDateTime = ZonedDateTime.of(LocalDateTime.of(2000, 01, 01, 00, 00), ZoneOffset.UTC);
	
	// *** BUNDLE ID
	private BundleEID source = null;
	private BundleTimestamp creationTimestamp = null;
	private int fragmentOffset = 0;
	private int origLength = 0;
	
	// *** BUNDLE SPEC
	private BundleEID destination = null;
	private BundleEID replyTo = null;
	private BundlePriority priority = null;
	private List<BundleDeliveryOption> deliveryOptions = new LinkedList<>();
	private int expiration = 0;
	private int deliveryRegID = 0;
	private final List<BundleExtensionBlock> blocks = new LinkedList<>();
	private final List<BundleMetadataBlock> metadata = new LinkedList<>();
	private boolean unreliable = false;
	private boolean critical = false;
	private int flowLabel = 0;
	
	private StatusReport statusReport = null;
	
	// *** BUNDLE PAYLOAD
	private BundlePayload payload = null;
	
	/**
	 * Creates a new bundle
	 * @param destination - the destination endpointID
	 * @throws IllegalArgumentException In case the destination is null
	 */
	public Bundle(BundleEID destination) throws IllegalArgumentException {
		this();
		if (destination == null)
			throw new IllegalArgumentException("Destination can't be null.");
		this.setDestination(destination);
	}
	
	/**
	 * Creates a new bundle
	 * @param destination - the destination endpointID
	 * @param expiration - expiration in seconds
	 * @throws IllegalArgumentException In case the destination is null
	 */
	public Bundle(BundleEID destination, int expiration) throws IllegalArgumentException {
		this(destination);
		this.setExpiration(expiration);
	}
	
	/**
	 * Creates an empty bundle (used in {@link BPSocket#receive()})
	 * @see BPSocket#receive()
	 */
	Bundle() {
		this.deliveryOptions.add(BundleDeliveryOption.None);
		this.creationTimestamp = new BundleTimestamp(0, 0);
		this.priority = new BundlePriority(0);
		this.expiration = 60;
	}

	/**
	 * Returns the status report
	 * @return The status report
	 */
	public StatusReport getStatusReport() {
		return this.statusReport;
	}
	
	/**
	 * Returns the data inside the payload or null if there is no payload
	 * @return The data inside the payload or null if there is no payload
	 * @see BundlePayload#getData()
	 */
	public byte[] getData() {
		if (this.payload == null)
			return null;
		else
			return this.payload.getData();
	}
	
	/**
	 * Sets the payload with the data passed
	 * @param data the data to be put in payload
	 * @return this bundle
	 */
	public Bundle setData(byte[] data) {
		this.payload = BundlePayload.of(data);
		return this;
	}
	
	/**
	 * Returns the input stream of data inside the payload or null if there is no payload
	 * @return The input stream of data inside the payload or null if there is no payload
	 * @see BundlePayload#getInputStream()
	 */
	public InputStream getInputStream() {
		if (this.payload == null)
			return null;
		else
			return this.payload.getInputStream();
	}

	/**
	 * Returns the buffered reader of data inside the payload or null if there is no payload
	 * @return The buffered reader of data inside the payload or null if there is no payload
	 * @see BundlePayload#getBufferedReader()
	 */
	public BufferedReader getBufferedReader() {
		if (this.payload == null)
			return null;
		else 
			return this.payload.getBufferedReader();
	}

	/**
	 * Returns the input stream reader of data inside the payload or null if there is no payload
	 * @return The input stream reader of data inside the payload or null if there is no payload
	 * @see BundlePayload#getInputStreamReader()
	 */
	public InputStreamReader getInputStreamReader() {
		if (this.payload == null)
			return null;
		else 
			return this.payload.getInputStreamReader();
	}
	
	/**
	 * Returns the source to which this bundle is being sent or from which the bundle was received. For sending, if omitted, will be set {@link BPSocket#getLocalEID()} when calling {@link BPSocket#send(Bundle)} 
	 * @return The source of the bundle
	 */
	public BundleEID getSource() {
		return this.source;
	}

	/**
	 * Returns the creation timestamp.
	 * @return the creation timestamp
	 */
	public BundleTimestamp getCreationTimestamp() {
		return creationTimestamp;
	}
	
	/**
	 * Returns the creation date time in UTC time.
	 * @return The creation date time in UTC time
	 */
	public ZonedDateTime getCreationDateTime() {
		return baseDateTime.plusSeconds(this.creationTimestamp.getSeconds());
	}

	/**
	 * Returns the fragment offset
	 * @return the fragment offset
	 */
	public int getFragmentOffset() {
		return fragmentOffset;
	}

	/**
	 * Returns the orig lenght
	 * @return the orig lenght
	 */
	public int getOrigLength() {
		return origLength;
	}

	/**
	 * Returns the destination to which this bundle is being sent
	 * @return the destination
	 */
	public BundleEID getDestination() {
		return destination;
	}

	/**
	 * Returns the reply-to destination to which this bundle is being sent
	 * @return the reply-to destination
	 */
	public BundleEID getReplyTo() {
		return replyTo;
	}

	/**
	 * Returns the priority
	 * @return the priority
	 */
	public BundlePriority getPriority() {
		return priority;
	}

	/**
	 * Returns the delivery options. Use the delivery options only for requesting options on sending bundles. 
	 * @return the delivery options
	 */
	public List<BundleDeliveryOption> getDeliveryOptions() {
		return new LinkedList<>(this.deliveryOptions);
	}

	/**
	 * Returns the bundle expiration (in seconds)
	 * @return the bundle expiration
	 */
	public int getExpiration() {
		return expiration;
	}

	/**
	 * Returns the delivery RegID
	 * @return the delivery RegID
	 */
	public int getDeliveryRegID() {
		return deliveryRegID;
	}

	/**
	 * Returns the extension blocks
	 * @return the extension blocks
	 */
	public BundleExtensionBlock[] getBlocks() {
		return this.blocks.toArray(new BundleExtensionBlock[this.blocks.size()]);
	}

	/**
	 * Returns the metadata blocks
	 * @return the metadata blocks
	 */
	public BundleExtensionBlock[] getMetadata() {
		return this.metadata.toArray(new BundleExtensionBlock[this.metadata.size()]);
	}

	/**
	 * Returns if the bundle is or not unreliable
	 * @return if the bundle is or not unreliable
	 */
	public boolean isUnreliable() {
		return unreliable;
	}

	/**
	 * Returns if the bundle is or not critical
	 * @return if the bundle is or not critical
	 */
	public boolean isCritical() {
		return critical;
	}

	/**
	 * Returns the flow label
	 * @return the flow label
	 */
	public int getFlowLabel() {
		return flowLabel;
	}

	/**
	 * Returns the payload
	 * @return the payload
	 */
	public BundlePayload getPayload() {
		return payload;
	}

	/**
	 * Sets the source to which this bundle is being sent or from which the bundle was received. For sending, if omitted, will be set {@link BPSocket#getLocalEID()} when calling {@link BPSocket#send(Bundle)}
	 * @param source - the source. For sending, if omitted, will be set {@link BPSocket#getLocalEID()} when calling {@link BPSocket#send(Bundle)}
	 * @return this bundle
	 */
	public Bundle setSource(BundleEID source) {
		this.source = source;
		return this;
	}

	/**
	 * Sets the fragment offset
	 * @param fragmentOffset - the fragment offset
	 * @return this bundle
	 */
	public Bundle setFragmentOffset(int fragmentOffset) {
		this.fragmentOffset = fragmentOffset;
		return this;
	}

	/**
	 * Sets the orig lenght
	 * @param origLength - the orig lenght
	 * @return this bundle
	 */
	public Bundle setOrigLength(int origLength) {
		this.origLength = origLength;
		return this;
	}

	/**
	 * Sets the destination to which this bundle is being sent
	 * @param destination - the destination
	 * @return this bundle
	 */
	public Bundle setDestination(BundleEID destination) {
		this.destination =  destination;
		return this;
	}

	/**
	 * Sets the reply-to destination to which this bundle is being sent
	 * @param replyTo - the reply-to destination
	 * @return this bundle
	 */
	public Bundle setReplyTo(BundleEID replyTo) {
		this.replyTo = replyTo;
		return this;
	}

	/**
	 * Sets the priority
	 * @param priority - the priority
	 * @return this bundle
	 */
	public Bundle setPriority(BundlePriority priority) {
		this.priority = priority;
		return this;
	}

	/**
	 * Sets and changes the current delivery options with the
	 * @param deliveryOptions - the new delivery options
	 * @return this bundle
	 * @throws NullPointerException if the specified collection is null
	 */
	public Bundle setDeliveryOptions(Collection<BundleDeliveryOption> deliveryOptions) throws NullPointerException {
		this.deliveryOptions = new LinkedList<>(deliveryOptions);
		return this;
	}
	
	/**
	 * Adds a new delivery option
	 * @param deliveryOption - the delivery options
	 * @return this bundle
	 * @throws NullPointerException if the delivery option is null
	 */
	public Bundle addDeliveryOption(BundleDeliveryOption deliveryOption) throws NullPointerException {
		if (this.deliveryOptions == null)
			throw new NullPointerException();
		this.deliveryOptions.add(deliveryOption);
		return this;
	}
	
	/**
	 * Removes a delivery option
	 * @param deliveryOption - the delivery option
	 * @return If the delivery option was removed or not
	 * @throws NullPointerException if the delivery option is null
	 */
	public boolean removeDeliveryOption(BundleDeliveryOption deliveryOption) throws NullPointerException {
		if (deliveryOption == null)
			throw new NullPointerException();
		return this.deliveryOptions.remove(deliveryOption);
	}
	
	/**
	 * Clears all the delivery options
	 */
	public void clearAllDeliveryOption() {
		this.deliveryOptions.clear();
	}

	/**
	 * Sets the expiration (in seconds)
	 * @param expiration - the expiration
	 * @return this bundle
	 */
	public Bundle setExpiration(int expiration) {
		this.expiration = expiration;
		return this;
	}

	/**
	 * Sets the delivery RegID
	 * @param deliveryRegID - the delivery RegID
	 * @return this bundle
	 */
	public Bundle setDeliveryRegID(int deliveryRegID) {
		this.deliveryRegID = deliveryRegID;
		return this;
	}

	/**
	 * Adds a new extension block
	 * @param block - the block
	 * @return this bundle
	 * @throws NullPointerException if the block is null
	 */
	public Bundle addBlock(BundleExtensionBlock block) throws NullPointerException {
		if (block == null)
			throw new NullPointerException();
		this.blocks.add(block);
		return this;
	}

	/**
	 * Adds a new metadata block
	 * @param metadata - the metadata block
	 * @return this bundle
	 * @throws NullPointerException if metadaa is null
	 */
	public Bundle addMetadata(BundleMetadataBlock metadata) throws NullPointerException {
		if (metadata == null)
			throw new NullPointerException();
		this.metadata.add(metadata);
		return this;
	}

	/**
	 * Sets the unreliable option
	 * @param unreliable - the unreliable
	 * @return this bundle
	 */
	public Bundle setUnreliable(boolean unreliable) {
		this.unreliable = unreliable;
		return this;
	}

	/**
	 * Sets the critical option
	 * @param critical - the critical
	 * @return this bundle
	 */
	public Bundle setCritical(boolean critical) {
		this.critical = critical;
		return this;
	}

	/**
	 * Sets the flow label
	 * @param flowLabel - the flow label
	 * @return this bundle
	 */
	public Bundle setFlowLabel(int flowLabel) {
		this.flowLabel = flowLabel;
		return this;
	}

	/**
	 * Sets the bundle payload
	 * @param payload - the payload
	 * @return this bundle
	 */
	public Bundle setPayload(BundlePayload payload) {
		this.payload = payload;
		return this;
	}
	
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + ((creationTimestamp == null) ? 0 : creationTimestamp.hashCode());
		result = prime * result + fragmentOffset;
		result = prime * result + origLength;
		result = prime * result + ((source == null) ? 0 : source.hashCode());
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj) {
			return true;
		}
		if (obj == null) {
			return false;
		}
		if (!(obj instanceof Bundle)) {
			return false;
		}
		Bundle other = (Bundle) obj;
		if (creationTimestamp == null) {
			if (other.creationTimestamp != null) {
				return false;
			}
		} else if (!creationTimestamp.equals(other.creationTimestamp)) {
			return false;
		}
		if (fragmentOffset != other.fragmentOffset) {
			return false;
		}
		if (origLength != other.origLength) {
			return false;
		}
		if (source == null) {
			if (other.source != null) {
				return false;
			}
		} else if (!source.equals(other.source)) {
			return false;
		}
		return true;
	}
	
	private void createStatusReport() {
		if (this.statusReport == null)
			this.statusReport = new StatusReport(this);
	}
	
	// ***** JNI UTILITY FUNCTIONS

	@SuppressWarnings("unused")
	private void setReplyTo_C(BundleEID replyTo) {
		this.setReplyTo(replyTo);
	}
	
	@SuppressWarnings("unused")
	private void setSource_C(BundleEID source) {
		this.setSource(source);
	}
	
	@SuppressWarnings("unused")
	private void setSource_C(String str) {
		this.setSource(BundleEID.of(str));
	}
	
	@SuppressWarnings("unused")
	private void setCritical_C(int val) {
		this.setCritical(val != 0);
	}
	
	@SuppressWarnings("unused")
	private void setUnreliable_C(int val) {
		this.setUnreliable(val != 0);
	}
	
	@SuppressWarnings("unused")
	private void setDeliveryOption_C(int value) {
		this.setDeliveryOptions(BundleDeliveryOption.of(value));
	}
	
	@SuppressWarnings("unused")
	private void setExpiration_C(int expiration) {
		this.setExpiration(expiration);
	}
	
	@SuppressWarnings("unused")
	private void setPriority_C(int priority, int ordinal) {
		this.setPriority(new BundlePriority(priority, ordinal));
	}
	
	@SuppressWarnings("unused")
	private void setCreationTimestamp_C(int seconds, int sequenceNumber) {
		this.creationTimestamp = new BundleTimestamp(seconds, sequenceNumber);
	}
	
	@SuppressWarnings("unused")
	private void setReplyTo_C(String str) {
		this.setReplyTo(BundleEID.of(str));
	}
	
	@SuppressWarnings("unused")
	private void setDestination_C(BundleEID destination) {
		this.setDestination(destination);
	}
	
	@SuppressWarnings("unused")
	private void setDestination_C(String str) {
		this.setDestination(BundleEID.of(str));
	}
	
	@SuppressWarnings("unused")
	private void setBuffer_C(byte[] buffer) {
		this.payload = new BundlePayloadMemory(buffer);
	}
	
	@SuppressWarnings("unused")
	private void setPayloadFile_C(String payloadFileName) {
		this.payload = new BundlePayloadFile(payloadFileName);
	}
	
	@SuppressWarnings("unused")
	private void setPayloadTemporaryFile_C(String payloadFileName) {
		this.payload = new BundlePayloadTemporaryFile(payloadFileName);
	}
	
	@SuppressWarnings("unused")
	private void addBlock_C(byte[] blocks, int flags, int type) {
		this.addBlock(new BundleExtensionBlock(blocks, flags, type));
	}
	
	@SuppressWarnings("unused")
	private void setDeliveryRegID_C(int deliveryRegID) {
		this.setDeliveryRegID(deliveryRegID);
	}
	
	@SuppressWarnings("unused")
	private void addMetadata_C(byte[] metadata, int flags, int type) {
		this.addMetadata(new BundleMetadataBlock(metadata, flags));
	}
	
	@SuppressWarnings("unused")
	private void setSourceStatusReport_C(String str) {
		createStatusReport();
		this.statusReport.setSource(BundleEID.of(str));
	}
	
	@SuppressWarnings("unused")
	private void setCreationTimestampStatusReport_C(int seconds, int sequenceNumber) {
		createStatusReport();
		this.statusReport.setCreationTimestamp(new BundleTimestamp(seconds, sequenceNumber));
	}
	
	@SuppressWarnings("unused")
	private void setFragmentOffsetStatusReport_C(int fragmentOffset) {
		createStatusReport();
		this.statusReport.setFragmentOffset(fragmentOffset);
	}
	
	@SuppressWarnings("unused")
	private void setFragmentOffset_C(int fragmentOffset) {
		this.setFragmentOffset(fragmentOffset);
	}
	
	@SuppressWarnings("unused")
	private void setOrigLength_C(int origLength) {
		this.setOrigLength(origLength);
	}
	
	@SuppressWarnings("unused")
	private void setOrigLengthStatusReport_C(int origLength) {
		createStatusReport();
		this.statusReport.setOrigLength(origLength);
	}
	
	@SuppressWarnings("unused")
	private void setReasonStatusReport_C(int val) {
		createStatusReport();
		this.statusReport.setReason(StatusReportReason.of(val));
	}
	
	@SuppressWarnings("unused")
	private void setFlagStatusReport_C(int val) {
		createStatusReport();
		this.statusReport.setFlags(StatusReportFlag.of(val));
	}
	
	@SuppressWarnings("unused")
	private void setReceiptTimestampStatusReport_C(int secs, int seqno) {
		createStatusReport();
		this.statusReport.setReceiptTimestamp(new BundleTimestamp(secs, seqno));
	}
	
	@SuppressWarnings("unused")
	private void setFlowLabel_C(int flowLabel) {
		this.setFlowLabel(flowLabel);
	}
	
	@SuppressWarnings("unused")
	private void setCustodyTimestampStatusReport_C(int secs, int seqno) {
		createStatusReport();
		this.statusReport.setCustodyTimestamp(new BundleTimestamp(secs, seqno));
	}
	
	@SuppressWarnings("unused")
	private void setForwardingTimestampStatusReport_C(int secs, int seqno) {
		createStatusReport();
		this.statusReport.setForwardingTimestamp(new BundleTimestamp(secs, seqno));
	}
	
	@SuppressWarnings("unused")
	private void setDeliveryTimestampStatusReport_C(int secs, int seqno) {
		createStatusReport();
		this.statusReport.setDeliveryTimestamp(new BundleTimestamp(secs, seqno));
	}
	
	@SuppressWarnings("unused")
	private void setDeletionTimestampStatusReport_C(int secs, int seqno) {
		createStatusReport();
		this.statusReport.setDeletionTimestamp(new BundleTimestamp(secs, seqno));
	}
	
	@SuppressWarnings("unused")
	private void setAckByAppTimestampStatusReport_C(int secs, int seqno) {
		createStatusReport();
		this.statusReport.setAckByAppTimestamp(new BundleTimestamp(secs, seqno));
	}
	
	@SuppressWarnings("unused")
	private String getSourceAsString() {
		return (this.source != null ? this.source.getEndpointID() : "");
	}
	
	@SuppressWarnings("unused")
	private int getCreationTimestampSeconds() {
		return this.creationTimestamp.getSeconds();
	}
	
	@SuppressWarnings("unused")
	private int getCreationTimestampSequenceNumber() {
		return this.creationTimestamp.getSequenceNumber();
	}
	
	@SuppressWarnings("unused")
	private String getDestinationAsString() {
		return (this.destination != null ? this.destination.getEndpointID() : "");
	}
	
	@SuppressWarnings("unused")
	private String getReplyToAsString() {
		return (this.replyTo != null ? this.replyTo.getEndpointID() : "");
	}
	
	@SuppressWarnings("unused")
	private int getPriorityVal() {
		return this.priority.getPriority();
	}
	
	@SuppressWarnings("unused")
	private int getPriorityOrdinal() {
		return this.priority.getOrdinal();
	}
	
	@SuppressWarnings("unused")
	private int getDeliveryOptionsVal() {
		int result = 0;
		for (BundleDeliveryOption currentDeliveryOption : this.deliveryOptions) {
			result += currentDeliveryOption.getValue();
		}
		return result;
	}
	
	@SuppressWarnings("unused")
	private int getUnreliableAsVal() {
		return (this.unreliable ? 1 : 0);
	}
	
	@SuppressWarnings("unused")
	private int getCriticalAsVal() {
		return (this.critical ? 1 : 0);
	}
	
	@SuppressWarnings("unused")
	private int getPayloadLocationAsVal() {
		return this.payload.getPayloadLocation().getValue();
	}
	
	@SuppressWarnings("unused")
	private int getPayloadFileNameSize() {
		if (this.payload instanceof BundlePayloadFile) {
			return ((BundlePayloadFile)this.payload).getFileName().length();
		} else {
			return 0;
		}
	}
	
	@SuppressWarnings("unused")
	private String getPayloadFileName() {
		if (this.payload instanceof BundlePayloadFile) {
			return ((BundlePayloadFile)this.payload).getFileName();
		} else {
			return "";
		}
	}
	
	@SuppressWarnings("unused")
	private int getPayloadMemorySize() {
		if (this.payload instanceof BundlePayloadMemory) {
			return ((BundlePayloadMemory)this.payload).getData().length;
		} else {
			return 0;
		}
	}
	
	@SuppressWarnings("unused")
	private byte[] getPayloadMemoryData() {
		if (this.payload instanceof BundlePayloadMemory) {
			return ((BundlePayloadMemory)this.payload).getData();
		} else {
			return null;
		}
	}
	
	@SuppressWarnings("unused")
	private int getMetadataSize() {
		return this.metadata.size();
	}
	
	@SuppressWarnings("unused")
	private int getMetadataType(int index) {
		return this.metadata.get(index).getType();
	}
	
	@SuppressWarnings("unused")
	private int getMetadataFlags(int index) {
		return this.metadata.get(index).getFlags();
	}
	
	@SuppressWarnings("unused")
	private int getMetadataDataSize(int index) {
		return this.metadata.get(index).getData().length;
	}
	
	@SuppressWarnings("unused")
	private byte[] getMetadataData(int index) {
		return this.metadata.get(index).getData();
	}
	
	@SuppressWarnings("unused")
	private int getBlocksSize() {
		return this.blocks.size();
	}
	
	@SuppressWarnings("unused")
	private int getBlocksType(int index) {
		return this.blocks.get(index).getType();
	}
	
	@SuppressWarnings("unused")
	private int getBlocksFlags(int index) {
		return this.blocks.get(index).getFlags();
	}
	
	@SuppressWarnings("unused")
	private int getBlocksDataSize(int index) {
		return this.blocks.get(index).getData().length;
	}
	
	@SuppressWarnings("unused")
	private byte[] getBlocksData(int index) {
		return this.blocks.get(index).getData();
	}
	
}
