package it.unibo.dtn.JAL;

/** 
 * <p>
 * Possible schemes used in {@link JALEngine#setForceEIDScheme(EngineForceEIDScheme)}.
 * </p>
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public enum EngineForceEIDScheme {
NOFORCE('N'),
IPN('I'),
DTN('D');
	
	private final char forceEIDScheme; 
	
	private EngineForceEIDScheme(char forceEID) {
		this.forceEIDScheme = forceEID;
	}
	
	/**
	 * Returns the char referred to the forcing scheme (according to C code)
	 * @return The char referred to the forcing scheme (according to C code)
	 */
	char getCharForceEIDScheme() {
		return this.forceEIDScheme;
	}
}
