package it.unibo.dtn.JAL;

/** 
 * Bundle Priority
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public final class BundlePriority {
	private int priority;
	private int ordinal;
	
	/**
	 * Creates a BundlePriority with the priority required
	 * @param priority Priority
	 * @throws IllegalArgumentException If ordinal is set with priority not Expedited or priority is Reserved
	 */
	public BundlePriority(BundlePriorityCardinal priority) throws IllegalArgumentException {
		this(priority.getValue());
	}
	
	/**
	 * Creates a BundlePriority with the priority required
	 * @param priority Priority
	 * @param ordinal Ordinal
	 * @throws IllegalArgumentException If ordinal is set with priority not Expedited or priority is Reserved
	 */
	public BundlePriority(BundlePriorityCardinal priority, int ordinal) throws IllegalArgumentException {
		this(priority.getValue(), ordinal);
	}
	
	/**
	 * Creates a BundlePriority with the priority required
	 * @throws IllegalArgumentException If ordinal is set with priority not Expedited or priority is Reserved
	 * @param priority priority
	 */
	public BundlePriority(int priority) throws IllegalArgumentException {
		this(priority, 0);
	}
	
	/**
	 * Creates a BundlePriority with the priority required
	 * @param priority priority
	 * @param ordinal ordinal
	 * @throws IllegalArgumentException If ordinal is set with priority not Expedited or priority is Reserved
	 */
	public BundlePriority(int priority, int ordinal) throws IllegalArgumentException {
		this.priority = priority;
		if (priority != BundlePriorityCardinal.Expedited.getValue() && ordinal != 0)
			throw new IllegalArgumentException("Can't set ordinal unless cardinal priority is " + BundlePriorityCardinal.Expedited.toString());
		if (priority == BundlePriorityCardinal.Reserved.getValue())
			throw new IllegalArgumentException("Can't use " + BundlePriorityCardinal.Reserved.toString() + " as a priority.");
		this.ordinal = ordinal;
	}

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + ordinal;
		result = prime * result + priority;
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
		if (!(obj instanceof BundlePriority)) {
			return false;
		}
		BundlePriority other = (BundlePriority) obj;
		if (ordinal != other.ordinal) {
			return false;
		}
		if (priority != other.priority) {
			return false;
		}
		return true;
	}

	public int getPriority() {
		return priority;
	}
	
	public BundlePriorityCardinal getPriorityAsEnum() {
		return BundlePriorityCardinal.of(this.priority);
	}

	public void setPriority(int priority) {
		this.priority = priority;
	}
	
	public void setPriority(BundlePriorityCardinal proprity) {
		this.setPriority(proprity.getValue());
	}

	public int getOrdinal() {
		return ordinal;
	}

	public void setOrdinal(int ordinal) {
		this.ordinal = ordinal;
	}
}
