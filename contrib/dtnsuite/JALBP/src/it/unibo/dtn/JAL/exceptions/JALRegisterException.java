package it.unibo.dtn.JAL.exceptions;

/** Register Exception
 * @author Andrea Bisacchi
 *
 */
public class JALRegisterException extends Exception {

	private static final long serialVersionUID = 5626146390624728511L;

	public JALRegisterException() {
	}

	public JALRegisterException(String message) {
		super(message);
	}

	public JALRegisterException(Throwable cause) {
		super(cause);
	}

	public JALRegisterException(String message, Throwable cause) {
		super(message, cause);
	}

	public JALRegisterException(String message, Throwable cause, boolean enableSuppression,
			boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}

}
