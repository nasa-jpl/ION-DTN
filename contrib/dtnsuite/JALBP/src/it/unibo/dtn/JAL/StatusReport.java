package it.unibo.dtn.JAL;

import java.util.LinkedList;
import java.util.List;

/** 
 * Status Report
 * @author Andrea Bisacchi
 * @version 1.0
 *
 */
public final class StatusReport {
	private final Bundle bundle;
	
	private BundleEID source;
	private BundleTimestamp creationTimestamp;
	private int fragmentOffset = 0;
	private int origLength = 0;
	
	private StatusReportReason reason;
	private List<StatusReportFlag> flags = new LinkedList<>();
	private BundleTimestamp receiptTimestamp;
	private BundleTimestamp custodyTimestamp;
	private BundleTimestamp forwardingTimestamp;
	private BundleTimestamp deliveryTimestamp;
	private BundleTimestamp deletionTimestamp;
	private BundleTimestamp ackByAppTimestamp;
	
	/**
	 * Creates a Status Report linked to a Bundle
	 * @param bundle The bundle where the Status Report is inserted into
	 */
	StatusReport(Bundle bundle) {
		this.bundle = bundle;
	}
	
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + ((ackByAppTimestamp == null) ? 0 : ackByAppTimestamp.hashCode());
		result = prime * result + ((bundle == null) ? 0 : bundle.hashCode());
		result = prime * result + ((creationTimestamp == null) ? 0 : creationTimestamp.hashCode());
		result = prime * result + ((custodyTimestamp == null) ? 0 : custodyTimestamp.hashCode());
		result = prime * result + ((deletionTimestamp == null) ? 0 : deletionTimestamp.hashCode());
		result = prime * result + ((deliveryTimestamp == null) ? 0 : deliveryTimestamp.hashCode());
		result = prime * result + ((flags == null) ? 0 : flags.hashCode());
		result = prime * result + ((forwardingTimestamp == null) ? 0 : forwardingTimestamp.hashCode());
		result = prime * result + fragmentOffset;
		result = prime * result + origLength;
		result = prime * result + ((reason == null) ? 0 : reason.hashCode());
		result = prime * result + ((receiptTimestamp == null) ? 0 : receiptTimestamp.hashCode());
		result = prime * result + ((source == null) ? 0 : source.hashCode());
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
		if (!(obj instanceof StatusReport)) {
			return false;
		}
		StatusReport other = (StatusReport) obj;
		if (ackByAppTimestamp == null) {
			if (other.ackByAppTimestamp != null) {
				return false;
			}
		} else if (!ackByAppTimestamp.equals(other.ackByAppTimestamp)) {
			return false;
		}
		if (bundle == null) {
			if (other.bundle != null) {
				return false;
			}
		} else if (!bundle.equals(other.bundle)) {
			return false;
		}
		if (creationTimestamp == null) {
			if (other.creationTimestamp != null) {
				return false;
			}
		} else if (!creationTimestamp.equals(other.creationTimestamp)) {
			return false;
		}
		if (custodyTimestamp == null) {
			if (other.custodyTimestamp != null) {
				return false;
			}
		} else if (!custodyTimestamp.equals(other.custodyTimestamp)) {
			return false;
		}
		if (deletionTimestamp == null) {
			if (other.deletionTimestamp != null) {
				return false;
			}
		} else if (!deletionTimestamp.equals(other.deletionTimestamp)) {
			return false;
		}
		if (deliveryTimestamp == null) {
			if (other.deliveryTimestamp != null) {
				return false;
			}
		} else if (!deliveryTimestamp.equals(other.deliveryTimestamp)) {
			return false;
		}
		if (flags != other.flags) {
			return false;
		}
		if (forwardingTimestamp == null) {
			if (other.forwardingTimestamp != null) {
				return false;
			}
		} else if (!forwardingTimestamp.equals(other.forwardingTimestamp)) {
			return false;
		}
		if (fragmentOffset != other.fragmentOffset) {
			return false;
		}
		if (origLength != other.origLength) {
			return false;
		}
		if (reason != other.reason) {
			return false;
		}
		if (receiptTimestamp == null) {
			if (other.receiptTimestamp != null) {
				return false;
			}
		} else if (!receiptTimestamp.equals(other.receiptTimestamp)) {
			return false;
		}
		if (source == null) {
			if (other.source != null) {
				return false;
			}
		} else if (!source.equals(other.source)) {
			return false;
		}
		return true;
	}

	/**
	 * Returns the bundle which this status report is referred to
	 * @return the bundle which this status report is referred to
	 */
	public Bundle getReferredBundle() {
		return bundle;
	}

	public BundleEID getSource() {
		return source;
	}

	void setSource(BundleEID source) {
		this.source = source;
	}

	public BundleTimestamp getCreationTimestamp() {
		return creationTimestamp;
	}

	void setCreationTimestamp(BundleTimestamp creationTimestamp) {
		this.creationTimestamp = creationTimestamp;
	}

	public int getFragmentOffset() {
		return fragmentOffset;
	}

	void setFragmentOffset(int fragmentOffset) {
		this.fragmentOffset = fragmentOffset;
	}

	public int getOrigLength() {
		return origLength;
	}

	void setOrigLength(int origLength) {
		this.origLength = origLength;
	}

	public StatusReportReason getReason() {
		return reason;
	}

	void setReason(StatusReportReason reason) {
		this.reason = reason;
	}

	public List<StatusReportFlag> getFlags() {
		return new LinkedList<>(this.flags);
	}

	void setFlags(List<StatusReportFlag> flags) {
		this.flags = new LinkedList<>(flags);
	}

	public BundleTimestamp getReceiptTimestamp() {
		return receiptTimestamp;
	}

	void setReceiptTimestamp(BundleTimestamp receiptTimestamp) {
		this.receiptTimestamp = receiptTimestamp;
	}

	public BundleTimestamp getCustodyTimestamp() {
		return custodyTimestamp;
	}

	void setCustodyTimestamp(BundleTimestamp custodyTimestamp) {
		this.custodyTimestamp = custodyTimestamp;
	}

	public BundleTimestamp getForwardingTimestamp() {
		return forwardingTimestamp;
	}

	void setForwardingTimestamp(BundleTimestamp forwardingTimestamp) {
		this.forwardingTimestamp = forwardingTimestamp;
	}

	public BundleTimestamp getDeliveryTimestamp() {
		return deliveryTimestamp;
	}

	void setDeliveryTimestamp(BundleTimestamp deliveryTimestamp) {
		this.deliveryTimestamp = deliveryTimestamp;
	}

	public BundleTimestamp getDeletionTimestamp() {
		return deletionTimestamp;
	}

	void setDeletionTimestamp(BundleTimestamp deletionTimestamp) {
		this.deletionTimestamp = deletionTimestamp;
	}

	public BundleTimestamp getAckByAppTimestamp() {
		return ackByAppTimestamp;
	}

	void setAckByAppTimestamp(BundleTimestamp ackByAppTimestamp) {
		this.ackByAppTimestamp = ackByAppTimestamp;
	}

	
}
