package it.unibo.dtn.JAL.exceptions;

import java.io.IOException;

/** Unregister Exception
 * @author Andrea Bisacchi
 *
 */
public class JALUnregisterException extends IOException {

	private static final long serialVersionUID = -2950992698091156927L;

	public JALUnregisterException() {
	}

	public JALUnregisterException(String message) {
		super(message);
	}

	public JALUnregisterException(Throwable cause) {
		super(cause);
	}

	public JALUnregisterException(String message, Throwable cause) {
		super(message, cause);
	}

}
