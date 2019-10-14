package it.unibo.dtn.JAL.tests;

import static org.junit.jupiter.api.Assertions.*;

import org.junit.jupiter.api.Test;

import it.unibo.dtn.JAL.BundleEID;

class TestEndpoint {

	@Test
	void testDTNEndpoint() {
		String str = "dtn:prova/ciao";
		BundleEID e = BundleEID.of(str);
		assertEquals(str, e.getEndpointID());
		assertEquals(str, e.toString());
		assertEquals("prova", e.getLocalString());
		assertEquals("ciao", e.getDemuxString());
	}
	
	void testIPNEndpoint() {
		String str = "ipn:5.100";
		BundleEID e = BundleEID.of(str);
		assertEquals(str, e.getEndpointID());
		assertEquals(str, e.toString());
		assertEquals(""+5, e.getLocalString());
		assertEquals(""+100, e.getDemuxString());
	}

}
