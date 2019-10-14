package it.unibo.dtn.JAL.exceptions;

/** Exception IPN Parameters Exception.
 * @author Andrea Bisacchi
 *
 */
public class JALIPNParametersException extends JALRegisterException {

	private static final long serialVersionUID = 5896208923655896757L;

	public JALIPNParametersException() {
	}

	public JALIPNParametersException(String message) {
		super(message);
	}

	public JALIPNParametersException(Throwable cause) {
		super(cause);
	}

	public JALIPNParametersException(String message, Throwable cause) {
		super(message, cause);
	}

	public JALIPNParametersException(String message, Throwable cause, boolean enableSuppression,
			boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}

}
