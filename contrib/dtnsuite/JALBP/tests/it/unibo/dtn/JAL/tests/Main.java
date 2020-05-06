package it.unibo.dtn.JAL.tests;

import java.util.function.Consumer;

import it.unibo.dtn.JAL.BPSocket;
import it.unibo.dtn.JAL.Bundle;
import it.unibo.dtn.JAL.BundleDeliveryOption;
import it.unibo.dtn.JAL.BundlePayload;
import it.unibo.dtn.JAL.JALEngine;
import it.unibo.dtn.JAL.exceptions.JALReceptionInterruptedException;
import it.unibo.dtn.JAL.exceptions.JALTimeoutException;

class Main {

	public static void main(String[] args) throws Exception {
		
		BPSocket testNonBlocking = BPSocket.register(15);
		Consumer<Bundle> handler = System.out::println;
		handler = handler.andThen(b -> {
			try {testNonBlocking.unregister();} catch (Throwable e) {}
		});
		Consumer<Throwable> exceptionHandler = System.err::println;
		exceptionHandler = exceptionHandler.andThen(b -> {
			try {testNonBlocking.unregister();} catch (Throwable e) {}
		});
		testNonBlocking.receive(handler, exceptionHandler);
		
		
		BPSocket socket = BPSocket.register(10);
		
		boolean stop = false;
		Bundle bundle = null;
		while (!stop) {
			try {
				bundle = socket.receive();
				if (bundle.getStatusReport() != null)
					continue;
				else
					System.out.print(new String(bundle.getData()));
			} catch (JALReceptionInterruptedException e) {continue;}
			catch (JALTimeoutException e) {
				System.out.println("Timeout");
				continue;
			}
			
			Bundle b = new Bundle(bundle.getSource());
			BundlePayload payload = BundlePayload.of("1234".getBytes());
			b.setPayload(payload);
			b.setReplyTo(socket.getLocalEID());
			b.addDeliveryOption(BundleDeliveryOption.Custody);
			b.addDeliveryOption(BundleDeliveryOption.CustodyReceipt);
			b.addDeliveryOption(BundleDeliveryOption.DeliveryReceipt);
			
			socket.send(b);
		}
		
		socket.unregister();
		JALEngine.getInstance().destroy();
	}

}
