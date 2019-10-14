package it.unibo.dtn.JAL;

/** 
 * Payload Location.<br>
 * Position where the Payload will be stored
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public enum BundlePayloadLocation {
File(0),
Memory(1),
TemporaryFile(2);
	
	private final int intVal;
	private BundlePayloadLocation(int val) {
		this.intVal = val;
	}
	
	/**
	 * The value according to C code 
	 * @return Value according to C code
	 */
	int getValue() {
		return this.intVal;
	}
	
	/**
	 * Creates a PayloadLocation from a value (according to C code)
	 * @param val The value according to C code
	 * @return A PayloadLocation or null if the val is not found
	 */
	static BundlePayloadLocation of(int val) {
		for (BundlePayloadLocation currentPayloadLocation : BundlePayloadLocation.values()) {
			if (currentPayloadLocation.getValue() == val)
				return currentPayloadLocation;
		}
		return null;
	}
	
}
