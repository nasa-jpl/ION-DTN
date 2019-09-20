package it.unibo.dtn.JAL.exceptions;

/**
 * 
 * @author Andrea Bisacchi
 *
 */
public class JALGeneralException extends Exception {

	private static final long serialVersionUID = -4937687803693122727L;

	public JALGeneralException() {
		super();
	}

	public JALGeneralException(String message, Throwable cause, boolean enableSuppression, boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}

	public JALGeneralException(String message, Throwable cause) {
		super(message, cause);
	}

	public JALGeneralException(String message) {
		super(message);
	}

	public JALGeneralException(Throwable cause) {
		super(cause);
	}

}
