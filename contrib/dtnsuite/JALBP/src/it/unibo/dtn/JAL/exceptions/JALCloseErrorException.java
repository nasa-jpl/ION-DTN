package it.unibo.dtn.JAL.exceptions;

/** 
 * Close Exception
 * @author Andrea Bisacchi
 *
 */
public class JALCloseErrorException extends JALUnregisterException {

	private static final long serialVersionUID = 6929339757136887871L;

	public JALCloseErrorException() {
	}

	public JALCloseErrorException(String message) {
		super(message);
	}

	public JALCloseErrorException(Throwable cause) {
		super(cause);
	}

	public JALCloseErrorException(String message, Throwable cause) {
		super(message, cause);
	}

}
