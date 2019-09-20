package it.unibo.dtn.JAL.exceptions;

/** Receiver Exception
 * @author Andrea Bisacchi
 *
 */
public class JALReceiverException extends JALSendException {

	private static final long serialVersionUID = -3217931555123026583L;

	public JALReceiverException() {
	}

	public JALReceiverException(String message) {
		super(message);
	}

	public JALReceiverException(Throwable cause) {
		super(cause);
	}

	public JALReceiverException(String message, Throwable cause) {
		super(message, cause);
	}

}
