package it.unibo.dtn.JAL;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.OpenOption;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;

/** 
 * Payload saved or loaded from File
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public class BundlePayloadFile extends BundlePayload {
	private final Path path;
	private byte[] data = null;
	private final boolean tempFile;
	
	/**
	 * Creates a Payload referred to the file passed (some DTN implementation can limit the lenght of fileName)
	 * @param fileName The file that the payload will use to get the data. <b>Some DTN implementations can limit the lenght of fileName</b>
	 * @throws IllegalArgumentException In case of null value
	 */
	public BundlePayloadFile(final String fileName) throws IllegalArgumentException {
		this(BundlePayloadLocation.File, fileName, false);
	}
	
	/**
	 * Creates a BundlePayloadFile
	 * @param payloadLocation The location
	 * @param fileName The filename
	 * @param isTempFile If it is a temp file or not
	 * @throws IllegalArgumentException In case of null value
	 */
	protected BundlePayloadFile(final BundlePayloadLocation payloadLocation, final String fileName, boolean isTempFile) throws IllegalArgumentException {
		super(payloadLocation);
		if (fileName == null)
			throw new IllegalArgumentException("Filename can't be null.");
		this.path = Paths.get(fileName);
		this.tempFile = isTempFile;
	}

	@Override
	public InputStream getInputStream() {
		OpenOption openOption = StandardOpenOption.READ;
		try {
			if (this.tempFile)
				openOption = StandardOpenOption.DELETE_ON_CLOSE;
			return Files.newInputStream(this.path, openOption);
		} catch (IOException e) {
			return null;
		}
	}
	
	@Override
	public byte[] getData() {
		if (data != null)
			return data;
		try {
			this.data = Files.readAllBytes(this.path);
			if (this.tempFile)
				Files.delete(this.path);
		} catch (IOException e) {}
		return data;
	}
	
	/**
	 * Returns the filename
	 * @return The filename
	 */
	public String getFileName() {
		return this.path.toString();
	}


}
