package it.unibo.dtn.JAL.exceptions;

/** Local EndpointID Exception
 * @author Andrea Bisacchi
 *
 */
public class JALLocalEIDException extends JALRegisterException {

	private static final long serialVersionUID = -1010954074533901681L;

	public JALLocalEIDException() {
	}

	public JALLocalEIDException(String message) {
		super(message);
	}

	public JALLocalEIDException(Throwable cause) {
		super(cause);
	}

	public JALLocalEIDException(String message, Throwable cause) {
		super(message, cause);
	}

	public JALLocalEIDException(String message, Throwable cause, boolean enableSuppression,
			boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}

}
