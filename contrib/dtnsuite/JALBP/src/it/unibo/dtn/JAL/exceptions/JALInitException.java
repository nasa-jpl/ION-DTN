package it.unibo.dtn.JAL.exceptions;

/** 
 * Init Exception
 * @author Andrea Bisacchi
 *
 */
public class JALInitException extends Exception {

	private static final long serialVersionUID = 3716072856487089335L;

	public JALInitException() {
	}

	public JALInitException(String message) {
		super(message);
	}

	public JALInitException(Throwable cause) {
		super(cause);
	}

	public JALInitException(String message, Throwable cause) {
		super(message, cause);
	}

	public JALInitException(String message, Throwable cause, boolean enableSuppression, boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}

}
