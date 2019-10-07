package it.unibo.dtn.JAL;

/** 
 * <p>
 * Metadata Block.
 * </p>
 * <p>
 * According to RFC6258 the type value is 0x008
 * </p>
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public class BundleMetadataBlock extends BundleExtensionBlock {

	private static final int METADATATYPE = 0x008;
	
	public BundleMetadataBlock(byte[] data, int flags) {
		super(data, flags, METADATATYPE);
	}

}
