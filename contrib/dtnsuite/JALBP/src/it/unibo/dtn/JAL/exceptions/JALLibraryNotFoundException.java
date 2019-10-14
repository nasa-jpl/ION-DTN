/**
 * 
 */
package it.unibo.dtn.JAL.exceptions;

/**
 * Exception generated if the JAL library is not installed properly.
 * @author Andrea Bisacchi
 */
public class JALLibraryNotFoundException extends RuntimeException {

	private static final long serialVersionUID = 5873096109560673286L;

	public JALLibraryNotFoundException() {
	}

	public JALLibraryNotFoundException(String message) {
		super(message);
	}

	public JALLibraryNotFoundException(Throwable cause) {
		super(cause);
	}

	public JALLibraryNotFoundException(String message, Throwable cause) {
		super(message, cause);
	}

	public JALLibraryNotFoundException(String message, Throwable cause, boolean enableSuppression,
			boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}

}
