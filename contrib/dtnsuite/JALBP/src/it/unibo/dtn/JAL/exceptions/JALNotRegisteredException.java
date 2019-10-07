package it.unibo.dtn.JAL.exceptions;

/** Not registered Exception
 * @author Andrea Bisacchi
 *
 */
public class JALNotRegisteredException extends RuntimeException {

	private static final long serialVersionUID = 9210458729062575129L;

	public JALNotRegisteredException() {
	}

	public JALNotRegisteredException(String message) {
		super(message);
	}

	public JALNotRegisteredException(Throwable cause) {
		super(cause);
	}

	public JALNotRegisteredException(String message, Throwable cause) {
		super(message, cause);
	}

	public JALNotRegisteredException(String message, Throwable cause, boolean enableSuppression,
			boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}

}
