package it.unibo.dtn.JAL;

/** 
 * Payload saved in Temporary File (deleted after the first read)
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public class BundlePayloadTemporaryFile extends BundlePayloadFile {
	/**
	 * The temporaryfile is deleted after first data reading
	 * @param fileName name of temporary file
	 */
	public BundlePayloadTemporaryFile(String fileName) {
		super(BundlePayloadLocation.TemporaryFile, fileName, true);
	}
}
