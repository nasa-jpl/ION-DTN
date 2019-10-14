package it.unibo.dtn.JAL;

/** 
 * Payload saved in memory
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public final class BundlePayloadMemory extends BundlePayload {
	//private int CRC;
	private byte[] buffer;
	
	/**
	 * Creates a Bundle Payload with the data passed
	 * @param data Payload data buffer
	 */
	public BundlePayloadMemory(byte[] data) {
		super(BundlePayloadLocation.Memory);
		this.buffer = data.clone();
	}

	@Override
	public byte[] getData() {
		return this.buffer;
	}

}
