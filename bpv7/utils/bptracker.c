#include "bpP.h"
#include "sdr.h"
#include <signal.h>
#include <pthread.h>

/*	Application data structure for tracking bundles	*/
typedef struct {
    Object      bundleObj;          /* ION bundle object reference */
    Object      trackingElt;        /* SDR list element for tracking */
    char        destEid[64];        /* Destination endpoint ID */
    time_t      sendTime;           /* When bundle was sent */
    int         bundleId;           /* Sequential bundle identifier */
    enum {
        BUNDLE_PENDING,             /* Awaiting transmission */
        BUNDLE_SUSPENDED,           /* Transmission suspended */
        BUNDLE_COMPLETED,           /* Successfully transmitted */
        BUNDLE_RELEASED,            /* Manually released */
        BUNDLE_CANCELED             /* Explicitly canceled */
    } status;
} TrackedBundle;

typedef struct {
    BpSAP       sap;               /* Bundle Protocol SAP */
    Object      trackedBundles;    /* SDR list of TrackedBundle objects */
    int         totalSent;         /* Total bundles sent */
    int         totalCompleted;    /* Total bundles completed */
    int         totalReleased;     /* Total bundles manually released */
    char        destEid[64];       /* Target destination EID */
} BundleTracker;

/* Global tracker for signal handling */
static BundleTracker *g_tracker = NULL;
static volatile int g_quit = 0;

/*	Suspend a specific bundle by ID	*/
int suspendBundle(BundleTracker *tracker, int bundleId)
{
    Sdr sdr = getIonsdr();
    Object elt;
    Object tbundleObj;
    TrackedBundle tbundle;
    int found = 0;
    
    CHKERR(sdr_begin_xn(sdr));
    
    for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; 
         elt = sdr_list_next(sdr, elt)) {
        tbundleObj = sdr_list_data(sdr, elt);
        
        if (tbundleObj == 0) continue;
        
        sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
        
        if (tbundle.bundleId == bundleId) {
            found = 1;
            
            if (tbundle.status == BUNDLE_COMPLETED || tbundle.status == BUNDLE_RELEASED) {
                printf("Bundle %d is already processed\n", bundleId);
            } else if (tbundle.status == BUNDLE_SUSPENDED) {
                printf("Bundle %d is already suspended\n", bundleId);
            } else if (tbundle.bundleObj != 0) {
                /* Use ION's built-in suspend function */
                if (bp_suspend(tbundle.bundleObj) == 0) {
                    printf("Bundle %d suspended\n", bundleId);
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
        putErrmsg("Can't suspend bundle", NULL);
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
    
    CHKERR(sdr_begin_xn(sdr));
    
    for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; 
         elt = sdr_list_next(sdr, elt)) {
        tbundleObj = sdr_list_data(sdr, elt);
        
        if (tbundleObj == 0) continue;
        
        sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
        
        if (tbundle.bundleId == bundleId) {
            found = 1;
            
            if (tbundle.status == BUNDLE_COMPLETED || tbundle.status == BUNDLE_RELEASED) {
                printf("Bundle %d is already processed\n", bundleId);
            } else if (tbundle.status != BUNDLE_SUSPENDED) {
                printf("Bundle %d is not suspended\n", bundleId);
            } else if (tbundle.bundleObj != 0) {
                /* Use ION's built-in resume function */
                if (bp_resume(tbundle.bundleObj) == 0) {
                    printf("Bundle %d resumed\n", bundleId);
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
        putErrmsg("Can't resume bundle", NULL);
        return -1;
    }
    
    if (!found) {
        printf("Bundle %d not found\n", bundleId);
        return 0;
    }
    
    return 1;
}

/*	Signal handler for graceful shutdown	*/
void signalHandler(int sig)
{
    printf("\nReceived signal %d. Initiating graceful shutdown...\n", sig);
    g_quit = 1;
}

/*	Print usage information	*/
void printUsage(const char *progName)
{
    printf("Usage: %s <source_eid> <dest_eid> <payload_size> <num_bundles>\n\n", progName);
    printf("Parameters:\n");
    printf("  source_eid     - Source endpoint ID (e.g., ipn:1.1)\n");
    printf("  dest_eid       - Destination endpoint ID (e.g., ipn:2.1)\n");
    printf("  payload_size   - Size of bundle payload in bytes (1-65536)\n");
    printf("  num_bundles    - Number of bundles to send (1-10)\n\n");
    printf("Interactive Commands (while running):\n");
    printf("  'q' or 'quit'     - Release all bundles and quit\n");
    printf("  's' or 'status'   - Show current status\n");
    printf("  'r <id>' or 'release <id>' - Release specific bundle by ID\n");
    printf("  'ra' or 'release-all'      - Release all bundles\n");
    printf("  'l' or 'list'     - List all active bundles\n");
    printf("  'h' or 'help'     - Show this help\n\n");
    printf("Examples:\n");
    printf("  %s ipn:1.1 ipn:2.1 1024 5\n", progName);
    printf("  (Send 5 bundles of 1024 bytes each from ipn:1.1 to ipn:2.1)\n\n");
    printf("Interactive usage:\n");
    printf("  > list              # Show all bundles with LOCAL/REMOTE status\n");
    printf("  > suspend 2         # Suspend bundle 2 (if LOCAL)\n");
    printf("  > resume 2          # Resume bundle 2\n");
    printf("  > release 3         # Release bundle 3 from detention\n");
    printf("  > quit              # Release all and exit\n");
}

/*	Initialize bundle tracking system	*/
int initializeBundleTracker(BundleTracker *tracker, const char *sourceEid, 
                           const char *destEid)
{
    Sdr sdr = getIonsdr();
    
    /* Open SAP with detention enabled */
    if (bp_open_source((char*)sourceEid, &(tracker->sap), 1) < 0) {
        putErrmsg("Can't open source endpoint with detention", sourceEid);
        return -1;
    }
    
    /* Create SDR list to track bundles */
    CHKERR(sdr_begin_xn(sdr));
    tracker->trackedBundles = sdr_list_create(sdr);
    if (tracker->trackedBundles == 0) {
        sdr_cancel_xn(sdr);
        putErrmsg("Can't create tracked bundles list", NULL);
        return -1;
    }
    
    tracker->totalSent = 0;
    tracker->totalCompleted = 0;
    tracker->totalReleased = 0;
    strncpy(tracker->destEid, destEid, sizeof(tracker->destEid) - 1);
    tracker->destEid[sizeof(tracker->destEid) - 1] = '\0';
    
    if (sdr_end_xn(sdr) < 0) {
        putErrmsg("Can't initialize bundle tracker", NULL);
        return -1;
    }
    
    return 0;
}

/*	Create a payload of specified size	*/
Object createPayload(int payloadSize)
{
    Sdr sdr = getIonsdr();
    Object payloadObj;
    char *buffer;
    int i;
    
    if (payloadSize <= 0 || payloadSize > 65536) {
        putErrmsg("Invalid payload size", itoa(payloadSize));
        return 0;
    }
    
    buffer = malloc(payloadSize);
    if (buffer == NULL) {
        putErrmsg("Can't allocate payload buffer", NULL);
        return 0;
    }
    
    /* Fill buffer with test pattern */
    for (i = 0; i < payloadSize; i++) {
        buffer[i] = (char)(i % 256);
    }
    
    CHKZERO(sdr_begin_xn(sdr));
    payloadObj = sdr_malloc(sdr, payloadSize);
    if (payloadObj == 0) {
        sdr_cancel_xn(sdr);
        free(buffer);
        putErrmsg("No SDR space for payload", NULL);
        return 0;
    }
    
    sdr_write(sdr, payloadObj, buffer, payloadSize);
    payloadObj = zco_create(sdr, ZcoSdrSource, payloadObj, 0, payloadSize, ZcoOutbound);
    
    if (sdr_end_xn(sdr) < 0 || payloadObj == 0) {
        free(buffer);
        putErrmsg("Can't create payload ZCO", NULL);
        return 0;
    }
    
    free(buffer);
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
    
    /* Send bundle with detention */
    if (bp_send(tracker->sap, tracker->destEid, NULL, lifespan, 0, 
                NoCustodyRequested, 0, 0, NULL, adu, &bundleObj) != 1) {
        putErrmsg("Failed to send bundle", tracker->destEid);
        return -1;
    }
    
    CHKERR(sdr_begin_xn(sdr));
    
    /* Create tracking bundle object */
    tbundleObj = sdr_malloc(sdr, sizeof(TrackedBundle));
    if (tbundleObj == 0) {
        sdr_cancel_xn(sdr);
        putErrmsg("No space for tracked bundle", NULL);
        bp_cancel(bundleObj);  /* Clean up the bundle */
        return -1;
    }
    
    /* Initialize tracking data */
    tbundle.bundleObj = bundleObj;
    tbundle.sendTime = time(NULL);
    tbundle.status = BUNDLE_PENDING;
    tbundle.bundleId = bundleId;
    strncpy(tbundle.destEid, tracker->destEid, sizeof(tbundle.destEid) - 1);
    tbundle.destEid[sizeof(tbundle.destEid) - 1] = '\0';
    
    /* Add to tracking list */
    trackingElt = sdr_list_insert_last(sdr, tracker->trackedBundles, tbundleObj);
    if (trackingElt == 0) {
        sdr_cancel_xn(sdr);
        putErrmsg("Can't add bundle to tracking list", NULL);
        sdr_free(sdr, tbundleObj);
        bp_cancel(bundleObj);
        return -1;
    }
    
    tbundle.trackingElt = trackingElt;
    sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
    
    /* Register tracking with ION */
    if (bp_track(bundleObj, trackingElt) < 0) {
        sdr_cancel_xn(sdr);
        putErrmsg("Can't register bundle tracking", NULL);
        sdr_list_delete(sdr, trackingElt, NULL, NULL);
        sdr_free(sdr, tbundleObj);
        bp_cancel(bundleObj);
        return -1;
    }
    
    tracker->totalSent++;
    
    if (sdr_end_xn(sdr) < 0) {
        putErrmsg("Can't complete tracked bundle send", NULL);
        return -1;
    }
    
    printf("Bundle %d sent to %s (object: " ADDR_FIELDSPEC ")\n", 
           bundleId, tracker->destEid, bundleObj);
    
    return 0;
}

/*	Check status of all tracked bundles	*/
int checkBundleStatus(BundleTracker *tracker)
{
    Sdr sdr = getIonsdr();
    Object elt, nextElt;
    Object tbundleObj;
    TrackedBundle tbundle;
    Bundle bundle;
    int bundlesActive = 0;
    
    CHKZERO(sdr_begin_xn(sdr));
    
    for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; elt = nextElt) {
        nextElt = sdr_list_next(sdr, elt);
        tbundleObj = sdr_list_data(sdr, elt);
        
        /* Check if tracking element still exists (bundle not destroyed) */
        if (tbundleObj == 0) {
            /* Bundle was destroyed, remove from our list */
            sdr_list_delete(sdr, elt, NULL, NULL);
            continue;
        }
        
        sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
        
        /* Skip already processed bundles */
        if (tbundle.status == BUNDLE_COMPLETED || tbundle.status == BUNDLE_RELEASED) {
            continue;
        }
        
        /* Try to read bundle object to check if it still exists */
        if (tbundle.bundleObj == 0) {
            /* Bundle was destroyed */
            printf("Bundle %d completed/destroyed\n", tbundle.bundleId);
            tbundle.status = BUNDLE_COMPLETED;
            tracker->totalCompleted++;
            
            /* Clean up tracking */
            sdr_free(sdr, tbundleObj);
            sdr_list_delete(sdr, elt, NULL, NULL);
            continue;
        }
        
        /* Read bundle to check current status */
        sdr_read(sdr, (char *) &bundle, tbundle.bundleObj, sizeof(Bundle));
        
        /* Check various bundle states */
        if (bundle.planXmitElt == 0 && bundle.ductXmitElt == 0 && 
            bundle.fwdQueueElt == 0 && bundle.dlvQueueElt == 0) {
            /* Bundle appears to be transmitted successfully */
            if (tbundle.status == BUNDLE_PENDING) {
                printf("Bundle %d appears to be transmitted\n", tbundle.bundleId);
                tbundle.status = BUNDLE_COMPLETED;
                tracker->totalCompleted++;
                
                /* Release the bundle */
                bp_release(tbundle.bundleObj);
                
                /* Clean up tracking */
                sdr_free(sdr, tbundleObj);
                sdr_list_delete(sdr, elt, NULL, NULL);
                continue;
            }
        }
        
        /* Count active bundles */
        if (tbundle.status == BUNDLE_PENDING || tbundle.status == BUNDLE_SUSPENDED) {
            bundlesActive++;
        }
        
        /* Update tracking object */
        sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
    }
    
    sdr_exit_xn(sdr);
    return bundlesActive;
}

/*	Release a specific bundle by ID	*/
int releaseBundle(BundleTracker *tracker, int bundleId)
{
    Sdr sdr = getIonsdr();
    Object elt, nextElt;
    Object tbundleObj;
    TrackedBundle tbundle;
    int found = 0;
    
    CHKERR(sdr_begin_xn(sdr));
    
    for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; elt = nextElt) {
        nextElt = sdr_list_next(sdr, elt);
        tbundleObj = sdr_list_data(sdr, elt);
        
        if (tbundleObj == 0) {
            sdr_list_delete(sdr, elt, NULL, NULL);
            continue;
        }
        
        sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
        
        if (tbundle.bundleId == bundleId) {
            found = 1;
            
            if (tbundle.status == BUNDLE_COMPLETED || tbundle.status == BUNDLE_RELEASED) {
                printf("Bundle %d is already processed\n", bundleId);
            } else if (tbundle.bundleObj != 0) {
                printf("Releasing bundle %d\n", bundleId);
                bp_release(tbundle.bundleObj);
                tbundle.status = BUNDLE_RELEASED;
                tracker->totalReleased++;
                sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
            }
            break;
        }
    }
    
    if (sdr_end_xn(sdr) < 0) {
        putErrmsg("Can't release bundle", NULL);
        return -1;
    }
    
    if (!found) {
        printf("Bundle %d not found\n", bundleId);
        return 0;
    }
    
    return 1;
}

/*	List all active bundles	*/
void listActiveBundles(BundleTracker *tracker)
{
    Sdr sdr = getIonsdr();
    Object elt;
    Object tbundleObj;
    TrackedBundle tbundle;
    Bundle bundle;
    time_t currentTime = time(NULL);
    int activeCount = 0;
    
    printf("\n=== Active Bundles ===\n");
    
    CHKVOID(sdr_begin_xn(sdr));
    
    for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; 
         elt = sdr_list_next(sdr, elt)) {
        tbundleObj = sdr_list_data(sdr, elt);
        
        if (tbundleObj == 0) continue;
        
        sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
        
        if (tbundle.status == BUNDLE_COMPLETED || tbundle.status == BUNDLE_RELEASED) {
            continue;
        }
        
        activeCount++;
        printf("Bundle %d: ", tbundle.bundleId);
        
        if (tbundle.bundleObj == 0) {
            printf("Status: DESTROYED\n");
            continue;
        }
        
        /* Read bundle details */
        sdr_read(sdr, (char *) &bundle, tbundle.bundleObj, sizeof(Bundle));
        
        printf("Status: ");
        switch (tbundle.status) {
            case BUNDLE_PENDING:
                printf("PENDING");
                break;
            case BUNDLE_SUSPENDED:
                printf("SUSPENDED");
                break;
            default:
                printf("UNKNOWN");
        }
        
        printf(", Age: %ld seconds", currentTime - tbundle.sendTime);
        printf(", Object: " ADDR_FIELDSPEC "", tbundle.bundleObj);
        
        /* Show queue status */
        if (bundle.planXmitElt != 0 || bundle.ductXmitElt != 0) {
            printf(", In queues");
        }
        
        printf("\n");
    }
    
    sdr_exit_xn(sdr);
    
    if (activeCount == 0) {
        printf("No active bundles\n");
    }
    printf("======================\n\n");
}

/*	Print current status	*/
void printStatus(BundleTracker *tracker)
{
    int active = checkBundleStatus(tracker);
    printf("\n=== Bundle Tracker Status ===\n");
    printf("Destination: %s\n", tracker->destEid);
    printf("Active bundles: %d\n", active);
    printf("Completed bundles: %d\n", tracker->totalCompleted);
    printf("Released bundles: %d\n", tracker->totalReleased);
    printf("Total sent: %d\n", tracker->totalSent);
    if (tracker->totalSent > 0) {
        printf("Success rate: %.1f%%\n", 
               (100.0 * tracker->totalCompleted) / tracker->totalSent);
    }
    printf("=============================\n\n");
}

/*	Release all bundles	*/
void releaseAllBundles(BundleTracker *tracker)
{
    Sdr sdr = getIonsdr();
    Object elt, nextElt;
    Object tbundleObj;
    TrackedBundle tbundle;
    int released = 0;
    
    printf("Releasing all detained bundles...\n");
    
    if (sdr_begin_xn(sdr) < 0) {
        putErrmsg("Can't begin release transaction", NULL);
        return;
    }
    
    /* Release all remaining bundles */
    for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; elt = nextElt) {
        nextElt = sdr_list_next(sdr, elt);
        tbundleObj = sdr_list_data(sdr, elt);
        
        if (tbundleObj != 0) {
            sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
            
            if (tbundle.bundleObj != 0 && tbundle.status != BUNDLE_COMPLETED && 
                tbundle.status != BUNDLE_RELEASED) {
                printf("Releasing bundle %d\n", tbundle.bundleId);
                bp_release(tbundle.bundleObj);
                tracker->totalReleased++;
                released++;
            }
            
            sdr_free(sdr, tbundleObj);
        }
        
        sdr_list_delete(sdr, elt, NULL, NULL);
    }
    
    /* Destroy the tracking list */
    sdr_list_destroy(sdr, tracker->trackedBundles, NULL, NULL);
    
    if (sdr_end_xn(sdr) < 0) {
        putErrmsg("Can't complete release", NULL);
    }
    
    printf("Released %d bundles.\n", released);
}

/*	Cleanup and close tracker	*/
void cleanupBundleTracker(BundleTracker *tracker)
{
    /* Release any remaining bundles */
    releaseAllBundles(tracker);
    
    /* Close the SAP */
    bp_close(tracker->sap);
    
    printf("Bundle tracker cleanup completed.\n");
    printf("Final stats: %d sent, %d completed, %d released\n",
           tracker->totalSent, tracker->totalCompleted, tracker->totalReleased);
}

/*	Thread function for user input	*/
void* inputHandler(void* arg)
{
    char input[100];
    int bundleId;
    BundleTracker *tracker = (BundleTracker*)arg;
    
    printf("\nInteractive mode enabled. Type 'h' for help.\n");
    
    while (!g_quit) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        /* Remove newline */
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "q") == 0 || strcmp(input, "quit") == 0) {
            printf("Quitting...\n");
            g_quit = 1;
            break;
        } else if (strcmp(input, "s") == 0 || strcmp(input, "status") == 0) {
            printStatus(tracker);
        } else if (strcmp(input, "l") == 0 || strcmp(input, "list") == 0) {
            listActiveBundles(tracker);
        } else if (strcmp(input, "ra") == 0 || strcmp(input, "release-all") == 0) {
            releaseAllBundles(tracker);
            printf("All bundles have been released.\n");
        } else if (sscanf(input, "r %d", &bundleId) == 1 || 
                   sscanf(input, "release %d", &bundleId) == 1) {
            releaseBundle(tracker, bundleId);
        } else if (sscanf(input, "suspend %d", &bundleId) == 1 ||
                   sscanf(input, "sp %d", &bundleId) == 1) {
            suspendBundle(tracker, bundleId);
        } else if (sscanf(input, "resume %d", &bundleId) == 1 ||
                   sscanf(input, "res %d", &bundleId) == 1) {
            resumeBundle(tracker, bundleId);
        } else if (strcmp(input, "h") == 0 || strcmp(input, "help") == 0) {
            printf("\nAvailable commands:\n");
            printf("  q, quit                  - Release all bundles and quit\n");
            printf("  s, status                - Show current status\n");
            printf("  l, list                  - List all active bundles\n");
            printf("  r <id>, release <id>     - Release specific bundle by ID\n");
            printf("  ra, release-all          - Release all bundles\n");
            printf("  suspend <id>, sp <id>    - Suspend specific bundle by ID\n");
            printf("  resume <id>, res <id>    - Resume specific bundle by ID\n");
            printf("  h, help                  - Show this help\n\n");
            printf("Note: Suspend/resume only works for bundles still in local storage.\n");
            printf("Use 'list' to see which bundles can be suspended or resumed.\n\n");
        } else if (strlen(input) > 0) {
            printf("Unknown command: %s (type 'h' for help)\n", input);
        }
    }
    
    return NULL;
}

/*	Main application	*/
int main(int argc, char *argv[])
{
    BundleTracker tracker;
    char *sourceEid, *destEid;
    int payloadSize, numBundles;
    Object *payloads;
    int i;
    pthread_t inputThread;
    
    /* Parse command line arguments */
    if (argc != 5) {
        printUsage(argv[0]);
        return 1;
    }
    
    sourceEid = argv[1];
    destEid = argv[2];
    payloadSize = atoi(argv[3]);
    numBundles = atoi(argv[4]);
    
    /* Validate arguments */
    if (payloadSize < 1 || payloadSize > 65536) {
        fprintf(stderr, "Error: Payload size must be between 1 and 65536 bytes\n");
        return 1;
    }
    
    if (numBundles < 1 || numBundles > 10) {
        fprintf(stderr, "Error: Number of bundles must be between 1 and 10\n");
        return 1;
    }
    
    printf("ION Bundle Tracker Starting...\n");
    printf("Source: %s -> Destination: %s\n", sourceEid, destEid);
    printf("Payload size: %d bytes, Bundles: %d\n\n", payloadSize, numBundles);
    
    /* Attach to BP */
    if (bpAttach() < 0) {
        putErrmsg("Can't attach to ION", NULL);
        return 1;
    }
    
    /* Set up signal handlers */
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    /* Initialize bundle tracker */
    if (initializeBundleTracker(&tracker, sourceEid, destEid) < 0) {
        putErrmsg("Can't initialize bundle tracker", NULL);
        bpDetach();
        return 1;
    }
    
    g_tracker = &tracker;
    
    /* Create payloads */
    payloads = malloc(numBundles * sizeof(Object));
    if (payloads == NULL) {
        putErrmsg("Can't allocate payload array", NULL);
        cleanupBundleTracker(&tracker);
        bpDetach();
        return 1;
    }
    
    printf("Creating %d payloads of %d bytes each...\n", numBundles, payloadSize);
    for (i = 0; i < numBundles; i++) {
        payloads[i] = createPayload(payloadSize);
        if (payloads[i] == 0) {
            fprintf(stderr, "Failed to create payload %d\n", i + 1);
            /* Clean up already created payloads */
            for (int j = 0; j < i; j++) {
                if (payloads[j] != 0) {
                    Sdr sdr = getIonsdr();
                    CHKERR(sdr_begin_xn(sdr));
                    zco_destroy(sdr, payloads[j]);
                    oK(sdr_end_xn(sdr));
                }
            }
            free(payloads);
            cleanupBundleTracker(&tracker);
            bpDetach();
            return 1;
        }
    }
    
    /* Send bundles */
    printf("\nSending %d bundles...\n", numBundles);
    for (i = 0; i < numBundles; i++) {
        if (sendTrackedBundle(&tracker, payloads[i], 300, i + 1) < 0) {
            fprintf(stderr, "Failed to send bundle %d\n", i + 1);
        }
        usleep(100000);  /* 100ms delay between sends */
    }
    
    free(payloads);
    
    /* Start input handler thread */
    if (pthread_create(&inputThread, NULL, inputHandler, &tracker) != 0) {
        putErrmsg("Can't create input thread", NULL);
        g_quit = 1;
    }
    
    /* Monitor bundle status */
    printf("\nMonitoring bundle status (Ctrl+C or type 'quit' to exit)...\n");
    printf("Use interactive commands to manage bundles.\n");
    
    while (!g_quit) {
        int active = checkBundleStatus(&tracker);
        
        if (active == 0 && tracker.totalCompleted + tracker.totalReleased == tracker.totalSent) {
            printf("\nAll bundles processed. Type 'quit' to exit or continue monitoring.\n");
        }
        
        sleep(5);  /* Check every 5 seconds */
    }
    
    /* Wait for input thread to finish */
    if (!g_quit) {
        g_quit = 1;
    }
    pthread_cancel(inputThread);
    pthread_join(inputThread, NULL);
    
    /* Cleanup */
    printf("\nShutting down...\n");
    cleanupBundleTracker(&tracker);
    bpDetach();
    
    return 0;
}