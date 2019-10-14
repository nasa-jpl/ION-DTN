package it.unibo.dtn.JAL;

/** 
 * Bundle Priority Cardinal
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public enum BundlePriorityCardinal {
Bulk(0),
Normal(1),
Expedited(2),
Reserved(3);
	
	private final int intVal;
	
	private BundlePriorityCardinal(int val) {
		this.intVal = val;
	}
	
	/**
	 * Gets the value (according to C code)
	 * @return The value
	 */
	int getValue() {
		return this.intVal;
	}
	
	/**
	 * Creates a DTNPriority from an integer value (according to C code).
	 * @param val The value (according to C code)
	 * @return A DTNPriority instance or null if the val is not correct
	 */
	static BundlePriorityCardinal of(int val) {
		for (BundlePriorityCardinal currentBundlePriorityCardinal : BundlePriorityCardinal.values()) {
			if (currentBundlePriorityCardinal.getValue() == val)
				return currentBundlePriorityCardinal;
		}
		return null;
	}
	
}
