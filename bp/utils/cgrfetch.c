/*

	cgrfetch.c:	CGR routing table analysis tool

*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cgr.h"
#include "lyst.h"

// A hop in a route
typedef struct {
	uvast fromNode;
	uvast toNode;
} Hop;

// A route considered by CGR
typedef struct {
	// Whether CGR has decided to use the route
	bool selected;
	// Time when route becomes available
	time_t fromTime;
	// Time when bundle will be delivered
	time_t deliveryTime;
	// Minimum capacity of all hops in the route
	uvast maxCapacity;
	// Hops taken in the route, from local node to destination node
	Lyst hops;
} Route;

// State of a CGR trace
typedef struct {
	// Current routes built by CGR, ordered from earliest to latest delivery
	// time
	Lyst routes;
	// Current route in walk over routes
	LystElt routeElt;
	// Current route being built by CGR when building routes OR
	// Current route selected by CGR when selecting proximate nodes
	Route *route;
	// Whether CGR is recomputing a route
	bool recomputing;
} TraceState;

static void hopDestroy(Hop *hop) {
	MRELEASE(hop);
}

static void hopDeleteFn(LystElt elt, void *data) {
	hopDestroy(lyst_data(elt));
}

static void routeDestroy(Route *route)
{
	lyst_destroy(route->hops);
	MRELEASE(route);
}

static void routeDeleteFn(LystElt elt, void *data) {
	routeDestroy(lyst_data(elt));
}

// Find where to insert a route so the list remains sorted. (copied from libcgr)
static LystElt findSpotForRoute(Lyst routes, Route *newRoute)
{
	LystElt routeElt;
	Route *route;

	for (routeElt = lyst_first(routes); routeElt;
	     routeElt = lyst_next(routeElt))
	{
		route = lyst_data(routeElt);

		if (route->deliveryTime > newRoute->deliveryTime)
		{
			return routeElt;
		}
	}

	return 0;
}

static void traceFn(void *data, unsigned int lineNbr,
                    unsigned int traceType, ...)
{
	TraceState *traceState = data;

	LystElt routeElt, nextElt;
	Route *route;
	Hop *hop;

	va_list args;
	const char *text;

	va_start(args, traceType);

	text = cgr_tracepoint_text(traceType);
	vprintf(text, args);

	switch (traceType) {
	case CgrIgnoreContact:
	case CgrIgnoreRoute:
	case CgrIgnoreProximateNode:
		printf(" %s",
			cgr_reason_text(va_arg(args, CgrReason)));
	}

	putchar('\n');

	va_end(args);
	va_start(args, traceType);

	switch (traceType) {
	case CgrBeginRoute:
		traceState->route = MTAKE(sizeof(Route));
		traceState->route->selected = false;
		traceState->route->hops = lyst_create();
		lyst_delete_set(traceState->route->hops, hopDeleteFn, NULL);
	break;

	case CgrHop:
		hop = MTAKE(sizeof(Hop));
		hop->fromNode = va_arg(args, uvast);
		hop->toNode = va_arg(args, uvast);

		// Hops are traced from destination node to local node, so
		// insert in reverse order.
		lyst_insert_first(traceState->route->hops, hop);
	break;

	case CgrAcceptRoute:
		route = traceState->route;

		// Discard firstHop.
		va_arg(args, uvast);
		route->fromTime = va_arg(args, unsigned int);
		route->deliveryTime = va_arg(args, unsigned int);
		route->maxCapacity = va_arg(args, uvast);

		if (traceState->recomputing)
		{
			// If recomputing, we need to find the right place to
			// insert the route and restart the walk from the
			// beginning (to keep in sync with CGR.)

			routeElt = findSpotForRoute(traceState->routes, route);

			if (routeElt)
			{
				lyst_insert_before(routeElt, route);
			}
			else
			{
				lyst_insert_last(traceState->routes, route);
			}

			traceState->routeElt = lyst_first(traceState->routes);
			traceState->recomputing = false;
		}
		else
		{
			// CGR traces route building in the correct order, so
			// just insert as normal.
			lyst_insert_last(traceState->routes, route);
		}
	break;

	case CgrDiscardRoute:
		// Discard the route being built.
		routeDestroy(traceState->route);

		if (traceState->recomputing)
		{
			// If recomputing, remove the route being recomputed and
			// move on the next route in the walk (to keep in sync
			// with CGR).
			nextElt = lyst_next(traceState->routeElt);
			lyst_delete(traceState->routeElt);
			traceState->routeElt = nextElt;
			traceState->recomputing = false;
		}
	break;

	case CgrRecomputeRoute:
		// CGR is going to be recomputing a route.
		traceState->recomputing = true;
	break;

	case CgrIgnoreRoute:
		// TODO: show route as disabled.
	break;

	case CgrIdentifyProximateNodes:
	case CgrSelectProximateNodes:
		// Start walking from the first route when identifying and
		// selecting proximate nodes.
		traceState->routeElt = lyst_first(traceState->routes);
		traceState->route = NULL;
	break;

	case CgrSelectProximateNode:
		// CGR has selected the current route in the walk.
		traceState->route = lyst_data(traceState->routeElt);
		// fallthrough
	case CgrAddProximateNode:
	case CgrUpdateProximateNode:
		// TODO: handle multiple routes with same proximate node.
	case CgrIgnoreProximateNode:
		// Move on to the next route in the walk.
		traceState->routeElt = lyst_next(traceState->routeElt);
	break;

	case CgrUseProximateNode:
		// CGR is done walking proximate nodes, so mark the current
		// selected route (if there is one) as the final selected route..
		if (traceState->route)
			traceState->route->selected = true;
	break;

	case CgrUseAllProximateNodes:
		// CGR decided to use all proximate nodes, so mark all routes as
		// selected.
		for (routeElt = lyst_first(traceState->routes); routeElt;
		     routeElt = lyst_next(routeElt))
		{
			route = lyst_data(routeElt);
			route->selected = true;
		}
	break;
	}

	va_end(args);
}

static int getDirective(uvast nodeNbr, Object plans, Bundle *bundle,
                        FwdDirective *directive)
{
	PsmAddress vductElt;
	VOutduct *vduct;

	findOutduct("udp", "*", &vduct, &vductElt);

	if (vductElt == 0) {
		putErrmsg("Can't find outduct.", NULL);
		return 0;
	}

	*directive = (FwdDirective) {
		.outductElt = vduct->outductElt,
	};

	return 1;
}

// Check if the contact exists in the route's hops.
static int checkContactIsHop(const IonCXref *contact, Route *route)
{
	LystElt hopElt;
	const Hop *hop;

	for (hopElt = lyst_first(route->hops); hopElt;
	     hopElt = lyst_next(hopElt))
	{
		hop = lyst_data(hopElt);

		if (contact->fromNode == hop->fromNode &&
		    contact->toNode == hop->toNode)
			return 1;
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

	IonRXref arg = {
		.fromNode = contact->fromNode,
		.toNode = contact->toNode,
	};

	for (sm_rbt_search(ionwm, ionvdb->rangeIndex, rfx_order_ranges, &arg,
	                   &rangeElt);
	     rangeElt; rangeElt = sm_rbt_next(ionwm, rangeElt))
	{
		range = psp(ionwm, sm_rbt_data(ionwm, rangeElt));

		if (range->fromNode > contact->fromNode ||
		    range->toNode > contact->toNode)
		{
			break;
		}

		if (range->toTime < contact->fromTime)
		{
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

static int run_cgrfetch(uvast nodeNumber, time_t atTime)
{
	if (bp_attach() < 0) {
		putErrmsg("cgrfetch can't attach to BP.", NULL);
		return 0;
	}

	// Flush the cached routing tables.
	cgr_stop();
	cgr_start();

	PsmPartition ionwm = getIonwm();
	IonVdb *ionvdb = getIonVdb();
	uvast localNode = getOwnNodeNbr();

	Lyst routes;
	LystElt routeElt;
	Route *route;

	PsmAddress contactElt;
	IonCXref *contact;
	IonRXref *range;

	size_t r;
	FILE *f;

	enum { TIME_BUF_SIZE = 32 };
	char timeBuf[TIME_BUF_SIZE];

	Object plans;

	Bundle bundle = {
		/* .extendedCOS = { */
		/* 	.flags = BP_MINIMUM_LATENCY, */
		/* }, */
		.payload = {
			.length = 0,
		},
		.returnToSender = 0,
		.clDossier = {
			.senderNodeNbr = localNode,
		},
		.expirationTime = atTime + 3600,
		.dictionaryLength = 0,
		.extensionsLength = {0, 0},
	};

	routes = lyst_create();

	if (routes == NULL) {
		putErrmsg("Unable to create routes list", NULL);
		return 0;
	}

	lyst_delete_set(routes, routeDeleteFn, NULL);

	CgrTrace trace = {
		.fn = traceFn,
		.data = &(TraceState) {
			.routes = routes,
			.recomputing = false,
		},
	};

	if (cgr_preview_forward(&bundle, (Object) &bundle, nodeNumber,
	      (Object) &plans, getDirective, atTime, &trace) < 0)
	{
		putErrmsg("Can't preview cgr.", NULL);
		return 0;
	}

#define DARK    "\"#444444\""
#define LIGHT   "\"#f1f1f1\""
#define HILITE  "\"#ff0000\""
#define INFO    "\"#546092\""
#define DISABLE "\"#dddddd\""

	f = fopen("routes.gv", "w");

	fputs("digraph {\n"
              "dpi = 300\n"
	      "fontname = Monospace\n"
	      "fontcolor = " DARK "\n"
	      "node [fontname = \"Monospace Bold\", style = filled,\n"
	      "      fillcolor = " LIGHT ", color = " DARK ",\n"
	      "      fontcolor = " DARK "]\n"
	      "edge [fontname = Monospace, arrowhead = vee,\n"
	      "      color = " DARK "]\n", f);

	for (routeElt = lyst_first(routes), r = 0; routeElt;
	     routeElt = lyst_next(routeElt), r += 1)
	{
		route = lyst_data(routeElt);

		// Output subgraph heading.
		fprintf(f,
			"subgraph cluster%zu {\n"
			"color = " DARK "\n"
			"margin = 20\n"
			"penwidth = %d\n"
			"labeljust = l\n",
			r,
			route->selected);

		// Output route title.
		fprintf(f,
			"label = <<table border=\"0\" cellborder=\"0\" cellspacing=\"0\">"
			"<tr><td align=\"left\"><b>ROUTE %zu</b></td></tr>"
			"<hr/>"
			"<tr><td></td></tr>",
			r + 1);

		strftime(timeBuf, TIME_BUF_SIZE, "%c", localtime(
			route->fromTime > atTime
				? &route->fromTime
				: &atTime));

		// Output dispatch time.
		fprintf(f,
			"<tr>"
			"  <td align=\"left\">dispatch </td>"
			"  <td align=\"left\"><font color=" INFO ">%s</font></td>"
			"</tr>",
			timeBuf);

		strftime(timeBuf, TIME_BUF_SIZE, "%c", localtime(
			&route->deliveryTime));

		// Output delivery time.
		fprintf(f,
			"<tr>"
			"  <td align=\"left\">deliver </td>"
			"  <td align=\"left\"><font color=" INFO ">%s</font></td>"
			"</tr>",
			timeBuf);

		// Output capacity.
		fprintf(f,
			"<tr>"
			"  <td align=\"left\">capacity </td>"
			"  <td align=\"left\"><font color=" INFO ">"
				UVAST_FIELDSPEC " bytes</font></td>"
			"</tr>",
			route->maxCapacity);

		fprintf(f,
			"</table>>"
			"node [shape = doublecircle]\n"
			"r%zun" UVAST_FIELDSPEC " r%zun" UVAST_FIELDSPEC "\n"
			"node [shape = circle]\n",

			r, localNode,
			r, nodeNumber);

		for (contactElt = sm_rbt_first(ionwm, ionvdb->contactIndex);
		     contactElt; contactElt = sm_rbt_next(ionwm, contactElt))
		{
			contact = psp(ionwm, sm_rbt_data(ionwm, contactElt));

			fprintf(f,
				"r%zun" UVAST_FIELDSPEC " [label = \""
					UVAST_FIELDSPEC "\"]\n"
				"r%zun" UVAST_FIELDSPEC " [label = \""
					UVAST_FIELDSPEC "\"]\n"
				"r%zun" UVAST_FIELDSPEC " -> r%zun"
					UVAST_FIELDSPEC,

				r, contact->fromNode, contact->fromNode,
				r, contact->toNode, contact->toNode,
				r, contact->fromNode,
				r, contact->toNode);

			if (contact->fromTime > route->deliveryTime ||
			    contact->toTime < route->fromTime)
			{
				fputs(" [color = " DISABLE ", fontcolor = "
					DISABLE "]", f);
			}
			else if (checkContactIsHop(contact, route))
			{
				fputs(" [color = " HILITE ", fontcolor = "
					HILITE "]", f);
			}

			range = findRange(contact);

			if (range)
			{
				fprintf(f, " [label = \"%u\"]", range->owlt);
			}

			fputc('\n', f);
		}

		fputs("}\n", f);
	}

	fputs("}\n", f);
	fclose(f);

	lyst_destroy(routes);

	bp_detach();

	return 0;
}

/* Main function defining cgrfetch utility startup */

#if defined (VXWORKS) || defined (RTEMS)
int	cgrfetch(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char *n = (char *) a2;
	char *t = (char *) a3;
	uvast nodeNumber = 0;
	time_t atTime;

#else
int	main(int argc, char **argv)
{
	char *n = NULL;
	char *t = NULL;
	uvast nodeNumber = 0;
	time_t atTime;

	if (argc > 3)
	{
		argc = 3;
	}

	switch (argc)
	{
		case 3:
			t = argv[2];
		case 2:
			n = argv[1];
	}

#endif

	if (n == NULL)
	{
		PUTS("Usage: cgrfetch destination-node [time-offset]");
		return 0;
	}

	nodeNumber = strtoul(n, NULL, 10);

	time(&atTime);

	if (t)
	{
		atTime += strtol(t, NULL, 10);
	}

	return run_cgrfetch(nodeNumber, atTime);
}
