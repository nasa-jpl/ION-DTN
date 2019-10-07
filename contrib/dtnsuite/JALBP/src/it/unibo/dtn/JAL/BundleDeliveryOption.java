package it.unibo.dtn.JAL;

import java.util.LinkedList;
import java.util.List;

/** 
 * Delivery option required when sending a bundle
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public enum BundleDeliveryOption {
None(0),
Custody(1),
DeliveryReceipt(2),
ReceiveReceipt(4),
ForwardReceipt(8),
CustodyReceipt(16),
DeleteReceipt(32),
SingletonDestination(64),
MultinodeDestination(128),
DoNotFragment(256);
	
	private final int intVal;
	private BundleDeliveryOption(int val) {
		this.intVal = val;
	}
	
	/**
	 * Returns the value according to C code
	 * @return The value according to C code
	 */
	int getValue() {
		return this.intVal;
	}
	
	/**
	 * Returns the List according to C code
	 * @param val The value according to C code
	 * @return The List
	 */
	static List<BundleDeliveryOption> of(int val) {
		List<BundleDeliveryOption> result = new LinkedList<>();
		if (val == None.getValue()) {
			result.add(None);
			return result;
		}
		if (val >= DoNotFragment.getValue()) {
			val -= DoNotFragment.getValue();
			result.add(DoNotFragment);
		}
		if (val >= MultinodeDestination.getValue()) {
			val -= MultinodeDestination.getValue();
			result.add(MultinodeDestination);
		}
		if (val >= SingletonDestination.getValue()) {
			val -= SingletonDestination.getValue();
			result.add(SingletonDestination);
		}
		if (val >= DeleteReceipt.getValue()) {
			val -= DeleteReceipt.getValue();
			result.add(DeleteReceipt);
		}
		if (val >= CustodyReceipt.getValue()) {
			val -= CustodyReceipt.getValue();
			result.add(CustodyReceipt);
		}
		if (val >= ForwardReceipt.getValue()) {
			val -= ForwardReceipt.getValue();
			result.add(ForwardReceipt);
		}
		if (val >= ReceiveReceipt.getValue()) {
			val -= ReceiveReceipt.getValue();
			result.add(ReceiveReceipt);
		}
		if (val >= DeliveryReceipt.getValue()) {
			val -= DeliveryReceipt.getValue();
			result.add(DeliveryReceipt);
		}
		if (val >= Custody.getValue()) {
			val -= Custody.getValue();
			result.add(Custody);
		}
		return result;
		}
	
}
