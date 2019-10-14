package it.unibo.dtn.JAL.exceptions;

/** Open Exception
 * @author Andrea Bisacchi
 *
 */
public class JALOpenException extends JALRegisterException {

	private static final long serialVersionUID = 8131873839176367226L;

	public JALOpenException() {
	}

	public JALOpenException(String message) {
		super(message);
	}

	public JALOpenException(Throwable cause) {
		super(cause);
	}

	public JALOpenException(String message, Throwable cause) {
		super(message, cause);
	}

	public JALOpenException(String message, Throwable cause, boolean enableSuppression, boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}

}
