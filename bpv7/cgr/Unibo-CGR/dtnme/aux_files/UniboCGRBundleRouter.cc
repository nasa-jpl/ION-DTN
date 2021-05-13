/*
 *    Copyright 2005-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/*
 *    Modifications made to this file by the patch file dtn2_mfs-33289-1.patch
 *    are Copyright 2015 United States Government as represented by NASA
 *       Marshall Space Flight Center. All Rights Reserved.
 *
 *    Released under the NASA Open Source Software Agreement version 1.3;
 *    You may obtain a copy of the Agreement at:
 * 
 *        http://ti.arc.nasa.gov/opensource/nosa/
 * 
 *    The subject software is provided "AS IS" WITHOUT ANY WARRANTY of any kind,
 *    either expressed, implied or statutory and this agreement does not,
 *    in any manner, constitute an endorsement by government agency of any
 *    results, designs or products resulting from use of the subject software.
 *    See the Agreement for the specific language governing permissions and
 *    limitations.
 */
/*
*     UniboCGRBundleRouter.cc
*
*  This is a modification of UniboCGRBundleRouter made by Giacomo Gori
*  on Spring 2021 with Carlo Caini as supervisor.
*  It's purpose is to make possible for DTNME v1.0 beta to work with interface_unibocgr_dtn2
*  to use UniboCGR as routing mechanism. 
*/

#ifdef HAVE_CONFIG_H
#  include <dtn-config.h>
#endif

#include "UniboCGRBundleRouter.h"
#include "RouteTable.h"
#include "bundling/BundleActions.h"
#include "bundling/BundleDaemon.h"
#include "bundling/TempBundle.h"
#include "contacts/Contact.h"
#include "contacts/ContactManager.h"
#include "contacts/Link.h"
#include "reg/Registration.h"

//Interface for UniboCGR
#include "Unibo-CGR/dtnme/interface/interface_unibocgr_dtn2.h"
//Giacomo Gori
#define NOMINAL_PRIMARY_BLKSIZE	29 // from ION 4.0.0: bpv7/library/libbpP.c

namespace dtn {

void unibo_cgr_router_shutdown(void*)
{
    BundleDaemon::instance()->router()->shutdown();
}

//----------------------------------------------------------------------
UniboCGRBundleRouter::UniboCGRBundleRouter(const char* classname,
                                   const std::string& name)
    : BundleRouter(classname, name),
      reception_cache_(std::string(logpath()) + "/reception_cache",
                       1024) // XXX/demmer configurable??
{
    route_table_ = new RouteTable(name);

    // register the global shutdown function
    BundleDaemon::instance()->set_rtr_shutdown(
            unibo_cgr_router_shutdown, (void *) 0);
    //Giacomo:: getting necessary info & call initialize
    struct timeval tv;
    gettimeofday(&tv, NULL);
    EndpointID eid = BundleDaemon::instance()->local_eid_ipn();
    std::string ipnName = eid.str();
    std::string delimiter1 = ":";
    std::string delimiter2 = ".";
    std::string s = ipnName.substr(ipnName.find(delimiter1) + 1, ipnName.find(delimiter2) - 1);
    std::stringstream convert;
    long ownNode;
    convert << s;
    convert >> ownNode;
    initialize_contact_graph_routing(ownNode, tv.tv_sec, this);

}

UniboCGRBundleRouter::UniboCGRBundleRouter()
    : BundleRouter("UniboCGRBundleRouter", "uniboCGR"),
      reception_cache_(std::string(logpath()) + "/reception_cache",
                       1024) // XXX/demmer configurable??
{
    route_table_ = new RouteTable("uniboCGR");

    // register the global shutdown function
    BundleDaemon::instance()->set_rtr_shutdown(
    		unibo_cgr_router_shutdown, (void *) 0);
   //Giacomo:: getting necessary info & call initialize
   struct timeval tv;
   gettimeofday(&tv, NULL);
   EndpointID eid = BundleDaemon::instance()->local_eid_ipn();
   std::string ipnName = eid.str();
   std::string delimiter1 = ":";
   std::string delimiter2 = ".";
   std::string s = ipnName.substr(ipnName.find(delimiter1) + 1, ipnName.find(delimiter2) - 1);
   std::stringstream convert;
   long ownNode;
   convert << s;
   convert >> ownNode;
   initialize_contact_graph_routing(ownNode, tv.tv_sec, this);
}

//----------------------------------------------------------------------
UniboCGRBundleRouter::~UniboCGRBundleRouter()
{
    if(route_table_ != NULL) {
        delete route_table_;
        route_table_ = NULL;
    } 
}
void UniboCGRBundleRouter::shutdown() {
    if(route_table_ != NULL) {
        delete route_table_;
        route_table_ = NULL;
    }
    //Giacomo: call shutdown
    struct timeval tv;
    gettimeofday(&tv, NULL);
    destroy_contact_graph_routing(tv.tv_sec);
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::add_route(RouteEntry *entry, bool skip_changed_routes)
{
    route_table_->add_entry(entry);
    if (!skip_changed_routes) {
        handle_changed_routes();
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::del_route(const EndpointIDPattern& dest)
{
    route_table_->del_entries(dest);

    // clear the reception cache when the routes change since we might
    // want to send a bundle back where it came from
    reception_cache_.evict_all();
    
    // XXX/demmer this should really call handle_changed_routes...
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_changed_routes()
{
    // clear the reception cache when the routes change since we might
    // want to send a bundle back where it came from
    reception_cache_.evict_all();
    reroute_all_bundles();
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_event(BundleEvent* event)
{
    dispatch_event(event);
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_bundle_received(BundleReceivedEvent* event)
{
    if (BundleDaemon::instance()->shutting_down()) return;

    bool should_route = true;
    
    Bundle* bundle = event->bundleref_.object();
    //log_debug("handle bundle received: *%p", bundle);

    EndpointID remote_eid(EndpointID::NULL_EID());

    if (event->link_ != NULL) {
        remote_eid = event->link_->remote_eid();
    }

    if (should_route) {
        route_bundle(bundle);
    } else {
        BundleDaemon::post_at_head(
            new BundleDeleteRequest(bundle, BundleProtocol::REASON_NO_ADDTL_INFO));
    }
} 

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::remove_from_deferred(const BundleRef& bundle, int actions)
{
    ContactManager* cm = BundleDaemon::instance()->contactmgr();
    oasys::ScopeLock l(cm->lock(), "UniboCGRBundleRouter::remove_from_deferred");

    const LinkSet* links = cm->links();
    LinkSet::const_iterator iter;
    for (iter = links->begin(); iter != links->end(); ++iter) {
        const LinkRef& link = *iter;

        // a bundle might be deleted immediately after being loaded
        // from storage, meaning that remove_from_deferred is called
        // before the deferred list is created (since the link isn't
        // fully set up yet). so just skip the link if there's no
        // router info, and therefore no deferred list
        if (link->router_info() == NULL) {
            continue;
        }
        
        DeferredList* deferred = deferred_list(link);

        oasys::ScopeLock l(deferred->list()->lock(), 
                           "UniboCGRBundleRouter::remove_from_deferred");

        ForwardingInfo info;
        if (deferred->find(bundle, &info))
        {
            if (info.action() & actions) {
                //log_debug("removing bundle *%p from link *%p deferred list",
                //          bundle.object(), (*iter).object());
                deferred->del(bundle);
            }
        }
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_bundle_transmitted(BundleTransmittedEvent* event)
{
    if (BundleDaemon::instance()->shutting_down()) return;

    const BundleRef& bundle = event->bundleref_;
    //log_debug("handle bundle transmitted (%s): *%p", 
    //          event->success_?"success":"failure", bundle.object());

    if (!event->success_) {
        // try again if not successful the first time 
        // (only applicable to the LTPUDP CLA as of 2014-12-04)
        route_bundle(bundle.object());
    } else {
        // if the bundle has a deferred single-copy transmission for
        // forwarding on any links, then remove the forwarding log entries
        remove_from_deferred(bundle, ForwardingInfo::FORWARD_ACTION);
    }

    // check if the transmission means that we can send another bundle
    // on the link
    const LinkRef& link = event->contact_->link();
    check_next_hop(link);
}

//----------------------------------------------------------------------
bool
UniboCGRBundleRouter::can_delete_bundle(const BundleRef& bundle)
{
    //log_debug("UniboCGRBundleRouter::can_delete_bundle: checking if we can delete *%p",
    //          bundle.object());

    // check if we haven't yet done anything with this bundle
    if (bundle->fwdlog()->get_count(ForwardingInfo::TRANSMITTED |
                                    ForwardingInfo::DELIVERED)
                < bundle->expected_delivery_and_transmit_count())
    {
        //log_debug("UniboCGRBundleRouter::can_delete_bundle(%" PRIbid "): "
        //          "not yet transmitted or delivered the expected number of times",
        //          bundle->bundleid());
        return false;
    }

    // check if we have local custody
    if (bundle->local_custody() || bundle->bibe_custody()) {
        //log_debug("UniboCGRBundleRouter::can_delete_bundle(%" PRIbid "): "
        //          "not deleting because we have custody",
        //          bundle->bundleid());
        return false;
    }

    return true;
}
    
//----------------------------------------------------------------------
void
UniboCGRBundleRouter::delete_bundle(const BundleRef& bundle)
{
    //log_debug("delete *%p", bundle.object());

    remove_from_deferred(bundle, ForwardingInfo::ANY_ACTION);
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_bundle_cancelled(BundleSendCancelledEvent* event)
{
    if (BundleDaemon::instance()->shutting_down()) return;

    Bundle* bundle = event->bundleref_.object();
    //log_debug("handle bundle cancelled: *%p", bundle);

    // if the bundle has expired, we don't want to reroute it.
    // XXX/demmer this might warrant a more general handling instead?
    if (!bundle->expired()) {
        route_bundle(bundle);
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_route_add(RouteAddEvent* event)
{
    add_route(event->entry_);
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_route_del(RouteDelEvent* event)
{
    del_route(event->dest_);
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::add_nexthop_route(const LinkRef& link, bool skip_changed_routes)
{
    // If we're configured to do so, create a route entry for the eid
    // specified by the link when it connected, using the
    // scheme-specific code to transform the URI to wildcard
    // the service part
    EndpointID eid = link->remote_eid();
    if (config_.add_nexthop_routes_ && eid != EndpointID::NULL_EID())
    { 
        EndpointIDPattern eid_pattern(link->remote_eid());

        // attempt to build a route pattern from link's remote_eid
        if (!eid_pattern.append_service_wildcard())
            // else assign remote_eid as-is
            eid_pattern.assign(link->remote_eid());

        // XXX/demmer this shouldn't call get_matching but instead
        // there should be a RouteTable::lookup or contains() method
        // to find the entry
        RouteEntryVec ignored;
        if (route_table_->get_matching(eid_pattern, link, &ignored) == 0) {
            RouteEntry *entry = new RouteEntry(eid_pattern, link);
            entry->set_action(ForwardingInfo::FORWARD_ACTION);
            add_route(entry, skip_changed_routes);
        }
    }
}

//----------------------------------------------------------------------
bool
UniboCGRBundleRouter::should_fwd(const Bundle* bundle, RouteEntry* route)
{
    if (route == NULL)
        return false;

    // simple RPF check -- if the bundle was received from the given
    // node, then don't send it back as long as the entry is still in
    // the reception cache (meaning our routes haven't changed).
    EndpointID prevhop;


//dzdebug - just use prevhop in bundle to prevent sending back to original sender for now
/*
    if (reception_cache_.lookup(bundle, &prevhop))
    {
        if (prevhop == route->link()->remote_eid() &&
            prevhop != EndpointID::NULL_EID())
        {
            log_debug("should_fwd bundle %" PRIbid ": "
                      "skip %s since bundle arrived from the same node",
                      bundle->bundleid(), route->link()->name());
            return false;
        }
    }
*/
    if ( (bundle->prevhop() == route->link()->remote_eid()) &&
        (bundle->prevhop() != EndpointID::NULL_EID()) ) {
        log_debug("should_fwd bundle %" PRIbid ": "
                  "skip %s since bundle arrived from the same node",
                  bundle->bundleid(), route->link()->name());
        return false;
    }

    return BundleRouter::should_fwd(bundle, route->link(), route->action());
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_contact_up(ContactUpEvent* event)
{
    LinkRef link = event->contact_->link();
    ASSERT(link != NULL);
    ASSERT(!link->isdeleted());

    if (! link->isopen()) {
        log_err("contact up(*%p): event delivered but link not open",
                link.object());
    }

    add_nexthop_route(link);
    check_next_hop(link);

    // check if there's a pending reroute timer on the link, and if
    // so, cancel it.
    // 
    // note that there's a possibility that a link just bounces
    // between up and down states but can't ever really send a bundle
    // (or part of one), which we don't handle here since we can't
    // distinguish that case from one in which the CL is actually
    // sending data, just taking a long time to do so.
    oasys::SPtr_Timer base_sptr;
    SPtr_RerouteTimer t;

    RerouteTimerMap::iterator iter = reroute_timers_.find(link->name_str());
    if (iter != reroute_timers_.end()) {
        base_sptr = iter->second;
        reroute_timers_.erase(iter);
        oasys::SharedTimerSystem::instance()->cancel(base_sptr);
        base_sptr.reset();
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_contact_down(ContactDownEvent* event)
{
    LinkRef link = event->contact_->link();
    ASSERT(link != NULL);
    ASSERT(!link->isdeleted());

    // if there are any bundles queued on the link when it goes down,
    // schedule a timer to cancel those transmissions and reroute the
    // bundles in case the link takes too long to come back up

    size_t num_queued = link->queue()->size();
    if (num_queued != 0) {
        RerouteTimerMap::iterator iter = reroute_timers_.find(link->name_str());
        if (iter == reroute_timers_.end()) {
            log_debug("link %s went down with %zu bundles queued, "
                      "scheduling reroute timer in %u seconds",
                      link->name(), num_queued,
                      link->params().potential_downtime_);

            SPtr_RerouteTimer t = std::make_shared<RerouteTimer>(this, link);
            oasys::SPtr_Timer base_sptr = t;
            oasys::SharedTimerSystem::instance()->schedule_in(link->params().potential_downtime_ * 1000, base_sptr);
            
            reroute_timers_[link->name_str()] = t;
        }
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::reroute_bundles(const LinkRef& link)
{
    ASSERT(!link->isdeleted());

    // if the reroute timer fires, the link should be down and there
    // should be at least one bundle queued on it.
    if (link->state() != Link::UNAVAILABLE) {
        log_warn("reroute timer fired but link *%p state is %s, not UNAVAILABLE",
                 link.object(), Link::state_to_str(link->state()));
        return;
    }
    
    log_debug("reroute timer fired -- cancelling %zu bundles on link *%p",
              link->queue()->size(), link.object());
    
    // cancel the queued transmissions and rely on the
    // BundleSendCancelledEvent handler to actually reroute the
    // bundles, being careful when iterating through the lists to
    // avoid STL memory clobbering since cancel_bundle removes from
    // the list
    oasys::ScopeLock l(link->queue()->lock(),
                       "UniboCGRBundleRouter::reroute_bundles");
    BundleRef bundle("UniboCGRBundleRouter::reroute_bundles");
    while (! link->queue()->empty()) {
        bundle = link->queue()->front();
        actions_->cancel_bundle(bundle.object(), link);

        //XXX/dz TCPCL can't guarantee that bundle can be immediately removed from the queue
        //ASSERT(! bundle->is_queued_on(link->queue()));
    }

    // there should never have been any in flight since the link is
    // unavailable
    //dz - ION3.6.2 killm and restart did not clear inflight so rerouting them also
    //ASSERT(link->inflight()->empty());

    while (! link->inflight()->empty()) {
        bundle = link->inflight()->front();
        actions_->cancel_bundle(bundle.object(), link);

        //XXX/dz TCPCL can't guarantee that bundle can be immediately removed from the inflight queue
        //ASSERT(! bundle->is_queued_on(link->inflight()));
    }

}    

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_link_available(LinkAvailableEvent* event)
{
    LinkRef link = event->link_;
    ASSERT(link != NULL);
    ASSERT(!link->isdeleted());

    // if it is a discovered link, we typically open it
    if (config_.open_discovered_links_ &&
        !link->isopen() &&
        link->type() == Link::OPPORTUNISTIC &&
        event->reason_ == ContactEvent::DISCOVERY)
    {
        actions_->open_link(link);
    }
    
    // check if there's anything to be forwarded to the link
    check_next_hop(link);
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_link_created(LinkCreatedEvent* event)
{
    LinkRef link = event->link_;
    ASSERT(link != NULL);
    ASSERT(!link->isdeleted());

    link->set_router_info(new DeferredList(logpath(), link));
                          
    // true=skip changed routes because we are about to call it
    add_nexthop_route(link, true); 
    handle_changed_routes();
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_link_deleted(LinkDeletedEvent* event)
{
    LinkRef link = event->link_;
    ASSERT(link != NULL);

    route_table_->del_entries_for_nexthop(link);

    RerouteTimerMap::iterator iter = reroute_timers_.find(link->name_str());
    if (iter != reroute_timers_.end()) {
        log_debug("link %s deleted, cancelling reroute timer", link->name());
        oasys::SPtr_Timer base_sptr = iter->second;
        reroute_timers_.erase(iter);
        oasys::SharedTimerSystem::instance()->cancel(base_sptr);
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_link_check_deferred(LinkCheckDeferredEvent* event)
{
    //log_debug("LinkCheckDeferred event for *%p", event->linkref_.object());
    check_next_hop(event->linkref_);
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_custody_timeout(CustodyTimeoutEvent* event)
{
    // the bundle daemon should have recorded a new entry in the
    // forwarding log for the given link to note that custody transfer
    // timed out, and of course the bundle should still be in the
    // pending list.
    //
    // therefore, trying again to forward the bundle should match
    // either the previous link or any other route

    
    // Check to see if the bundle is still pending
    BundleRef bref("handle_custody_timerout");
    bref = pending_bundles_->find(event->bundle_id_);
    
    if (bref != NULL) {
        route_bundle(bref.object());
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::get_routing_state(oasys::StringBuffer* buf)
{
    buf->appendf("Route table for %s router:\n\n", name_.c_str());
    route_table_->dump(buf);
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::tcl_dump_state(oasys::StringBuffer* buf)
{
    oasys::ScopeLock l(route_table_->lock(),
                       "UniboCGRBundleRouter::tcl_dump_state");

    RouteEntryVec::const_iterator iter;
    for (iter = route_table_->route_table()->begin();
         iter != route_table_->route_table()->end(); ++iter)
    {
        const RouteEntry* e = *iter;
        buf->appendf(" {%s %s source_eid %s priority %d} ",
                     e->dest_pattern().c_str(),
                     e->next_hop_str().c_str(),
                     e->source_pattern().c_str(),
                     e->priority());
    }
}

//----------------------------------------------------------------------
bool
UniboCGRBundleRouter::fwd_to_nexthop(Bundle* bundle, RouteEntry* route)
{
    bool result = false;

    const LinkRef& link = route->link();

    // if the link is available and not open, open it
    if (link->isavailable() && (!link->isopen()) && (!link->isopening())) {
        log_debug("opening *%p because a message is intended for it",
                  link.object());
        actions_->open_link(link);
    }

    // always add the bundle to the link's deferred list so that they will
    // be delivered in the correct order. future calls to check_next_hop
    // will move the bundle to the transmit queue in due time
    DeferredList* deferred = deferred_list(link);
    if (! bundle->is_queued_on(deferred->list())) {
        BundleRef bref(bundle, "UniboCGRBundleRouter::fwd_to_nexthop");
        ForwardingInfo info(ForwardingInfo::NONE,
                            route->action(),
                            link->name_str(),
                            0xffffffff,
                            link->remote_eid(),
                            route->custody_spec());

        oasys::ScopeLock l(deferred->list()->lock(), 
                           "UniboCGRBundleRouter::fwd_to_nexthop");

        deferred->add(bref, info);

        result = true;
    } else {
        log_warn("bundle *%p already exists on deferred list of link *%p",
                 bundle, link.object());
    }
   
    // XXX/dz could check to see if there is space available on the queue 
    // and call check_next_hop here if needed...

    return result;
}

//----------------------------------------------------------------------
int
UniboCGRBundleRouter::getBacklogForNode(unsigned long long neighbor, int priority, long int* byteApp, long int * byteTot)
{
    //Giacomo:
    //Dal neighbor forma il nome completo del nodo (ipn:neighbor.0)
    //poi consulta la routing table e ottiene in matches il nexthop link
	//infine calcola i byte in coda applicabili e totali
    RouteEntryVec matchess;
    RouteEntryVec::iterator iters;
    long int  byteCount;
	dtn::Bundle * bundle;
    LinkRef null_link("UniboCGRBundleRouter::route_bundle");
    std::stringstream convert;
    std::string eids;
    int result = -1;
    convert << "ipn:";
    convert << neighbor;
    convert << ".0";
    convert >> eids;
    EndpointID* ied = new EndpointID(eids);
	route_table_->get_matching(*ied,null_link,&matchess);
	if(matchess.size() > 0)
	{
		iters = matchess.begin();
		RouteEntry* res = *iters;
		dtn::UniboCGRBundleRouter::DeferredList* deferredx;
		deferredx = deferred_list(res->link());
		const dtn::LinkRef& link = res->link();
		oasys::ScopeLock l(deferredx->list()->lock(),
					   "computeApplicableBacklog");
		dtn::BundleList::iterator iter = deferredx->list()->begin();
		result=0;
		while(iter != deferredx->list()->end())
		{
			bundle = *iter;
			//Capisci quanti byte occupa il bundle
			byteCount = 0;
			//Giacomo: per ora prendo la dim del payload e l'header lo considero una dim fissa
			byteCount += bundle->durable_size();
			byteCount += NOMINAL_PRIMARY_BLKSIZE;
			if(bundle->priority() >= priority)
			{
				//byte applicabili
				(*byteApp) += byteCount;
			}
			//in ogni caso li aggiungo ai totali
			(*byteTot) += byteCount;
			result++;
			++iter;
		}
		//link queue
		size_t dimLinkQueue = link->queue()->size();
		if(dimLinkQueue > 0)
		{
			const dtn::BundleList* bl = link->queue();
			oasys::ScopeLock l(bl->lock(),
										"computeApplicableBacklog2");
			dtn::BundleList::iterator bli;
			for(bli = bl->begin(); bli != bl->end(); ++bli)
			{
				bundle = *bli;
				byteCount = 0;
				//per ora prendo solo la dim del payload e l'header lo considero una dim fissa
				byteCount += bundle->durable_size();
				byteCount += NOMINAL_PRIMARY_BLKSIZE;
				if(bundle->priority() >= priority)
				{
					//byte applicabili
					(*byteApp) += byteCount;
				}
				(*byteTot) += byteCount;
				result++;
			}
		}
	}
	else
	{
		(*byteTot) = 0;
		(*byteApp) = 0;
		result = 0;
	}
    return result;
}

//----------------------------------------------------------------------
int
UniboCGRBundleRouter::route_bundle(Bundle* bundle, bool skip_check_next_hop)
{
    RouteEntryVec matches;
    RouteEntryVec::iterator iter;
    int result_cgr;

    // check to see if forwarding is suppressed to all nodes
    if (bundle->fwdlog()->get_count(EndpointIDPattern::WILDCARD_EID(),
                                    ForwardingInfo::SUPPRESSED) > 0)
    {
        log_info("route_bundle: "
                 "ignoring bundle %" PRIbid " since forwarding is suppressed",
                 bundle->bundleid());
        return 0;
    }

    
    LinkRef null_link("UniboCGRBundleRouter::route_bundle");
    //Giacomo: chiamo UniboCGR e prendo il risultato
    struct timeval tv;
    gettimeofday(&tv, NULL);
    std::string res = "";
    result_cgr = callUniboCGR(tv.tv_sec, bundle, &res);
    log_debug("unibocgr return %d with next hop: %s ",result_cgr, res);
    //Check: if UniboCGR failed use the normal "static" way
    //result_cgr is -8 when the bundle have DTN name instead of IPN.
    if(res == "" || result_cgr == -8)
    {
    	res = bundle->dest().str();
    	if(result_cgr==-8)
    	{
    		writeLog("Warning: bundle is using DTN name instead of IPN!");
    	}
    }
    std::vector<std::string> result;
    std::istringstream iss(res);
    for(std::string res; iss >> res; )
        result.push_back(res);
    for(int i = 0; i < result.size(); i++)
    {
    	EndpointID* eidRes = new EndpointID(result[i]);
    	route_table_->get_matching(*eidRes, null_link, &matches);
    }

    // sort the matching routes by priority, allowing subclasses to
    // override the way in which the sorting occurs
    sort_routes(bundle, &matches);

    //log_debug("route_bundle bundle id %" PRIbid ": checking %zu route entry matches",
    //          bundle->bundleid(), matches.size());

    bool forwarded;    
    unsigned int count = 0;
    for (iter = matches.begin(); iter != matches.end(); ++iter)
    {
        RouteEntry* route = *iter;

        //log_debug("checking route entry %p link %s (%p) [remote EID: %s]",
        //          *iter, route->link()->name(), route->link().object(), 
        //          route->link()->remote_eid().c_str());

        if (! should_fwd(bundle, *iter)) {
            continue;
        }

        DeferredList* dl = deferred_list(route->link());

        if (dl == 0)
          continue;

        if ( bundle->is_queued_on(dl->list()) ) {
            //log_debug("route_bundle bundle %" PRIbid ": "
            //          "ignoring link *%p since already deferred",
            //          bundle->bundleid(), route->link().object());
            continue;
        }

        // this call will add the bundle to the deferred list and should now be  
        // done before calling check_next_hop to prevent a bundle getting stuck
        // in the deferred list while a link is open
        forwarded = fwd_to_nexthop(bundle, *iter);

        // because there may be bundles that already have deferred
        // transmission on the link, we first call check_next_hop to
        // get them into the queue before trying to route the new
        // arrival, otherwise it might leapfrog the other deferred
        // bundles
        // XXX/dz bundles are now always added to the deferred list to maintain
        // the proper ordering and check_next_hop initiates the transmits
        if (!skip_check_next_hop) {
            check_next_hop(route->link());
        }
        
        if (forwarded ) {
            ++count;


            // only send it through the primary path if one exists
            if (BundleRouter::config_.static_router_prefer_always_on_ &&
                (route->link()->type() == Link::ALWAYSON)) {
                break;
            }
        }
    }

    //log_debug("route_bundle bundle id %" PRIbid ": forwarded on %u links",
    //          bundle->bundleid(), count);
    return count;
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::sort_routes(Bundle* bundle, RouteEntryVec* routes)
{
    (void)bundle;
    std::sort(routes->begin(), routes->end(), RoutePrioritySort());
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::check_next_hop(const LinkRef& next_hop)
{
    // if the link isn't open, there's nothing to do now
    if (! next_hop->isopen()) {
        //log_debug("check_next_hop %s -> %s: link not open...",
        //          next_hop->name(), next_hop->nexthop());
        return;
    }
    
    // if the link queue doesn't have space (based on the low water
    // mark) don't do anything
    if (! next_hop->queue_has_space()) {
        //log_debug("check_next_hop %s -> %s: no space in queue...",
        //          next_hop->name(), next_hop->nexthop());
        return;
    }

    //log_debug("check_next_hop %s -> %s: checking deferred bundle list...",
    //          next_hop->name(), next_hop->nexthop());

    // because the loop below will remove the current bundle from
    // the deferred list, invalidating any iterators pointing to its
    // position, make sure to advance the iterator before processing
    // the current bundle
    DeferredList* deferred = deferred_list(next_hop);

    // XXX/dz only move up to the Link high queue limit bundles in any one call
    // otherwise there is always space available in the queue and this loop
    // will not end until the entire list of bundles is transmitted.
    size_t bundles_moved = 0;
    size_t max_to_move = next_hop->params().qlimit_bundles_high_;

    // if queue was changed from active limits to not active then move all bundles
    if (0 == max_to_move)
    {
        max_to_move = deferred->list()->size();
    }

    BundleRef bundle("UniboCGRBundleRouter::check_next_hop");

    deferred->list()->lock()->lock(__func__);

    BundleList::iterator iter = deferred->list()->begin();
    while (iter != deferred->list()->end())
    {
        if (next_hop->queue_is_full()) {
            //log_debug("check_next_hop %s: link queue is full, stopping loop",
            //          next_hop->name());
            deferred->list()->lock()->unlock();
            break;
        }
        
        bundle = *iter;
        ++iter;

        ForwardingInfo info = deferred->info(bundle);

        // if should_fwd returns false, then the bundle was either
        // already transmitted or is in flight on another node. since
        // it's possible that one of the other transmissions will
        // fail, we leave it on the deferred list for now, relying on
        // the transmitted handlers to clean up the state
        if (! BundleRouter::should_fwd(bundle.object(), next_hop,
                                       info.action()))
        {
            //log_debug("check_next_hop: not forwarding to link %s",
            //          next_hop->name());
            continue;
        }
        
        // if the link is available and not open, open it
        if (next_hop->isavailable() &&
            (!next_hop->isopen()) && (!next_hop->isopening()))
        {
            log_debug("check_next_hop: "
                      "opening *%p because a message is intended for it",
                      next_hop.object());
            actions_->open_link(next_hop);
        }

        // remove the bundle from the deferred list
        deferred->del(bundle);

        // release the deferred list lock to prevent possible deadlock if a redirect to BIBE
        // takes place while queueing the bndle
        deferred->list()->lock()->unlock();


        //log_debug("check_next_hop: sending *%p to *%p",
        //          bundle.object(), next_hop.object());
        actions_->queue_bundle(bundle.object() , next_hop,
                               info.action(), info.custody_spec());

        // break out if we have now moved the max
        if (++bundles_moved >= max_to_move) {
            //log_debug("check_next_hop %s: moved max bundles to queue: %u",
            //          next_hop->name(), max_to_move);
            break;
        }

        deferred->list()->lock()->lock(__func__);

        iter = deferred->list()->begin();
    }

    if (deferred->list()->lock()->is_locked_by_me()) {
        deferred->list()->lock()->unlock();
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::reroute_all_bundles()
{
    // XXX/dz Instead of locking the pending_bundles_ during this whole
    // process, the lock is now only taken when the iterator is being
    // updated.
 
    log_debug("reroute_all_bundles... %zu bundles on pending list",
              pending_bundles_->size());

    // XXX/demmer this should cancel previous scheduled transmissions
    // if any decisions have changed

    pending_bundles_->lock()->lock("UniboCGRBundleRouter::reroute_all_bundles");
    pending_bundles_t::iterator iter = pending_bundles_->begin();
    if ( iter == pending_bundles_->end() ) {
      pending_bundles_->lock()->unlock();
      return;
    }
    pending_bundles_->lock()->unlock();

    bool skip_check_next_hop = false;
    while (true)
    {
        route_bundle(iter->second, skip_check_next_hop); // for <map> lists

        // only do the check_next_hop for the first bundle which 
        // primes the pump for all of the other bundles
        if (!skip_check_next_hop) {
            skip_check_next_hop = true;
        }

        pending_bundles_->lock()->lock("UniboCGRBundleRouter::reroute_all_bundles");
        ++iter;
        if ( iter == pending_bundles_->end() ) {
          pending_bundles_->lock()->unlock();
          break;
        }
        pending_bundles_->lock()->unlock();
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::recompute_routes()
{
    reroute_all_bundles();
}

//----------------------------------------------------------------------
UniboCGRBundleRouter::DeferredList::DeferredList(const char* logpath,
                                             const LinkRef& link)
    : RouterInfo(),
      Logger("%s/deferred/%s", logpath, link->name()),
      list_(link->name_str() + ":deferred"),
      count_(0)
{
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::DeferredList::dump_stats(oasys::StringBuffer* buf)
{
    buf->appendf(" -- %zu bundles_deferred", count_);
}

//----------------------------------------------------------------------
bool
UniboCGRBundleRouter::DeferredList::find(const BundleRef& bundle,
                                     ForwardingInfo* info)
{
    InfoMap::const_iterator iter = info_.find(bundle->bundleid());
    if (iter == info_.end()) {
        return false;
    }
    *info = iter->second;
    return true;
}

//----------------------------------------------------------------------
const ForwardingInfo&
UniboCGRBundleRouter::DeferredList::info(const BundleRef& bundle)
{
    InfoMap::const_iterator iter = info_.find(bundle->bundleid());
    ASSERT(iter != info_.end());
    return iter->second;
}

//----------------------------------------------------------------------
bool
UniboCGRBundleRouter::DeferredList::add(const BundleRef&      bundle,
                                    const ForwardingInfo& info)
{
    ASSERT(list_.lock()->is_locked_by_me());
 
    if (bundle->is_queued_on(&list_)) {
        log_err("bundle *%p already in deferred list!",
                bundle.object());
        return false;
    }

    //log_debug("adding *%p to deferred (size: %zu, count: %zu)",
    //          bundle.object(), list_.size(), count_);

    count_++;
    info_.insert(InfoMap::value_type(bundle->bundleid(), info));
    list_.push_back(bundle);

    return true;
}

//----------------------------------------------------------------------
bool
UniboCGRBundleRouter::DeferredList::del(const BundleRef& bundle)
{
    ASSERT(list_.lock()->is_locked_by_me());
 
    size_t n = info_.erase(bundle->bundleid());
    ASSERT(n == 1);
    
    if (! list_.erase(bundle)) {
        return false;
    }
    
    ASSERT(count_ > 0);
    count_--;
   
    //log_debug("removed *%p from deferred (size: %zu, count: %zu)",
    //          bundle.object(), list_.size(), count_);

    return true;
}

//----------------------------------------------------------------------
UniboCGRBundleRouter::DeferredList*
UniboCGRBundleRouter::deferred_list(const LinkRef& link)
{
    DeferredList* dq = dynamic_cast<DeferredList*>(link->router_info());
#if 0
    ASSERT(dq != NULL);
#endif
    return dq;
}


//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_registration_added(RegistrationAddedEvent* event)
{
    Registration* reg = event->registration_;
    
    if (reg == NULL || reg->session_flags() == 0) {
        return;
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_registration_removed(RegistrationRemovedEvent* event)
{
    (void)event;
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_registration_expired(RegistrationExpiredEvent* event)
{
    // XXX/demmer lookup session and remove reg from subscribers
    // and/or remove the whole session if reg is the custodian
    (void)event;
}

//----------------------------------------------------------------------
UniboCGRBundleRouter::RerouteTimer::RerouteTimer(UniboCGRBundleRouter* router, const LinkRef& link)
            : router_(router), link_(link)
{
}

//----------------------------------------------------------------------
UniboCGRBundleRouter::RerouteTimer::~RerouteTimer()
{
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::RerouteTimer::timeout(const struct timeval& now)
{
    (void)now;
    router_->reroute_bundles(link_);
}


} // namespace dtn
