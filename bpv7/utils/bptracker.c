#include "bpP.h"
#include "sdr.h"
#include <signal.h>
#include <pthread.h>

/*	Intermediate tracking with suspend/resume added */
typedef struct {
    Object      bundleObj;          /* ION bundle object reference */
    Object      trackingElt;        /* SDR list element for tracking */
    char        destEid[64];        /* Destination endpoint ID */
    time_t      sendTime;           /* When bundle was sent */
    int         bundleId;           /* Sequential bundle identifier */
    enum {
        BUNDLE_PENDING,             /* Awaiting transmission */
        BUNDLE_SUSPENDED,           /* Transmission suspended */
        BUNDLE_TRANSMITTED,         /* Successfully transmitted, now detained */
        BUNDLE_COMPLETED            /* Manually released */
    } status;
    
    /* Add just a few more fields gradually */
    int         inQueues;           /* Boolean: still in transmission queues */
    time_t      transmitTime;       /* When transmission completed */
} TrackedBundle;

typedef struct {
    BpSAP       sap;               /* Bundle Protocol SAP */
    Object      trackedBundles;    /* SDR list of TrackedBundle objects */
    int         totalSent;         /* Total bundles sent */
    int         totalTransmitted;  /* Total bundles transmitted (not released) */
    int         totalCompleted;    /* Total bundles manually released */
    char        destEid[64];       /* Target destination EID */
} BundleTracker;

static volatile int g_quit = 0;

void signalHandler(int sig)
{
    printf("\nReceived signal %d. Shutting down...\n", sig);
    fflush(stdout);
    g_quit = 1;
}

/*	Convert bundle status to string	*/
const char *statusToString(int status)
{
    switch (status) {
        case BUNDLE_PENDING:     return "PENDING";
        case BUNDLE_SUSPENDED:   return "SUSPENDED";
        case BUNDLE_TRANSMITTED: return "TRANSMITTED";
        case BUNDLE_COMPLETED:   return "COMPLETED";
        default:                 return "UNKNOWN";
    }
}

/*	Suspend a specific bundle by ID	*/
int suspendBundle(BundleTracker *tracker, int bundleId)
{
    Sdr sdr = getIonsdr();
    Object elt;
    Object tbundleObj;
    TrackedBundle tbundle;
    int found = 0;
    
    printf("DEBUG: Attempting to suspend bundle %d\n", bundleId);
    fflush(stdout);
    
    if (sdr_begin_xn(sdr) < 0) {
        printf("ERROR: Can't begin suspend transaction\n");
        return -1;
    }
    
    for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; 
         elt = sdr_list_next(sdr, elt)) {
        tbundleObj = sdr_list_data(sdr, elt);
        
        if (tbundleObj == 0) continue;
        
        sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
        
        if (tbundle.bundleId == bundleId) {
            found = 1;
            printf("DEBUG: Found bundle %d for suspension\n", bundleId);
            fflush(stdout);
            
            if (tbundle.status == BUNDLE_COMPLETED) {
                printf("Bundle %d is already completed\n", bundleId);
            } else if (tbundle.status == BUNDLE_SUSPENDED) {
                printf("Bundle %d is already suspended\n", bundleId);
            } else if (tbundle.status == BUNDLE_TRANSMITTED) {
                printf("Bundle %d is already transmitted (cannot suspend)\n", bundleId);
            } else if (tbundle.bundleObj != 0) {
                printf("DEBUG: Calling bp_suspend for bundle %d\n", bundleId);
                fflush(stdout);
                
                /* Use ION's built-in suspend function */
                if (bp_suspend(tbundle.bundleObj) == 0) {
                    printf("Bundle %d suspended successfully\n", bundleId);
                    tbundle.status = BUNDLE_SUSPENDED;
                    sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
                } else {
                    printf("Failed to suspend bundle %d\n", bundleId);
                }
            }
            break;
        }
    }
    
    if (sdr_end_xn(sdr) < 0) {
        printf("ERROR: Can't complete suspend transaction\n");
        return -1;
    }
    
    if (!found) {
        printf("Bundle %d not found\n", bundleId);
        return 0;
    }
    
    return 1;
}

/*	Resume a specific bundle by ID	*/
int resumeBundle(BundleTracker *tracker, int bundleId)
{
    Sdr sdr = getIonsdr();
    Object elt;
    Object tbundleObj;
    TrackedBundle tbundle;
    int found = 0;
    
    printf("DEBUG: Attempting to resume bundle %d\n", bundleId);
    fflush(stdout);
    
    if (sdr_begin_xn(sdr) < 0) {
        printf("ERROR: Can't begin resume transaction\n");
        return -1;
    }
    
    for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; 
         elt = sdr_list_next(sdr, elt)) {
        tbundleObj = sdr_list_data(sdr, elt);
        
        if (tbundleObj == 0) continue;
        
        sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
        
        if (tbundle.bundleId == bundleId) {
            found = 1;
            printf("DEBUG: Found bundle %d for resumption\n", bundleId);
            fflush(stdout);
            
            if (tbundle.status == BUNDLE_COMPLETED) {
                printf("Bundle %d is already completed\n", bundleId);
            } else if (tbundle.status == BUNDLE_TRANSMITTED) {
                printf("Bundle %d is already transmitted (cannot resume)\n", bundleId);
            } else if (tbundle.status != BUNDLE_SUSPENDED) {
                printf("Bundle %d is not suspended (status: %s)\n", 
                       bundleId, statusToString(tbundle.status));
            } else if (tbundle.bundleObj != 0) {
                printf("DEBUG: Calling bp_resume for bundle %d\n", bundleId);
                fflush(stdout);
                
                /* Use ION's built-in resume function */
                if (bp_resume(tbundle.bundleObj) == 0) {
                    printf("Bundle %d resumed successfully\n", bundleId);
                    tbundle.status = BUNDLE_PENDING;
                    sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
                } else {
                    printf("Failed to resume bundle %d\n", bundleId);
                }
            }
            break;
        }
    }
    
    if (sdr_end_xn(sdr) < 0) {
        printf("ERROR: Can't complete resume transaction\n");
        return -1;
    }
    
    if (!found) {
        printf("Bundle %d not found\n", bundleId);
        return 0;
    }
    
    return 1;
}

/*	Create a payload of specified size	*/
Object createPayload(int payloadSize)
{
    Sdr sdr = getIonsdr();
    Object payloadObj;
    char *buffer;
    int i;
    
    printf("DEBUG: Creating payload of size %d\n", payloadSize);
    fflush(stdout);
    
    if (payloadSize <= 0 || payloadSize > 65536) {
        printf("ERROR: Invalid payload size %d\n", payloadSize);
        return 0;
    }
    
    buffer = malloc(payloadSize);
    if (buffer == NULL) {
        printf("ERROR: Can't allocate payload buffer\n");
        return 0;
    }
    
    /* Fill buffer with test pattern */
    for (i = 0; i < payloadSize; i++) {
        buffer[i] = (char)(i % 256);
    }
    
    if (sdr_begin_xn(sdr) < 0) {
        free(buffer);
        printf("ERROR: Can't begin payload transaction\n");
        return 0;
    }
    
    payloadObj = sdr_malloc(sdr, payloadSize);
    if (payloadObj == 0) {
        sdr_cancel_xn(sdr);
        free(buffer);
        printf("ERROR: No SDR space for payload\n");
        return 0;
    }
    
    sdr_write(sdr, payloadObj, buffer, payloadSize);
    payloadObj = zco_create(sdr, ZcoSdrSource, payloadObj, 0, payloadSize, ZcoOutbound);
    
    if (sdr_end_xn(sdr) < 0 || payloadObj == 0) {
        free(buffer);
        printf("ERROR: Can't create payload ZCO\n");
        return 0;
    }
    
    free(buffer);
    printf("DEBUG: Payload created successfully\n");
    fflush(stdout);
    return payloadObj;
}

/*	Send a bundle with tracking	*/
int sendTrackedBundle(BundleTracker *tracker, Object adu, int lifespan, int bundleId)
{
    Sdr sdr = getIonsdr();
    Object bundleObj;
    TrackedBundle tbundle;
    Object tbundleObj;
    Object trackingElt;
    
    printf("DEBUG: Sending bundle %d\n", bundleId);
    fflush(stdout);
    
    /* Send bundle with detention */
    if (bp_send(tracker->sap, tracker->destEid, NULL, lifespan, 0, 
                NoCustodyRequested, 0, 0, NULL, adu, &bundleObj) != 1) {
        printf("ERROR: Failed to send bundle to %s\n", tracker->destEid);
        return -1;
    }
    
    printf("DEBUG: Bundle %d sent successfully (obj: " ADDR_FIELDSPEC ")\n", 
           bundleId, bundleObj);
    fflush(stdout);
    
    if (sdr_begin_xn(sdr) < 0) {
        printf("ERROR: Can't begin tracking transaction\n");
        return -1;
    }
    
    /* Create tracking bundle object */
    tbundleObj = sdr_malloc(sdr, sizeof(TrackedBundle));
    if (tbundleObj == 0) {
        sdr_cancel_xn(sdr);
        printf("ERROR: No space for tracked bundle\n");
        bp_cancel(bundleObj);
        return -1;
    }
    
    printf("DEBUG: Creating tracking record for bundle %d\n", bundleId);
    fflush(stdout);
    
    /* Initialize tracking data */
    tbundle.bundleObj = bundleObj;
    tbundle.sendTime = time(NULL);
    tbundle.transmitTime = 0;  /* Not transmitted yet */
    tbundle.status = BUNDLE_PENDING;
    tbundle.bundleId = bundleId;
    tbundle.inQueues = 1;      /* Assume in queues initially */
    strncpy(tbundle.destEid, tracker->destEid, sizeof(tbundle.destEid) - 1);
    tbundle.destEid[sizeof(tbundle.destEid) - 1] = '\0';
    
    /* Add to tracking list */
    trackingElt = sdr_list_insert_last(sdr, tracker->trackedBundles, tbundleObj);
    if (trackingElt == 0) {
        sdr_cancel_xn(sdr);
        printf("ERROR: Can't add bundle to tracking list\n");
        sdr_free(sdr, tbundleObj);
        bp_cancel(bundleObj);
        return -1;
    }
    
    tbundle.trackingElt = trackingElt;
    sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
    
    printf("DEBUG: Registering bundle tracking with ION\n");
    fflush(stdout);
    
    /* Register tracking with ION */
    if (bp_track(bundleObj, trackingElt) < 0) {
        sdr_cancel_xn(sdr);
        printf("ERROR: Can't register bundle tracking\n");
        sdr_list_delete(sdr, trackingElt, NULL, NULL);
        sdr_free(sdr, tbundleObj);
        bp_cancel(bundleObj);
        return -1;
    }
    
    tracker->totalSent++;
    
    if (sdr_end_xn(sdr) < 0) {
        printf("ERROR: Can't complete tracked bundle send\n");
        return -1;
    }
    
    printf("DEBUG: Bundle %d tracking completed\n", bundleId);
    fflush(stdout);
    
    return 0;
}

/*	Check status of all tracked bundles - with suspend/resume awareness	*/
int checkBundleStatus(BundleTracker *tracker)
{
    Sdr sdr = getIonsdr();
    Object elt, nextElt;
    Object tbundleObj;
    TrackedBundle tbundle;
    Bundle bundle;
    int bundlesActive = 0;
    time_t currentTime = time(NULL);
    
    printf("DEBUG: Checking bundle status (with suspend/resume)...\n");
    fflush(stdout);
    
    if (sdr_begin_xn(sdr) < 0) {
        printf("ERROR: Can't begin status check transaction\n");
        return -1;
    }
    
    for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; elt = nextElt) {
        nextElt = sdr_list_next(sdr, elt);
        tbundleObj = sdr_list_data(sdr, elt);
        
        if (tbundleObj == 0) {
            printf("DEBUG: Found null tracking object, removing\n");
            fflush(stdout);
            sdr_list_delete(sdr, elt, NULL, NULL);
            continue;
        }
        
        sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
        
        printf("DEBUG: Checking bundle %d (status=%s)\n", 
               tbundle.bundleId, statusToString(tbundle.status));
        fflush(stdout);
        
        /* Skip already processed bundles */
        if (tbundle.status == BUNDLE_COMPLETED) {
            continue;
        }
        
        /* Skip suspended bundles - don't check transmission status */
        if (tbundle.status == BUNDLE_SUSPENDED) {
            bundlesActive++;
            continue;
        }
        
        /* Try to read bundle object to check if it still exists */
        if (tbundle.bundleObj == 0) {
            printf("DEBUG: Bundle %d object is null - marking completed\n", tbundle.bundleId);
            fflush(stdout);
            tbundle.status = BUNDLE_COMPLETED;
            tracker->totalCompleted++;
            sdr_free(sdr, tbundleObj);
            sdr_list_delete(sdr, elt, NULL, NULL);
            continue;
        }
        
        printf("DEBUG: Reading bundle %d object details\n", tbundle.bundleId);
        fflush(stdout);
        
        /* Read bundle to check current status */
        memset(&bundle, 0, sizeof(Bundle));
        sdr_read(sdr, (char *) &bundle, tbundle.bundleObj, sizeof(Bundle));
        
        printf("DEBUG: Bundle %d object read successfully\n", tbundle.bundleId);
        fflush(stdout);
        
        /* Check queue status - simplified version */
        int wasInQueues = tbundle.inQueues;
        tbundle.inQueues = (bundle.planXmitElt != 0 || bundle.ductXmitElt != 0 || 
                           bundle.fwdQueueElt != 0 || bundle.dlvQueueElt != 0);
        
        printf("DEBUG: Bundle %d queue check: wasInQueues=%d, nowInQueues=%d\n", 
               tbundle.bundleId, wasInQueues, tbundle.inQueues);
        fflush(stdout);
        
        /* Detect transmission completion */
        if (wasInQueues && !tbundle.inQueues && tbundle.status == BUNDLE_PENDING) {
            printf("Bundle %d transmission completed (with suspend/resume)\n", tbundle.bundleId);
            fflush(stdout);
            
            printf("DEBUG: About to update bundle %d status to TRANSMITTED\n", tbundle.bundleId);
            fflush(stdout);
            
            tbundle.status = BUNDLE_TRANSMITTED;
            tbundle.transmitTime = currentTime;
            tracker->totalTransmitted++;
            
            printf("DEBUG: Updating bundle %d tracking record\n", tbundle.bundleId);
            fflush(stdout);
            
            sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
            
            printf("DEBUG: Bundle %d status update completed\n", tbundle.bundleId);
            fflush(stdout);
        } else {
            /* Just update the inQueues status */
            printf("DEBUG: Updating bundle %d queue status only\n", tbundle.bundleId);
            fflush(stdout);
            
            sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
        }
        
        /* Count active bundles */
        if (tbundle.status == BUNDLE_PENDING || tbundle.status == BUNDLE_SUSPENDED || 
            tbundle.status == BUNDLE_TRANSMITTED) {
            bundlesActive++;
        }
    }
    
    printf("DEBUG: About to end status check transaction\n");
    fflush(stdout);
    
    if (sdr_end_xn(sdr) < 0) {
        printf("ERROR: Can't end status check transaction\n");
        return -1;
    }
    
    printf("DEBUG: Status check completed successfully, active bundles: %d\n", bundlesActive);
    fflush(stdout);
    
    return bundlesActive;
}

/*	List all tracked bundles	*/
void listActiveBundles(BundleTracker *tracker)
{
    Sdr sdr = getIonsdr();
    Object elt;
    Object tbundleObj;
    TrackedBundle tbundle;
    time_t currentTime = time(NULL);
    int totalCount = 0;
    
    printf("\n=== Tracked Bundles (With Suspend/Resume) ===\n");
    printf("%-3s %-12s %-7s %-15s\n", "ID", "Status", "Age(s)", "Object_Addr");
    printf("%-3s %-12s %-7s %-15s\n", "---", "------------", "-------", "---------------");
    
    if (sdr_begin_xn(sdr) < 0) {
        printf("ERROR: Can't begin list transaction\n");
        return;
    }
    
    for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; 
         elt = sdr_list_next(sdr, elt)) {
        tbundleObj = sdr_list_data(sdr, elt);
        
        if (tbundleObj == 0) continue;
        
        sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
        totalCount++;
        
        printf("%-3d %-12s %-7ld " ADDR_FIELDSPEC "\n", 
               tbundle.bundleId, 
               statusToString(tbundle.status),
               currentTime - tbundle.sendTime,
               tbundle.bundleObj);
        
        /* Show additional info for transmitted bundles */
        if (tbundle.status == BUNDLE_TRANSMITTED && tbundle.transmitTime > 0) {
            printf("    └─ Transmitted: %ld seconds ago\n", 
                   currentTime - tbundle.transmitTime);
        }
    }
    
    sdr_exit_xn(sdr);
    
    if (totalCount == 0) {
        printf("No bundles tracked\n");
    }
    printf("=== Total: %d bundles ===\n\n", totalCount);
}

/*	Print current status	*/
void printStatus(BundleTracker *tracker)
{
    int active = checkBundleStatus(tracker);
    printf("\n=== Bundle Tracker Status (With Suspend/Resume) ===\n");
    printf("Destination: %s\n", tracker->destEid);
    printf("Total sent: %d\n", tracker->totalSent);
    printf("Transmitted (detained): %d\n", tracker->totalTransmitted);
    printf("Manually released: %d\n", tracker->totalCompleted);
    printf("Still active: %d\n", active);
    printf("====================================================\n\n");
}

void* inputHandler(void* arg)
{
    char input[100];
    int bundleId;
    BundleTracker *tracker = (BundleTracker*)arg;
    
    sleep(2);  /* Let main thread settle */
    
    while (!g_quit) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (g_quit) break;
            sleep(1);
            continue;
        }
        
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "q") == 0 || strcmp(input, "quit") == 0) {
            printf("Quitting...\n");
            fflush(stdout);
            g_quit = 1;
            break;
        } else if (strcmp(input, "s") == 0 || strcmp(input, "status") == 0) {
            printStatus(tracker);
        } else if (strcmp(input, "l") == 0 || strcmp(input, "list") == 0) {
            listActiveBundles(tracker);
        } else if (sscanf(input, "suspend %d", &bundleId) == 1 ||
                   sscanf(input, "sp %d", &bundleId) == 1) {
            suspendBundle(tracker, bundleId);
        } else if (sscanf(input, "resume %d", &bundleId) == 1 ||
                   sscanf(input, "res %d", &bundleId) == 1) {
            resumeBundle(tracker, bundleId);
        } else if (strcmp(input, "h") == 0 || strcmp(input, "help") == 0) {
            printf("\nAvailable commands:\n");
            printf("  q, quit                  - Quit program\n");
            printf("  s, status                - Show current status\n");
            printf("  l, list                  - List all bundles with status\n");
            printf("  suspend <id>, sp <id>    - Suspend specific bundle by ID\n");
            printf("  resume <id>, res <id>    - Resume specific bundle by ID\n");
            printf("  h, help                  - Show this help\n\n");
            printf("Bundle Status Meanings:\n");
            printf("  PENDING       - Bundle queued for transmission\n");
            printf("  SUSPENDED     - Bundle transmission suspended\n");
            printf("  TRANSMITTED   - Bundle transmitted by CLA, now detained\n");
            printf("  COMPLETED     - Bundle destroyed/completed\n\n");
            fflush(stdout);
        } else if (strlen(input) > 0) {
            printf("Unknown command: %s (type 'h' for help)\n", input);
            fflush(stdout);
        }
    }
    
    return NULL;
}

int main(int argc, char *argv[])
{
    BundleTracker tracker;
    char *sourceEid, *destEid;
    int payloadSize, numBundles;
    Object *payloads;
    int i;
    pthread_t inputThread;
    
    if (argc != 5) {
        printf("Usage: %s <source_eid> <dest_eid> <payload_size> <num_bundles>\n", argv[0]);
        return 1;
    }
    
    sourceEid = argv[1];
    destEid = argv[2];
    payloadSize = atoi(argv[3]);
    numBundles = atoi(argv[4]);
    
    printf("Bundle Tracker with Suspend/Resume Starting...\n");
    printf("Source: %s -> Destination: %s\n", sourceEid, destEid);
    printf("Payload size: %d bytes, Bundles: %d\n", payloadSize, numBundles);
    fflush(stdout);
    
    /* Initialize Bundle Protocol */
    printf("DEBUG: Attaching to Bundle Protocol\n");
    fflush(stdout);
    
    if (bpAttach() < 0) {
        printf("ERROR: Can't attach to Bundle Protocol\n");
        return 1;
    }
    
    /* Set up signal handlers */
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    /* Open SAP with detention */
    printf("DEBUG: Opening SAP with detention enabled\n");
    fflush(stdout);
    
    if (bp_open_source(sourceEid, &(tracker.sap), 1) < 0) {
        printf("ERROR: Can't open source endpoint with detention\n");
        bpDetach();
        return 1;
    }
    
    /* Initialize tracker */
    Sdr sdr = getIonsdr();
    if (sdr_begin_xn(sdr) < 0) {
        printf("ERROR: Can't begin tracker init transaction\n");
        bp_close(tracker.sap);
        bpDetach();
        return 1;
    }
    
    tracker.trackedBundles = sdr_list_create(sdr);
    if (tracker.trackedBundles == 0) {
        sdr_cancel_xn(sdr);
        printf("ERROR: Can't create tracked bundles list\n");
        bp_close(tracker.sap);
        bpDetach();
        return 1;
    }
    
    tracker.totalSent = 0;
    tracker.totalTransmitted = 0;
    tracker.totalCompleted = 0;
    strncpy(tracker.destEid, destEid, sizeof(tracker.destEid) - 1);
    tracker.destEid[sizeof(tracker.destEid) - 1] = '\0';
    
    if (sdr_end_xn(sdr) < 0) {
        printf("ERROR: Can't complete tracker initialization\n");
        bp_close(tracker.sap);
        bpDetach();
        return 1;
    }
    
    /* Create payloads */
    payloads = malloc(numBundles * sizeof(Object));
    if (payloads == NULL) {
        printf("ERROR: Can't allocate payload array\n");
        bp_close(tracker.sap);
        bpDetach();
        return 1;
    }
    
    printf("DEBUG: Creating %d payloads\n", numBundles);
    fflush(stdout);
    
    for (i = 0; i < numBundles; i++) {
        payloads[i] = createPayload(payloadSize);
        if (payloads[i] == 0) {
            printf("ERROR: Failed to create payload %d\n", i + 1);
            free(payloads);
            bp_close(tracker.sap);
            bpDetach();
            return 1;
        }
    }
    
    /* Send bundles */
    printf("DEBUG: Sending %d bundles with detention enabled\n", numBundles);
    fflush(stdout);
    
    for (i = 0; i < numBundles; i++) {
        if (sendTrackedBundle(&tracker, payloads[i], 300, i + 1) < 0) {
            printf("ERROR: Failed to send bundle %d\n", i + 1);
            g_quit = 1;
            break;
        }
        usleep(100000);  /* 100ms delay between sends */
    }
    
    free(payloads);
    
    if (g_quit) {
        printf("ERROR: Exiting due to send failure\n");
        bp_close(tracker.sap);
        bpDetach();
        return 1;
    }
    
    /* Start input handler thread */
    printf("DEBUG: Starting input thread\n");
    fflush(stdout);
    
    if (pthread_create(&inputThread, NULL, inputHandler, &tracker) != 0) {
        printf("ERROR: Can't create input thread\n");
        g_quit = 1;
    }
    
    /* Monitor bundle status */
    printf("DEBUG: Entering monitoring loop\n");
    fflush(stdout);
    
    while (!g_quit) {
        int result = checkBundleStatus(&tracker);
        if (result < 0) {
            printf("ERROR: Status check failed, continuing...\n");
            fflush(stdout);
        }
        sleep(5);  /* Check every 5 seconds */
    }
    
    /* Cleanup */
    printf("DEBUG: Cleaning up\n");
    fflush(stdout);
    
    if (!g_quit) g_quit = 1;
    pthread_cancel(inputThread);
    pthread_join(inputThread, NULL);
    
    bp_close(tracker.sap);
    bpDetach();
    
    printf("DEBUG: Program completed\n");
    fflush(stdout);
    return 0;
}