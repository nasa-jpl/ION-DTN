package it.unibo.dtn.JAL.exceptions;

/** NullPointerException passed to C code
 * @author Andrea Bisacchi
 *
 */
public class JALNullPointerException extends RuntimeException {

	private static final long serialVersionUID = 2343869017726215223L;

	public JALNullPointerException() {
	}

	public JALNullPointerException(String message) {
		super(message);
	}

	public JALNullPointerException(Throwable cause) {
		super(cause);
	}

	public JALNullPointerException(String message, Throwable cause) {
		super(message, cause);
	}

	public JALNullPointerException(String message, Throwable cause, boolean enableSuppression,
			boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}

}
