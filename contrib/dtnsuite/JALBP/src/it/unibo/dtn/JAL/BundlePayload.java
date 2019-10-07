package it.unibo.dtn.JAL;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;

/** 
 * Bundle Payload
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public abstract class BundlePayload {
	private final BundlePayloadLocation location;
	
	/**
	 * Base constructor of Bundle Payload
	 * @param location PayloadLocation
	 * @throws IllegalArgumentException In case of null value
	 */
	protected BundlePayload(BundlePayloadLocation location) throws IllegalArgumentException {
		if (location == null)
			throw new IllegalArgumentException("Location can't be null.");
		this.location = location;
	}
	
	/**
	 * Returns the payload location
	 * @return The payload location where the data are saved
	 */
	public BundlePayloadLocation getPayloadLocation() {
		return this.location;
	}
	
	/**
	 * Returns the input stream for reading data
	 * @return the input stream for reading data
	 */
	public InputStream getInputStream() {
		return new ByteArrayInputStream(this.getData());
	}

	/**
	 * Returns the input stream reader for reading data
	 * @return the input stream reader for reading data
	 */
	public InputStreamReader getInputStreamReader() {
		return new InputStreamReader(this.getInputStream());
	}
	
	/**
	 * Returns the buffered reader for reading data
	 * @return the buffered reader for reading data
	 */
	public BufferedReader getBufferedReader() {
		return new BufferedReader(this.getInputStreamReader());
	}
	
	/**
	 * Gets the data payload data
	 * @return The payload data
	 */
	public abstract byte[] getData();

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + ((location == null) ? 0 : location.hashCode());
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
		if (!(obj instanceof BundlePayload)) {
			return false;
		}
		BundlePayload other = (BundlePayload) obj;
		if (location != other.location) {
			return false;
		}
		return true;
	}

	@Override
	public String toString() {
		return "Payload type="+this.location + "\tdata=" + new String(this.getData());
	}
	
	/**
	 * Payload factory
	 * @param data To be inserted the payload
	 * @return The BundlePayload created
	 * @throws IllegalArgumentException In case of null pointer passed
	 */
	public static BundlePayload of(byte[] data) throws IllegalArgumentException {
		if (data == null)
			throw new IllegalArgumentException("Data can't be null.");
		return new BundlePayloadMemory(data);
	}
	
	/**
	 * Payload factory
	 * @param fileName The filename
	 * @return The BundlePayload created
	 * @throws IllegalArgumentException
	 */
	public static BundlePayload of(String fileName) throws IllegalArgumentException {
		if (fileName == null)
			throw new IllegalArgumentException("Filename can't be null.");
		return new BundlePayloadFile(fileName);
	}
}
