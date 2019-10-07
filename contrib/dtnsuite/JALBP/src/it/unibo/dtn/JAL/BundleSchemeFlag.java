package it.unibo.dtn.JAL;

/** 
 * <p>
 * Possible schemes: IPN or DTN 
 * </p>
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public enum BundleSchemeFlag {
IPN, DTN;
	
	/**
	 * Returns the EIDscheme from a char (according to C code)
	 * @param scheme as a char. (I=IPN, D=DTN)
	 * @return The Bundle Scheme Flag or null if the param is not 'I' or 'D'
	 */
	static BundleSchemeFlag getSchemeFromChar(char scheme) {
		switch (scheme) {
		case 'I':
			return BundleSchemeFlag.IPN;

		case 'D':
			return BundleSchemeFlag.DTN;
			
		default:
			return null;
		}
	}
	
}
