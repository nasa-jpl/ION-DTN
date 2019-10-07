package it.unibo.dtn.JAL;

import java.net.URI;

/** 
 * Bundle Endpoint ID. You can choose between two schemes: IPN and DTN.
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public abstract class BundleEID {

	/**
	 * None endpoint according to standard. The value will be <b>dtn:none</b>
	 */
	public static final BundleEID NoneEndpoint = new BundleEIDDTNScheme("none");

	private URI uri = null;

	protected BundleEID() {}

	protected void setEndpointID(URI uri) {
		this.uri = uri;
	}

	/**
	 * Returns the Endpoint ID as String
	 * @return The Endpoint as a String
	 */
	public String getEndpointID() {
		return this.uri.toString();
	}

	@Override
	public String toString() {
		return this.uri.toString();
	}

	/**
	 * Returns the localString
	 * @return The localString
	 */
	public abstract String getLocalString();

	/**
	 * Returns the demuxString
	 * @return The demuxString
	 */
	public abstract String getDemuxString();

	/**
	 * Factory constructor of EID
	 * @param str String in IPN or DTN schema
	 * @return The DTNEndpointID or null in case of malformed string
	 */
	public static BundleEID of(String str) {
		if (str == null || str.length() == 0 || str.equals(NoneEndpoint.getEndpointID()))
			return NoneEndpoint;

		URI uri = null;
		try {
			uri = URI.create(str);
		} catch (Exception e) { // Malformed
			return null;
		}
		return BundleEID.of(uri);
	}

	/**
	 * Factory constructor of EID
	 * @param uri IPN or DTN URI
	 * @return The DTNEndpointID or null in case of malformed uri
	 */
	public static BundleEID of(URI uri) {
		if (uri == null)
			return null;
		BundleEID result = null;
		try {
			if (uri.getScheme().equals("ipn")) {
				String firstNumber = uri.toString().substring(4, uri.toString().indexOf("."));
				String secondNumber = uri.toString().substring(uri.toString().indexOf(".")+1);
				result = new BundleEIDIPNScheme(Integer.parseInt(firstNumber), Integer.parseInt(secondNumber));
			} else if (uri.getScheme().equals("dtn")) {
				String firstPart = uri.toString().substring(4, uri.toString().indexOf("/"));
				String secondPart = null;
				try {
					secondPart = uri.toString().substring(uri.toString().indexOf("/") + 1);
				} catch (IndexOutOfBoundsException e) {	}
				result = new BundleEIDDTNScheme(firstPart, secondPart);
			}
		} catch (Exception e) {
			return null;
		}
		return result;
	}

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + ((uri == null) ? 0 : uri.toString().hashCode());
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
		if (!(obj instanceof BundleEID)) {
			return false;
		}
		BundleEID other = (BundleEID) obj;
		if (uri == null) {
			if (other.uri != null) {
				return false;
			}
		} else if (!uri.toString().equals(other.uri.toString())) {
			return false;
		}
		return true;
	}

}