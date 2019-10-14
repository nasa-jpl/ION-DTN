package it.unibo.dtn.JAL;

import it.unibo.dtn.JAL.exceptions.JALCloseErrorException;
import it.unibo.dtn.JAL.exceptions.JALDTN2ParametersException;
import it.unibo.dtn.JAL.exceptions.JALIPNParametersException;
import it.unibo.dtn.JAL.exceptions.JALGeneralException;
import it.unibo.dtn.JAL.exceptions.JALInitException;
import it.unibo.dtn.JAL.exceptions.JALLocalEIDException;
import it.unibo.dtn.JAL.exceptions.JALNoImplementationFoundException;
import it.unibo.dtn.JAL.exceptions.JALNotRegisteredException;
import it.unibo.dtn.JAL.exceptions.JALNullPointerException;
import it.unibo.dtn.JAL.exceptions.JALOpenException;
import it.unibo.dtn.JAL.exceptions.JALReceiveException;
import it.unibo.dtn.JAL.exceptions.JALReceiverException;
import it.unibo.dtn.JAL.exceptions.JALReceptionInterruptedException;
import it.unibo.dtn.JAL.exceptions.JALRegisterException;
import it.unibo.dtn.JAL.exceptions.JALSendException;
import it.unibo.dtn.JAL.exceptions.JALTimeoutException;
import it.unibo.dtn.JAL.exceptions.JALUnregisterException;

/** 
 * Exception Manager. Very usefull to have a generic resolution of Exceptions.
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
class ExceptionManager {
	private ExceptionManager() {}
	
	/**
	 * Throws Exception in case the <b>error</b> is an AL_BP Exception
	 * @param error The error resulted in AL_BP
	 * @param errorString The custom String in case of Exception
	 * @throws JALNullPointerException 
	 * @throws JALInitException 
	 * @throws JALRegisterException 
	 * @throws JALUnregisterException 
	 * @throws JALNotRegisteredException 
	 * @throws JALSendException 
	 * @throws JALReceiveException 
	 * @throws JALGeneralException 
	 */
	public static void checkError(int error, String errorString) throws JALNullPointerException, JALInitException, JALRegisterException, JALUnregisterException, JALNotRegisteredException, JALSendException, JALReceiveException, JALGeneralException {
		switch (error) {
		case 0: // No error -> everything is ok
			break;

		case 1:	throw new JALNullPointerException(errorString + " Error, null pointer passed to C code.");

		case 2:	throw new JALInitException(errorString + " Error on initializing ALBP.");

		case 3:	throw new JALOpenException(errorString + " Error on opening DTN socket.");

		case 4:	throw new JALLocalEIDException(errorString + " Error on building local EID.");

		case 5:	throw new JALRegisterException(errorString + " Error on registering DTN socket.");

		case 6:	throw new JALCloseErrorException(errorString + " Error on closing DTN socket.");

		case 7:	throw new JALUnregisterException(errorString + " Error on unregistering DTN socket.");

		case 8:	throw new JALNotRegisteredException(errorString + " Error, the socket was not registered.");

		case 9: throw new JALSendException(errorString + " General error on sending bundle.");
		
		case 10: throw new JALReceiveException(errorString + " General error on receiving bundle.");
		
		case 11: throw new JALReceptionInterruptedException(errorString + " Error, the reception was interrupted.");

		case 12: throw new JALTimeoutException(errorString + " Error, timeout on receiving bundle.");
		
		case 13: throw new JALReceiverException(errorString + " Error, the receiver is not indicated or is dtn:none.");
		
		case 14: throw new JALIPNParametersException(errorString + " Error on passing value for IPN schema.");

		case 15: throw new JALDTN2ParametersException(errorString + " Error on parameters passed to DTN2.");

		case 16: throw new JALNoImplementationFoundException(errorString + " Error, no implementation found.");

		default: throw new JALGeneralException(errorString + " General error occurred. The error is not defined. Error code=" + error);
		}
	}

	/**
	 * The same as {@link #checkError(int, String) checkError} but without the costum String
	 * @param error The error resulted in AL_BP
	 * @throws JALGeneralException 
	 * @throws JALNotRegisteredException 
	 * @throws JALUnregisterException 
	 * @throws JALRegisterException 
	 * @throws JALInitException 
	 * @throws JALNullPointerException 
	 * @throws JALReceiveException 
	 * @throws JALSendException 
	 */
	public static void checkError(int error) throws JALSendException, JALReceiveException, JALNullPointerException, JALInitException, JALRegisterException, JALUnregisterException, JALNotRegisteredException, JALGeneralException {
		checkError(error, "");
	}

}
