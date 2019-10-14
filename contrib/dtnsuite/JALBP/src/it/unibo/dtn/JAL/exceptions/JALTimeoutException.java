package it.unibo.dtn.JAL.exceptions;

/**Timeout Exception
 * @author Andrea Bisacchi
 *
 */
public class JALTimeoutException extends JALReceiveException {

	private static final long serialVersionUID = 3461692587016464226L;

	public JALTimeoutException() {
	}

	public JALTimeoutException(String message) {
		super(message);
	}

	public JALTimeoutException(Throwable cause) {
		super(cause);
	}

	public JALTimeoutException(String message, Throwable cause) {
		super(message, cause);
	}

}
