package it.unibo.dtn.JAL.exceptions;

import java.io.IOException;

/** Send Exception
 * @author Andrea Bisacchi
 *
 */
public class JALSendException extends IOException {

	private static final long serialVersionUID = 8956910223105504096L;

	public JALSendException() {
	}

	public JALSendException(String arg0) {
		super(arg0);
	}

	public JALSendException(Throwable arg0) {
		super(arg0);
	}

	public JALSendException(String arg0, Throwable arg1) {
		super(arg0, arg1);
	}

}
