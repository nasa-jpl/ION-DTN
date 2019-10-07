package it.unibo.dtn.JAL;

import java.net.URI;

/** 
 * Endpoint using DTN scheme.<br>
 * Example: <b>dtn:host1/program1</b>
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public final class BundleEIDDTNScheme extends BundleEID {
	private final String localName;
	private final String demuxString;

	/**
	 * Creates a DTN schema EndpointID (without demuxString)
	 * @param localName LocalName
	 * @throws IllegalArgumentException In case localName is null.
	 */
	public BundleEIDDTNScheme(String localName) throws IllegalArgumentException {
		this(localName, null);
	}
	
	/**
	 * Creates a DTN schema EndpointID
	 * @param localName LocalName
	 * @param demuxString DemuxString
	 * @throws IllegalArgumentException In case localName is null.
	 */
	public BundleEIDDTNScheme(String localName, String demuxString) throws IllegalArgumentException {
		if (localName == null || localName.length() == 0) 
			throw new IllegalArgumentException("Error, localName can't be empty.");

		this.localName = localName;
		this.demuxString = demuxString;

		StringBuilder builder = new StringBuilder();
		builder.append("dtn:");
		builder.append(localName);
		if (demuxString != null && demuxString.length() > 0) {
			builder.append('/');
			builder.append(demuxString);
		}
		try {
			super.setEndpointID(new URI(builder.toString()));
		} catch (Exception e) {
			throw new IllegalArgumentException("Error on building DTN EID.");
		}
	}

	/**
	 * Returns the LocalName
	 * @return The LocalName
	 */
	public String getLocalName() {
		return this.localName;
	}

	@Override
	public String getDemuxString() {
		return this.demuxString;
	}

	@Override
	public String getLocalString() {
		return this.getLocalName();
	}

}

