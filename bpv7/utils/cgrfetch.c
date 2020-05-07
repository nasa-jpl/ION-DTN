/*

	cgrfetch.c:	CGR routing table analysis tool

*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cgr.h"
#include "lyst.h"

#define DARK      "\"#444444\""
#define LIGHT     "\"#F8F8F8\""
#define HILIGHT   "\"#EC1C24\""
#define DISABLED  "\"#DDDDDD\""

#define GRAPHVIZ_FILENAME "route.gv"
#define IMAGE_FILENAME    "route.svg.base64"

typedef struct {
	uvast fromNode;
	uvast toNode;
} Hop;

typedef struct {
	enum {
		// Route wasn't identified as a proximate node
		DEFAULT,
		// Route was identified as a proximate node but another route
		// with the same first hop was chosen instead
		IDENTIFIED,
		// Route was identified as a proximate node and the best route
		// for the first hop
		CONSIDERED,
		// Route was selected by CGR to forward along
		SELECTED,
	} flag;

	// Why the route was ignored or not selected
	CgrReason ignoreReason;

	// First hop neighbor in the route
	uvast firstHop;
	// Time when route becomes available
	time_t fromTime;
	// Time when bundle will be delivered
	time_t deliveryTime;
	// Minimum capacity of all hops in the route
	uvast maxCapacity;
	// Capacity payload class
	int payloadClass;
	// Hops taken in the route, from local node to destination node
	Lyst hops;
} Route;

typedef struct {
	// Current routes built by CGR, ordered from earliest to latest delivery
	// time
	Lyst routes;
	// Current route in walk over routes
	LystElt routeElt;
	// Current route being built by CGR when building routes OR
	// Current route selected by CGR when selecting proximate nodes
	Route *route;
} TraceState;

// Command line arguments
static enum {
	FLAGS_DEFAULT    = 0,
	OUTPUT_JSON      = 1 << 0,
	OUTPUT_TRACE_MSG = 1 << 1,
	LIST_OUTDUCTS    = 1 << 2,
} flags = OUTPUT_JSON | OUTPUT_TRACE_MSG;

static uvast destNode;
static time_t dispatchOffset = 0;
static time_t expirationOffset = 3600;
static unsigned int bundleSize = 0;
static int minLatency = 0;
static FILE *outputFile = NULL;
static char *outductProto = "udp";
static char *outductName = "*";

// Chosen outduct
static PsmAddress vductElt;
static VOutduct *vduct;

// Print a string and exit.
#define DIES(str) do \
{ \
	fputs("cgrfetch: " str "\n", stderr); \
	exit(EXIT_FAILURE); \
} while (0)

// Print a formatted string and exit.
#define DIEF(fmt, ...) do \
{ \
	fprintf(stderr, "cgrfetch: " fmt "\n", __VA_ARGS__); \
	exit(EXIT_FAILURE); \
} while (0)

static void hopDeleteFn(LystElt elt, void *data)
{
	MRELEASE(lyst_data(elt));
}

static void routeDestroy(Route *route)
{
	lyst_destroy(route->hops);
	MRELEASE(route);
}

static void routeDeleteFn(LystElt elt, void *data)
{
	routeDestroy(lyst_data(elt));
}

// Walk routes until a considered route is found.
static LystElt nextConsidered(LystElt routeElt)
{
	Route *route;

	while (routeElt)
	{
		route = lyst_data(routeElt);

		if (route->flag == CONSIDERED)
		{
			break;
		}

		routeElt = lyst_next(routeElt);
	}

	return routeElt;
}

static void outputTraceMsg(void *data, unsigned int lineNbr,
		           CgrTraceType traceType, va_list args)
{
	vfprintf(stderr, cgr_tracepoint_text(traceType), args);

	switch (traceType) {
	case CgrUpdateRoute:
		fputs("other route has", stderr);
		// fallthrough
	case CgrIgnoreContact:
	case CgrExcludeRoute:
	case CgrSkipRoute:
		fputc(' ', stderr);
		fputs(cgr_reason_text(va_arg(args, CgrReason)), stderr);
	default:
		break;
	}

	fputc('\n', stderr);
}

static void handleTraceState(void *data, unsigned int lineNbr,
		             CgrTraceType traceType, va_list args)
{
	TraceState *traceState = data;
	LystElt routeElt;
	Route *route;
	Hop *hop;

	switch (traceType) {
	case CgrBeginRoute:
		traceState->route = MTAKE(sizeof(Route));
		traceState->route->flag = DEFAULT;
		traceState->route->ignoreReason = CgrReasonMax;
		traceState->route->hops = lyst_create();
		lyst_delete_set(traceState->route->hops, hopDeleteFn, NULL);
		break;

	case CgrHop:
		// Create a new hop and add to the current route.
		hop = MTAKE(sizeof(Hop));
		hop->fromNode = va_arg(args, uvast);
		hop->toNode = va_arg(args, uvast);

		// Hops are traced from destination node to local node, so
		// insert in reverse order.
		lyst_insert_first(traceState->route->hops, hop);
		break;

	case CgrProposeRoute:
		route = traceState->route;

		route->firstHop = va_arg(args, uvast);
		route->fromTime = va_arg(args, unsigned int);
		route->deliveryTime = va_arg(args, unsigned int);
		lyst_insert_last(traceState->routes, route);
		traceState->routeElt = lyst_last(traceState->routes);
		break;

	case CgrNoRoute:
		// The route that was constructed is null.
		routeDestroy(traceState->route);
		break;

	case CgrIdentifyRoutes:
		// Start walking from the first route.
		traceState->routeElt = lyst_first(traceState->routes);
		break;

	case CgrCheckRoute:
		traceState->route = lyst_data(traceState->routeElt);
		break;

	case CgrExcludeRoute:
		// Mark why the current route was ignored and move on to the
		// next.
		traceState->route->ignoreReason = va_arg(args, CgrReason);
		traceState->routeElt = lyst_next(traceState->routeElt);
		break;

	case CgrUpdateRoute:
		// Find the proximate node being replaced and mark it as no
		// longer considered.
		for (routeElt = nextConsidered(lyst_first(traceState->routes));
		     routeElt; routeElt = nextConsidered(lyst_next(routeElt)))
		{
			route = lyst_data(routeElt);

			if (route->firstHop == traceState->route->firstHop)
			{
				route->flag = IDENTIFIED;
				route->ignoreReason = va_arg(args, CgrReason);
				break;
			}
		}

		// Fallthrough
	case CgrAddRoute:
		// Mark the current route as considered and continue the walk.
		traceState->route->flag = CONSIDERED;
		traceState->routeElt = lyst_next(traceState->routeElt);
		break;

	case CgrSelectRoutes:
		// Set that no route has been selected and start walking from
		// the first considered route.
		traceState->route = NULL;
		traceState->routeElt =
			nextConsidered(lyst_first(traceState->routes));
		break;

	case CgrSelectRoute:
		// Mark the current route as selected and move to the next
		// considered one.
		traceState->route = lyst_data(traceState->routeElt);
		traceState->routeElt =
			nextConsidered(lyst_next(traceState->routeElt));
		break;

	case CgrSkipRoute:
		// Mark why the current route was not used for forwarding.
		route = lyst_data(traceState->routeElt);
		route->ignoreReason = va_arg(args, CgrReason);

		traceState->routeElt =
			nextConsidered(lyst_next(traceState->routeElt));
		break;

	case CgrUseRoute:
		// CGR is done walking proximate nodes, so mark the current
		// selected route (if there is one) as the final selected route.
		if (traceState->route)
		{
			traceState->route->flag = SELECTED;
		}

		break;

	case CgrUseAllRoutes:
		// CGR decided to use all proximate nodes, so mark all
		// considered routes as selected.
		for (routeElt = nextConsidered(lyst_first(traceState->routes));
		     routeElt; routeElt = nextConsidered(lyst_next(routeElt)))
		{
			route = lyst_data(routeElt);
			route->flag = SELECTED;
		}

		break;

	default:
		break;
	}
}

// Build the routes list and output trace messages.
static void traceFnDefault(void *data, unsigned int lineNbr,
		           CgrTraceType traceType, ...)
{
	va_list args;

	va_start(args, traceType);
	outputTraceMsg(data, lineNbr, traceType, args);
	va_end(args);

	va_start(args, traceType);
	handleTraceState(data, lineNbr, traceType, args);
	va_end(args);
}

// Build the routes list but don't output trace messages.
static void traceFnQuiet(void *data, unsigned int lineNbr,
		         CgrTraceType traceType, ...)
{
	va_list args;

	va_start(args, traceType);
	handleTraceState(data, lineNbr, traceType, args);
	va_end(args);
}

// Check if the contact exists in the route's hops.
static int contactIsHop(const IonCXref *contact, Route *route)
{
	LystElt hopElt;
	const Hop *hop;

	for (hopElt = lyst_first(route->hops); hopElt;
	     hopElt = lyst_next(hopElt))
	{
		hop = lyst_data(hopElt);

		if (contact->fromNode == hop->fromNode &&
		    contact->toNode == hop->toNode)
		{
			return 1;
		}
	}

	return 0;
}

// Try to find a range for the contact. (copied from libcgr)
static IonRXref *findRange(const IonCXref *contact)
{
	PsmPartition ionwm = getIonwm();
	IonVdb *ionvdb = getIonVdb();

	PsmAddress rangeElt;
	IonRXref *range;
	static IonRXref discovery;

	IonRXref arg = {
		.fromNode = contact->fromNode,
		.toNode = contact->toNode,
	};

	if (contact->type == CtDiscovered || contact->confidence < 1.0)
	{
		discovery.owlt = 0;
		return &discovery;
	}

	oK(sm_rbt_search(ionwm, ionvdb->rangeIndex, rfx_order_ranges, &arg,
			&rangeElt));
	while (rangeElt)
	{
		range = psp(ionwm, sm_rbt_data(ionwm, rangeElt));

		if (range->fromNode > contact->fromNode
		|| range->toNode > contact->toNode)
		{
			break;
		}

		if (range->toTime < contact->fromTime)
		{
			rangeElt = sm_rbt_next(ionwm, rangeElt);
			continue;	/*	Past.	*/
		}

		if (range->fromTime > contact->fromTime)
		{
			break;
		}

		return range;
	}

	return NULL;
}

static inline const char *boolToStr(int b)
{
	return b ? "true" : "false";
}

static void output_json(Lyst routes, time_t dispatchTime,
		        time_t expirationTime)
{
	PsmPartition ionwm;
	IonVdb *ionvdb;
	uvast localNode;

	size_t r;
	LystElt routeElt;
	Route *route;

	PsmAddress contactElt;
	IonCXref *contact;
	IonRXref *range;

	FILE *f;
	int ret;

	char buf[BUFSIZ];
	size_t nBytes;

	ionwm = getIonwm();
	ionvdb = getIonVdb();
	localNode = getOwnNodeNbr();

	fprintf(outputFile,
		"{"
		  "\"constants\": {"
		    "\"DEFAULT\": %u,"
		    "\"IDENTIFIED\": %u,"
		    "\"CONSIDERED\": %u,"
		    "\"SELECTED\": %u"
		  "},"
		  "\"params\": {"
		    "\"localNode\": " UVAST_FIELDSPEC ","
		    "\"destNode\": " UVAST_FIELDSPEC ","
		    "\"dispatchTime\": %u,"
		    "\"expirationTime\": %u,"
		    "\"bundleSize\": %d,"
		    "\"minLatency\": %s"
		  "},"
		  "\"routes\": ["
		,
		DEFAULT, IDENTIFIED, CONSIDERED, SELECTED,

		localNode, destNode, (unsigned int)(dispatchTime),
		(unsigned int)(expirationTime), bundleSize,
		boolToStr(minLatency)
	);

	for (routeElt = lyst_first(routes), r = 0; routeElt;
	     routeElt = lyst_next(routeElt), r += 1)
	{
		route = lyst_data(routeElt);

		f = fopen(GRAPHVIZ_FILENAME, "w");

		if (!f)
		{
			DIES("unable to open '" GRAPHVIZ_FILENAME "'");
		}

		fprintf(f,
			"digraph {\n"
			"bgcolor = transparent\n"
			"node [fontname = Monospace, style = filled,\n"
			"      fillcolor = " LIGHT ", color = " DARK ",\n"
			"      fontcolor = " DARK "]\n"
			"edge [fontname = Monospace, arrowhead = vee,\n"
			"      color = " DARK "]\n"
			"node [shape = trapezium, orientation = 180]\n"
			UVAST_FIELDSPEC "\n"
			"node [shape = trapezium, orientation = 0]\n"
			UVAST_FIELDSPEC "\n"
			"node [shape = circle]\n"
			,
			localNode,
			destNode);

		for (contactElt = sm_rbt_first(ionwm, ionvdb->contactIndex);
		     contactElt; contactElt = sm_rbt_next(ionwm, contactElt))
		{
			contact = psp(ionwm, sm_rbt_data(ionwm, contactElt));

			fprintf(f,
				UVAST_FIELDSPEC " -> " UVAST_FIELDSPEC
				,
				contact->fromNode, contact->toNode);

			if (contact->fromTime > route->deliveryTime ||
			    contact->toTime < route->fromTime)
			{
				fputs(" [color = " DISABLED ", fontcolor = "
					DISABLED "]", f);
			}
			else if (contactIsHop(contact, route))
			{
				fputs(" [color = " HILIGHT ", fontcolor = "
					HILIGHT "]", f);
			}

			range = findRange(contact);

			if (range)
			{
				fprintf(f, " [label = \"%u\"]", range->owlt);
			}

			fputc('\n', f);
		}

		fputs("}", f);
		fclose(f);

		if (r)
		{
			fputc(',', outputFile);
		}

		fprintf(outputFile,
			"{"
			  "\"flag\": %u,"
			  "\"fromTime\": %u,"
			  "\"deliveryTime\": %u,"
			  "\"maxCapacity\": " UVAST_FIELDSPEC ","
			  "\"payloadClass\": %d,"
			  "\"ignoreReason\": \"%s\","
			  "\"graph\": \"data:image/svg+xml;base64,"
			,
			route->flag, (unsigned int)(route->fromTime),
			(unsigned int)(route->deliveryTime), route->maxCapacity,
			route->payloadClass, cgr_reason_text(route->ignoreReason)
		);

		ret = system("dot -Tsvg '" GRAPHVIZ_FILENAME "' | base64 -w 0 "
		             ">'" IMAGE_FILENAME "'");

		if (ret != EXIT_SUCCESS)
		{
			DIES("unable to call dot/base64");
		}

		f = fopen(IMAGE_FILENAME, "r");

		if (!f)
		{
			DIES("unable to open '" IMAGE_FILENAME "'");
		}

		do {
			nBytes = fread(buf, sizeof(char), BUFSIZ, f);
			fwrite(buf, sizeof(char), nBytes, outputFile);
		} while (nBytes == BUFSIZ);

		fclose(f);
		fputs("\"}", outputFile);
	}

	fputs("]}", outputFile);
	fclose(outputFile);
}

static void run_cgrfetch(void)
{
	uvast localNode;

	time_t nowTime;
	time_t dispatchTime;
	time_t expirationTime;

	Lyst routes;

	localNode = getOwnNodeNbr();

	nowTime = time(NULL);
	dispatchTime = nowTime + dispatchOffset;
	expirationTime = nowTime + expirationOffset;

	Bundle bundle = {
		.ancillaryData = {
			.flags = minLatency ? BP_MINIMUM_LATENCY
			                    : BP_BEST_EFFORT,
		},
		.payload = {
			.length = bundleSize,
		},
		.returnToSender = 0,
		.clDossier = {
			.senderNodeNbr = localNode,
		},
		.expirationTime = expirationTime,
		.extensionsLength = 0,
	};

	routes = lyst_create();

	if (!routes)
	{
		DIES("unable to create routes list");
	}

	lyst_delete_set(routes, routeDeleteFn, NULL);

	CgrTraceFn traceFn = traceFnDefault;

	if (!(flags & OUTPUT_TRACE_MSG))
	{
		traceFn = traceFnQuiet;
	}

	CgrTrace trace = {
		.fn = traceFn,
		.data = &(TraceState) {
			.routes = routes,
		},
	};

	// Flush the cached routing tables.
	cgr_stop();

	if (bpAttach() < 0)
	{
		DIES("Can't attach to BP");
	}

	cgr_start();

	Sdr	sdr = getIonsdr();
	if (sdr_begin_xn(sdr) != 1)
	{
		DIES("Can't lock database");
	}

	if (cgr_preview_forward(&bundle, (Object)(&bundle), destNode,
			dispatchTime, &trace) < 0)
	{
		DIES("unable to simulate cgr");
	}

	sdr_exit_xn(sdr);
	ionDetach();

	if (flags & OUTPUT_JSON)
	{
		output_json(routes, dispatchTime, expirationTime);
	}

	lyst_destroy(routes);
}

// Try to parse the given string into the form PROTO:NAME and assign PROTO to outductProto
// and NAME to outductName. Return 1 on success and 0 otherwise.
static int parseOutduct(char *str)
{
	char *sep = strchr(str, ':');

	if (!sep || sep == str)
		return 0;

	outductProto = str;
	outductName = sep + 1;

	// Check this before splitting the string so the original string can be
	// used in the error message.
	if (!outductName[0])
	{
		return 0;
	}

	*sep = '\0';

	return 1;
}

static void listOutducts(void) {
	PsmPartition bpwm = getIonwm();
	PsmAddress elt;
	VOutduct *vduct;

	for (elt = sm_list_first(bpwm, getBpVdb()->outducts); elt;
	     elt = sm_list_next(bpwm, elt))
	{
		vduct = (VOutduct *) psp(bpwm, sm_list_data(bpwm, elt));
		printf("%s:%s\n", vduct->protocolName, vduct->ductName);
	}
}

static void usage(const char *name)
{
	fprintf(stderr,
		"Usage: %s [-q] [-j] [-m] [-t DISPATCH-OFFSET]\n"
		"       [-e EXPIRATION-OFFSET] [-s BUNDLE-SIZE]\n"
		"       [-o OUTPUT-FILE] [-d PROTO:NAME] DEST-NODE\n"
		"\n"
		"In the first case, run a CGR simulation from the local node to\n"
		"DEST-NODE. Output trace messages to stderr (unless -q) and JSON\n"
		"to stdout (unless -j).\n"
		"\n"
		"Options:\n"
		"  -q                    disable trace message output\n"
		"  -j                    disable JSON output\n"
		"  -m                    use a minimum-latency extended COS\n"
		"                        for the bundle\n"
		"  -t DISPATCH-OFFSET    request a dispatch time of DISPATCH-\n"
		"                        OFFSET seconds from now (default: %u)\n"
		"  -e EXPIRATION-OFFSET  set the bundle expiration time to\n"
		"                        EXPIRATION-OFFSET seconds from now\n"
		"                        (default: %u)\n"
		"  -s BUNDLE-SIZE        set the bundle payload size to BUNDLE-\n"
		"                        SIZE bytes (default: %u)\n"
		"  -o OUTPUT-FILE        send JSON to OUTPUT-FILE (default: stdout)\n"
		"  -d PROTO:NAME         use the outduct with protocol PROTO and\n"
		"                        name NAME (default: %s:%s)\n"
		"                        list available outducts with -d list\n"
		,
		name,
		(unsigned int)(dispatchOffset),
		(unsigned int)(expirationOffset),
		bundleSize,
		outductProto,
		outductName
	);
}

#if defined (ION_LWT)
static void teardown(void)
{
	bp_detach();
}

int	cgrfetch(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	char *end;
#if defined (ION_LWT)

	if (a3)
	{
		dispatchOffset = strtoul((char *)(a3), &end, 10);

		if (end == (char *)(a3))
		{
			DIES("invalid dispatch offset");
		}
	}

	if (a4)
	{
		expirationOffset = strtoul((char *)(a4), &end, 10);

		if (end == (char *)(a4))
		{
			DIES("invalid expiration offset");
		}
	}

	if (a5)
	{
		bundleSize = strtoul((char *)(a5), &end, 10);

		if (end == (char *)(a5))
		{
			DIES("invalid bundle size");
		}
	}

	if (a6)
	{
		minLatency = ((char *)(a6))[0] == '1';
	}

	if (a7)
	{
		outputFile = fopen(a7, "w");

		if (!outputFile)
		{
			DIEF("unable to open '%s'", a7);
		}
	}

	if (a8)
	{
		if (!parseOutduct((char *)(a8)))
		{
			DIEF("invalid outduct '%s'", a8);
		}
	}
#else
	int opt;
	char **args;

	opterr = 0;

	while ((opt = getopt(argc, argv, ":hqjt:e:s:mo:d:")) >= 0)
	{
		switch (opt)
		{
		case 'h':
			usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;

		case 'q':
			flags &= ~OUTPUT_TRACE_MSG;
			break;

		case 'j':
			flags &= ~OUTPUT_JSON;
			break;

		case 't':
			dispatchOffset = strtoul(optarg, &end, 10);

			if (end == optarg)
			{
				DIEF("invalid dispatch offset '%s'", optarg);
			}

			break;

		case 'e':
			expirationOffset = strtoul(optarg, &end, 10);

			if (end == optarg)
			{
				DIEF("invalid expiration offset '%s'", optarg);
			}

			break;

		case 's':
			bundleSize = strtoul(optarg, &end, 10);

			if (end == optarg)
			{
				DIEF("invalid bundle size '%s'", optarg);
			}

			break;

		case 'm':
			minLatency = 1;
			break;

		case 'o':
			outputFile = fopen(optarg, "w");

			if (!outputFile)
			{
				DIEF("unable to open '%s'", optarg);
			}

			break;

		case 'd':
			if (strcmp(optarg, "list") == 0)
			{
				flags |= LIST_OUTDUCTS;
				break;
			}

			if (!parseOutduct(optarg))
			{
				DIEF("invalid outduct '%s'", optarg);
			}

			break;

		case ':':
			DIEF("option '-%c' takes an argument", optopt);
			break;

		case '?':
			fprintf(stderr, "unknown option '-%c'\n", optopt);
			break;
		}
	}
#endif

	if (bp_attach() < 0)
	{
		DIES("unable to attach to bp");
	}

	if (flags & LIST_OUTDUCTS)
	{
		listOutducts();
		exit(EXIT_SUCCESS);
	}

#if defined (VXWORKS) || defined (RTEMS)
	if (!a2)
	{
		DIES("a destination node is required");
	}

	destNode = strtoul((char *)(a2), &end, 10);

	if (end == (char *)(a2))
	{
		DIES("invalid destination node");
	}
#else
	args = &argv[optind];

	if (!args[0])
	{
		DIES("a destination node is required");
	}

	destNode = strtoul(args[0], &end, 10);

	if (end == args[0])
	{
		DIEF("invalid destination node '%s'", args[0]);
	}
#endif

	if (!outputFile)
	{
		outputFile = stdout;
	}

	findOutduct(outductProto, outductName, &vduct, &vductElt);

	if (!vductElt)
	{
		DIEF("unable to find outduct %s:%s", outductProto, outductName);
	}

	run_cgrfetch();
	exit(EXIT_SUCCESS);
}

// vim: sw=8 noexpandtab
