package it.unibo.dtn.JAL.tests;

import static org.junit.jupiter.api.Assertions.*;

import org.junit.jupiter.api.Test;

import it.unibo.dtn.JAL.BundlePriority;
import it.unibo.dtn.JAL.BundlePriorityCardinal;

class TestBundlePriority {

	@Test
	void testPriority() {
		assertThrows(IllegalArgumentException.class, () -> {
			new BundlePriority(BundlePriorityCardinal.Bulk, 2);
		});
		assertThrows(IllegalArgumentException.class, () -> {
			new BundlePriority(BundlePriorityCardinal.Normal, 2);
		});
		assertThrows(IllegalArgumentException.class, () -> {
			new BundlePriority(BundlePriorityCardinal.Reserved);
		});
		assertDoesNotThrow(() -> {
			new BundlePriority(BundlePriorityCardinal.Expedited, 2);
		});
	}

}
