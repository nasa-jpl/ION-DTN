package it.unibo.dtn.JAL;

import it.unibo.dtn.JAL.exceptions.JALDTN2ParametersException;
import it.unibo.dtn.JAL.exceptions.JALGeneralException;
import it.unibo.dtn.JAL.exceptions.JALInitException;
import it.unibo.dtn.JAL.exceptions.JALLibraryNotFoundException;
import it.unibo.dtn.JAL.exceptions.JALNoImplementationFoundException;
import it.unibo.dtn.JAL.exceptions.JALNotRegisteredException;
import it.unibo.dtn.JAL.exceptions.JALNullPointerException;
import it.unibo.dtn.JAL.exceptions.JALReceiveException;
import it.unibo.dtn.JAL.exceptions.JALRegisterException;
import it.unibo.dtn.JAL.exceptions.JALSendException;
import it.unibo.dtn.JAL.exceptions.JALUnregisterException;

/** 
 * Class used to force some parameters in DTN sockets.<br>
 * You can force the EID scheme and the local IPN Node in case you are using DTN2 implementation and you are forcing IPN scheme.<br>
 * <p>
 * <b>Usage:</b><br>
 * <ol>
 * <li>Call {@link #getInstance()} to get the instance.</li>
 * <li>(Optional) Set the forcing parameters before opening {@link BPSocket}s ({@link #setForceEIDScheme(EngineForceEIDScheme)} and {@link #setIPNNodeForDTN2(int)}).</li>
 * <li>(Optional) Call {@link #init()} function.</li>
 * <li>Call {@link #destroy()} function when all {@link BPSocket}s are unregistered and you are not willing to open anymore.</li>
 * </ol>
 * </p>
 * 
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public final class JALEngine {
	private EngineForceEIDScheme forceEID = EngineForceEIDScheme.NOFORCE;
	private int IPNNodeForDTN2 = 0;
	
	private boolean initialized = false;
	
	/**
	 * Returns the Force EID scheme
	 * @return The Force EID
	 */
	public EngineForceEIDScheme getForceEID() {
		return this.forceEID;
	}
	
	/**
	 * Requires to force the scheme
	 * @param forceEIDscheme - the scheme to force
	 * @return This object (for serial settings calls)
	 * @throws IllegalStateException In case you try to change after the creation of a {@link BPSocket}
	 */
	public JALEngine setForceEIDScheme(EngineForceEIDScheme forceEIDscheme) throws IllegalStateException {
		if (this.initialized)
			throw new IllegalStateException();
		this.forceEID = forceEIDscheme;
		return this;
	}
	
	/**
	 * Returns the IPNNodeForDTN2
	 * @return The IPNNodeForDTN2
	 */
	public int getIPNNodeForDTN2() {
		return this.IPNNodeForDTN2;
	}
	
	/**
	 * Sets the local IPN node number. To be used if forcing IPN scheme and using DTN2 implementation.
	 * @param IPNNodeForDTN2 - local IPN Node number
	 * @return This object (for serial settings calls)
	 * @throws IllegalStateException In case you try to change after the creation of a {@link BPSocket}
	 */
	public JALEngine setIPNNodeForDTN2(int IPNNodeForDTN2) throws IllegalStateException {
		if (this.initialized)
			throw new IllegalStateException();
		this.IPNNodeForDTN2 = IPNNodeForDTN2;
		return this;
	}
	
	/**
	 * <b>Important function.</b><br>
	 * Before calling this function, if you need to, you have to call {@link #setForceEIDScheme(EngineForceEIDScheme)} and {@link #setIPNNodeForDTN2(int)}.<br>
	 * This function have to be called once before using {@link BPSocket}.<br>
	 * In case you will not call {@link #init()} the {@link BPSocket} will do it for you.
	 * @throws JALNoImplementationFoundException - in case no DTN implementations were found in the current system
	 * @throws JALDTN2ParametersException - in case you are forcing IPN scheme but the 
	 * @throws JALInitException - in case of other occurrences
	 */
	public void init() throws JALNoImplementationFoundException, JALDTN2ParametersException, JALInitException {
		if (this.initialized) return;
		int error = JALEngine.c_init(this.forceEID.getCharForceEIDScheme(), this.IPNNodeForDTN2);
		try {
			ExceptionManager.checkError(error, "Error on ALBPEngine.init.");
		}
		catch (JALSendException | JALReceiveException | JALNullPointerException
				| JALRegisterException | JALUnregisterException | JALNotRegisteredException | JALGeneralException e) {
			throw new JALInitException(e);
		} 
		this.initialized = true;
	}
	
	/**
	 * <b>Important function.</b><br>
	 * You have to call this function when all {@link BPSocket}s are closed and you don't want to use {@link BPSocket}s anymore. 
	 */
	public void destroy() {
		if (this.initialized) {
			JALEngine.c_destroy();
			this.initialized = false;
		}
	}
	
	/**
	 * Returns the EIDFormat. Before using this you have to initialize the engine calling {@link #init()}
	 * @return The EIDFormat
	 * @throws JALInitException - in case the library is not initialized
	 */
	public BundleSchemeFlag getEIDFormat() throws JALInitException {
		if (this.initialized)
			return BundleSchemeFlag.getSchemeFromChar(JALEngine.c_get_eid_format());
		else
			throw new  JALInitException("Library not initialized.");
	}
	
	private static JALEngine instance = null; // Singleton
	private JALEngine() {}
	/**
	 * Returns the instance of JALEngine.
	 * @return The instance of JALEngine
	 * @throws JALLibraryNotFoundException In case the library is not found in the system (probably not installed)
	 * @throws JALNoImplementationFoundException - in case no DTN implementations were found in the current system
	 * @throws JALDTN2ParametersException - in case you are forcing IPN scheme but the 
	 * @throws JALInitException - in case of other occurrences
	 */
	public static JALEngine getInstance() throws JALLibraryNotFoundException, JALNoImplementationFoundException, JALDTN2ParametersException, JALInitException {
		if (JALEngine.instance == null) {
			JALEngine.loadLibrary();
			JALEngine.instance = new JALEngine();
			JALEngine.instance.init();
		}
		return JALEngine.instance;
	}
	
	/**
	 * Loads the al_bp library for JNI.
	 * @throws JALLibraryNotFoundException In case the library is not found in the system (probably not installed)
	 */
	private static void loadLibrary() throws JALLibraryNotFoundException {
		final String libraryName = "al_bp";
		try {
			System.loadLibrary(libraryName);
		} catch (Throwable e) {
			throw new JALLibraryNotFoundException("Error, library " + libraryName + " not found in: " + System.getProperty("java.library.path"), e);
		}
	}
	
	private static native int c_init(char force_eid, int IPNNodeForDTN2);
	private static native void c_destroy();
	private static native char c_get_eid_format();
	
}
