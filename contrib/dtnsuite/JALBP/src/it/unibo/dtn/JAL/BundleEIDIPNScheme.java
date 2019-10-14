package it.unibo.dtn.JAL;

import java.net.URI;

/** 
 * Endpoint using IPN scheme.<br>
 * Example: <b>ipn:10.3</b><br>
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public final class BundleEIDIPNScheme extends BundleEID {
	private final int localNumber;
	private final int demuxNumber;
	
	/**
	 * Builds an Endpoint according to IPN schema.<br>
	 * It follows the schema: ipn:localNumber.demuxNumber<br>
	 * Example: ipn:10.3<br>
	 * @param localNumber Local number
	 * @param demuxNumber Demux number
	 * @throws IllegalArgumentException In case the localNumber or demuxNumer are less then 0
	 */
	public BundleEIDIPNScheme(int localNumber, int demuxNumber) throws IllegalArgumentException {
		if (localNumber < 0 || demuxNumber < 0) 
			throw new IllegalArgumentException("Error, localNumber and/or demuxNumber can't be negative.");
			
		this.localNumber = localNumber;
		this.demuxNumber = demuxNumber;
		
		StringBuilder builder = new StringBuilder();
		builder.append("ipn:");
		builder.append(localNumber);
		builder.append('.');
		builder.append(demuxNumber);
		try {
			super.setEndpointID(new URI(builder.toString()));
		} catch (Exception e) {
			throw new IllegalArgumentException("Error on building IPN EID.");
		}
	}

	/**
	 * Gets the localNumber
	 * @return The localNumber
	 */
	public int getLocalNumber() {
		return this.localNumber;
	}
	
	/**
	 * Gets the demuxNumber
	 * @return The demuxNumber
	 */
	public int getDemuxNumber() {
		return this.demuxNumber;
	}

	/**
	 * The localNumber as String
	 * @return The localNumber as String
	 */
	@Override
	public String getLocalString() {
		return ""+this.getLocalNumber();
	}

	/**
	 * The demuxNumber as String
	 * @return The demuxNumber as String
	 */
	@Override
	public String getDemuxString() {
		return ""+this.getDemuxNumber();
	}
	
}
