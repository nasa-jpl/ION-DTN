package it.unibo.dtn.JAL;

/** 
 * Status Report Reason
 * @author Andrea Bisacchi
 * @version 1.0
 */
public enum StatusReportReason {
NoAddtlInfo(0),
LifetimeExpired(1),
ForwardedUnidirLink(2),
TransmissionCancelled(3),
DepletedStorage(4),
EndpointIDUnintelligible(5),
NoRouteToDest(6),
NoTimelyContact(7),
BlockUnintelligible(8);
	
	private final int intVal;
	private StatusReportReason(int val) {
		this.intVal = val;
	}
	
	/**
	 * Gets the value
	 * @return The value (for C code)
	 */
	int getValue() {
		return this.intVal;
	}
	
	/**
	 * Gets the Status Report Reason by val (in C code)
	 * @param val The val from C code
	 * @return The Status Report Reason if val is valid, null otherwise
	 */
	static StatusReportReason of(int val) {
		for (StatusReportReason currentReason : StatusReportReason.values()) {
			if (currentReason.getValue() == val)
				return currentReason;
		}
		return null;
	}
	
}
