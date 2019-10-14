package it.unibo.dtn.JAL;

/**
 * Bundle timestamp
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public class BundleTimestamp {
	private int seconds;
	private int sequenceNumber;
	
	/**
	 * Creates a timestamp
	 * @param seconds The seconds
	 * @param sequenceNumber The sequence Number
	 */
	public BundleTimestamp(int seconds, int sequenceNumber) {
		super();
		this.seconds = seconds;
		this.sequenceNumber = sequenceNumber;
	}
	
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + seconds;
		result = prime * result + sequenceNumber;
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
		if (!(obj instanceof BundleTimestamp)) {
			return false;
		}
		BundleTimestamp other = (BundleTimestamp) obj;
		if (seconds != other.seconds) {
			return false;
		}
		if (sequenceNumber != other.sequenceNumber) {
			return false;
		}
		return true;
	}

	/**
	 * Gets the seconds of creation
	 * @return The seconds of creation
	 */
	public int getSeconds() {
		return seconds;
	}
	
	/**
	 * Returns the seconds
	 * @param seconds the seconds
	 */
	public void setSeconds(int seconds) {
		this.seconds = seconds;
	}
	
	/**
	 * Returns the sequence number
	 * @return the sequence number
	 */
	public int getSequenceNumber() {
		return sequenceNumber;
	}
	
	/**
	 * Sets the sequence number
	 * @param sequenceNumber the sequence number
	 */
	public void setSequenceNumber(int sequenceNumber) {
		this.sequenceNumber = sequenceNumber;
	}
	
	@Override
	public String toString() {
		StringBuilder result = new StringBuilder();
		result.append(this.seconds);
		result.append('.');
		result.append(this.sequenceNumber);
		return result.toString();
	}

}
