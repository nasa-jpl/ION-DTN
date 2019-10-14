package it.unibo.dtn.JAL;

import java.util.Arrays;

/** 
 * <p>
 * Extension Block
 * </p>
 * <p>
 * According to RFC5050 and RFC6257 the possible types are:
 * <ul>
 * <li>PAYLOAD_BLOCK               = 0x001 Defined in RFC5050</li>
 * <li>BUNDLE_AUTHENTICATION_BLOCK = 0x002 Defined in RFC6257</li>
 * <li>PAYLOAD_SECURITY_BLOCK      = 0x003 Defined in RFC6257</li>
 * <li>CONFIDENTIALITY_BLOCK       = 0x004 Defined in RFC6257</li>
 * <li>PREVIOUS_HOP_BLOCK          = 0x005 Defined in RFC6259</li>
 * <li>METADATA_BLOCK              = 0x008 Defined in RFC6258</li>
 * <li>EXTENSION_SECURITY_BLOCK    = 0x009 Defined in RFC6257</li>
 * <li>SESSION_BLOCK               = 0x00c NOT IN SPEC YET</li>
 * <li>AGE_BLOCK                   = 0x00a draft-irtf-dtnrg-bundle-age-block-01</li>
 * <li>QUERY_EXTENSION_BLOCK       = 0x00b draft-irtf-dtnrg-bpq-00</li>
 * <li>SEQUENCE_ID_BLOCK           = 0x010 NOT IN SPEC YET</li>
 * <li>OBSOLETES_ID_BLOCK          = 0x011 NOT IN SPEC YET</li>
 * <li>API_EXTENSION_BLOCK         = 0x100 INTERNAL ONLY -- NOT IN SPEC</li>
 * <li>UNKNOWN_BLOCK               = 0x101 INTERNAL ONLY -- NOT IN SPEC</li>
 * </ul>
 * </p>
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public class BundleExtensionBlock {
	private int type;
	private int flags;
	private byte[] data;
	
	/**
	 * Creates an Extension block (can be metadata or general block)
	 * @param data The data
	 * @param flags Flags
	 * @param type The type
	 */
	public BundleExtensionBlock(byte[] data, int flags, int type) {
		this.data = data;
		this.flags = flags;
		this.type = type;
	}
	
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + Arrays.hashCode(data);
		result = prime * result + flags;
		result = prime * result + type;
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj) {
			return true;
		}
		if (obj == null) {
			return false;
		}
		if (!(obj instanceof BundleExtensionBlock)) {
			return false;
		}
		BundleExtensionBlock other = (BundleExtensionBlock) obj;
		if (!Arrays.equals(data, other.data)) {
			return false;
		}
		if (flags != other.flags) {
			return false;
		}
		if (type != other.type) {
			return false;
		}
		return true;
	}

	public int getType() {
		return type;
	}

	public int getFlags() {
		return flags;
	}

	public byte[] getData() {
		return data;
	}

}
