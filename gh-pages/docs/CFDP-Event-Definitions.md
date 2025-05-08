# CFDP Event Field Meanings Reference

## Event Type Codes
- **0**: CfdpNoEvent (internal interrupt, not relevent to user)
- **1**: CfdpTransactionInd (transaction started)
- **2**: CfdpEofSentInd (EOF sent)
- **3**: CfdpTransactionFinishedInd (transaction finished)
- **4**: CfdpMetadataRecvInd (metadata received)
- **5**: CfdpFileSegmentRecvInd (file data segment received)
- **6**: CfdpEofRecvInd (EOF received)
- **7**: CfdpSuspendedInd (suspended)
- **8**: CfdpResumedInd (resumed)
- **9**: CfdpReportInd (transaction report)
- **10**: CfdpFaultInd (fault)
- **11**: CfdpAbandonedInd (abandoned)

---

The following is a list (non-exhaustive) of CFDP events and state information available through the `cfdptest` test utility or using `cfdp_get_event()` API.

## Field Meanings by Event Type

### **Event 1: CfdpTransactionInd (Transaction Started)**
**Occurs:** When sender initiates a file transfer

| Field | Typical Values | Meaning |
|-------|----------------|---------|
| **Condition** | `0` (CfdpNoError) | Transaction setup successful |
| **Progress** | `0` bytes | No data sent yet |

---

### **Event 2: CfdpEofSentInd (EOF Sent)**
**Occurs:** When sender finishes transmitting all file data

| Field | Typical Values | Meaning |
|-------|----------------|---------|
| **Condition** | `0` (CfdpNoError) | File data transmission completed successfully |
| **Progress** | File size | All file data has been transmitted (at least once) |

**Key Insight:** This event means "I sent everything" but doesn't guarantee receiver got it.

---

### **Event 3: CfdpTransactionFinishedInd (Transaction Finished)** 
**Occurs:** When transaction completes (most important event for diagnostics)

#### **Acknowledged Mode (closureLatency > 0):**

| Condition | DeliveryCode | FileStatus | Meaning |
|-----------|--------------|------------|---------|
| **0** (NoError) | **0** (Complete) | **2** (Retained) | **SUCCESS** - Finish PDU received (sender), file delivered/received and saved (receiver) |
| **0** (NoError) | **0** (Complete) | **0** (Discarded) | File delivered (sender) but receiver discarded it (checksum/policy issue) |
| **0** (NoError) | **1** (Incomplete) | **3** (Unreported) | Finish PDU received (sender) but receiver reports problems |
| **10** (CheckLimitReached) | **1** (Incomplete) | **3** (Unreported) | **TIMEOUT** - Finish PDU never received (sender) |
| **5** (ChecksumFailure) | **1** (Incomplete) | **0** (Discarded) | Receiver detected file corruption (receiver) |
| **15** (CancelRequested) | **1** (Incomplete) | **3** (Unreported) | User cancelled transaction (sender/receiver) |

#### **Unacknowledged Mode (closureLatency = 0):**

| Condition | DeliveryCode | FileStatus | Meaning |
|-----------|--------------|------------|---------|
| **0** (NoError) | **1** (Incomplete) | **3** (Unreported) | **NORMAL (sender side)** - File sent successfully, no confirmation expected at sender side |
| **0** (NoError) | **0** (Complete) | **2** (Retained) | **SUCCESS (receiver side)** - File sent successfully, file saved |
| **15** (CancelRequested) | **1** (Incomplete) | **3** (Unreported) | ❌ User cancelled transaction |

**Key Insight:** Only `condition=0` + `deliveryCode=0` + `fileStatus=2` means guaranteed success!

---

### **Event 4: CfdpMetadataRecvInd (Metadata Received)**
**Occurs:** When receiver gets file transfer metadata (receiver side)

| Field | Typical Values | Meaning |
|-------|----------------|---------|
| **Condition** | `0` (CfdpNoError) | Metadata processed successfully |
| **Progress** | `0` bytes | No file data received yet |

---

### **Event 5: CfdpFileSegmentRecvInd (File Data Received)**
**Occurs:** Periodically as receiver gets file data chunks

| Field | Typical Values | Meaning |
|-------|----------------|---------|
| **Condition** | `0` (CfdpNoError) | Data segment received and processed |
| **Progress** | Increasing | Bytes received so far |

---

### **Event 6: CfdpEofRecvInd (EOF Received)**
**Occurs:** When receiver gets EOF PDU (receiver side)

| Field | Possible Values | Meaning |
|-------|----------------|---------|
| **Condition** | `0` (CfdpNoError) | EOF received, checking file completeness |
| **Condition** | `5` (ChecksumFailure) | File corruption detected |
| **Condition** | `9` (InvalidFileStructure) | File structure problems |
| **Progress** | Expected file size | Total bytes that should have been received |

---

### **Event 7: CfdpSuspendedInd (Suspended)**
**Occurs:** When transaction is paused

| Field | Typical Values | Meaning |
|-------|----------------|---------|
| **Condition** | `14` (SuspendRequested) | User/system requested suspension |
| **DeliveryCode** | `1` (Incomplete) | Transfer paused |
| **FileStatus** | `3` (Unreported) | Transfer incomplete |
| **Progress** | Current bytes | Progress when suspended |

---

### **Event 8: CfdpResumedInd (Resumed)**
**Occurs:** When suspended transaction restarts

| Field | Typical Values | Meaning |
|-------|----------------|---------|
| **Condition** | `0` (CfdpNoError) | Resume successful |
| **DeliveryCode** | `1` (Incomplete) | Transfer continuing |
| **FileStatus** | `3` (Unreported) | Transfer still in progress |
| **Progress** | Resume point | Bytes completed when resumed |

---

### **Event 9: CfdpReportInd (Transaction Report)**
**Occurs:** When user requests transaction status

| Field | Meaning |
|-------|---------|
| **Condition** | Current transaction condition |
| **Progress** | Current progress |

**Key Insight:** This is a snapshot of current transaction state, not an event-triggered change.

---

### **Event 10: CfdpFaultInd (Fault)**
**Occurs:** When recoverable errors occur

| Condition | Meaning |
|-----------|---------|
| **1** (AckLimitReached) | Too many retransmissions |
| **4** (FilestoreRejection) | Receiver rejected file operation |
| **5** (ChecksumFailure) | Data corruption detected |
| **6** (FileSizeError) | File size mismatch |
| **8** (InactivityDetected) | No progress for too long |

**DeliveryCode/FileStatus:** Reflect current state when fault occurred.

---

### **Event 11: CfdpAbandonedInd (Abandoned)**
**Occurs:** When transaction cannot recover from faults

| Field | Typical Values | Meaning |
|-------|----------------|---------|
| **Condition** | Various fault codes | The fault that caused abandonment |
| **DeliveryCode** | `1` (Incomplete) | Transfer failed |
| **FileStatus** | `0` (Discarded) or `3` (Unreported) | File not delivered |
| **Progress** | Last known progress | How much was completed before failure |

---

## Condition Code Reference

| Code | Name | Meaning |
|------|------|---------|
| **0** | CfdpNoError | Success/Normal operation |
| **1** | CfdpAckLimitReached | Too many ACK retries |
| **2** | CfdpKeepaliveLimitReached | Keepalive timeout |
| **3** | CfdpInvalidTransmissionMode | Wrong CFDP mode |
| **4** | CfdpFilestoreRejection | File operation rejected |
| **5** | CfdpChecksumFailure | Data corruption |
| **6** | CfdpFileSizeError | File size mismatch |
| **7** | CfdpNakLimitReached | Too many NAKs |
| **8** | CfdpInactivityDetected | No activity timeout |
| **9** | CfdpInvalidFileStructure | Malformed file |
| **10** | CfdpCheckLimitReached | **Finish PDU timeout** |
| **11** | CfdpUnsupportedChecksumType | Unknown checksum |
| **14** | CfdpSuspendRequested | User suspension |
| **15** | CfdpCancelRequested | User cancellation |

## DeliveryCode Reference

| Code | Name | Meaning |
|------|------|---------|
| **0** | CfdpDataComplete | All data successfully delivered |
| **1** | CfdpDataIncomplete | Data missing/incomplete/in-progress |

## FileStatus Reference

| Code | Name | Meaning |
|------|------|---------|
| **0** | CfdpFileDiscarded | File was discarded (error/policy) |
| **1** | CfdpFileRejected | File delivery was rejected |
| **2** | CfdpFileRetained | File successfully stored |
| **3** | CfdpFileStatusUnreported | Status unknown/not available |
