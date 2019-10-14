package it.unibo.dtn.JAL;

import java.util.LinkedList;
import java.util.List;

/** 
 * <p>
 * The status report flags
 * </p>
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public enum StatusReportFlag {
Received(1),
CustodyAccepted(2),
Forwarded(4),
Delivered(8),
Deleted(16),
AckedByApp(32);
	
	private final int intVal;
	private StatusReportFlag(int val) {
		this.intVal = val;
	}
	
	/**
	 * Gets the value (for C code)
	 * @return The value according to C code
	 */
	int getValue() {
		return this.intVal;
	}
	
	/**
	 * Returns the List from value according to C code
	 * @param val The value according to C code
	 * @return The List from the value according to C code
	 */
	static List<StatusReportFlag> of(int val) {
		List<StatusReportFlag> result = new LinkedList<>();
		if (val >= AckedByApp.getValue()) {
			val -= AckedByApp.getValue();
			result.add(AckedByApp);
		}
		if (val >= Deleted.getValue()) {
			val -= Deleted.getValue();
			result.add(Deleted);
		}
		if (val >= Delivered.getValue()) {
			val -= Delivered.getValue();
			result.add(Delivered);
		}
		if (val >= Forwarded.getValue()) {
			val -= Forwarded.getValue();
			result.add(Forwarded);
		}
		if (val >= CustodyAccepted.getValue()) {
			val -= CustodyAccepted.getValue();
			result.add(CustodyAccepted);
		}
		if (val >= Received.getValue()) {
			val -= Received.getValue();
			result.add(Received);
		}
		return result;
	}
	
}
