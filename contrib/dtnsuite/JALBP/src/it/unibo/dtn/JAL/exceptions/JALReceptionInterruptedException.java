package it.unibo.dtn.JAL.exceptions;

/** Reception Exception
 * @author Andrea Bisacchi
 *
 */
public class JALReceptionInterruptedException extends JALReceiveException {

	private static final long serialVersionUID = -3447143910599245491L;

	public JALReceptionInterruptedException() {
	}

	public JALReceptionInterruptedException(String message) {
		super(message);
	}

	public JALReceptionInterruptedException(Throwable cause) {
		super(cause);
	}

	public JALReceptionInterruptedException(String message, Throwable cause) {
		super(message, cause);
	}

}
