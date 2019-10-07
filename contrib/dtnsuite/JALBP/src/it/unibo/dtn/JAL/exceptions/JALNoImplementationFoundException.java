package it.unibo.dtn.JAL.exceptions;

/** No Implementation Found Exception
 * @author Andrea Bisacchi
 *
 */
public class JALNoImplementationFoundException extends JALInitException {

	private static final long serialVersionUID = -6097056302780187215L;

	public JALNoImplementationFoundException() {
	}

	public JALNoImplementationFoundException(String message) {
		super(message);
	}

	public JALNoImplementationFoundException(Throwable cause) {
		super(cause);
	}

	public JALNoImplementationFoundException(String message, Throwable cause) {
		super(message, cause);
	}

	public JALNoImplementationFoundException(String message, Throwable cause, boolean enableSuppression,
			boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}

}
