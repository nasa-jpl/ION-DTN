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
*  This is a modification of BundleRouter made by Giacomo Gori
*  on Summer 2020 with Carlo Caini as supervisor.
*  It's purpose is to make possible for DTN2 to work with interface_unibocgr_dtn2
*  to use Unibo-CGR as routing mechanism
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
#include "session/Session.h"

//Interface for Unibo-CGR
#include "Unibo-CGR/dtn2/interface/interface_unibocgr_dtn2.h"
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
    reroute_all_sessions();
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_event(BundleEvent* event)
{
    dispatch_event(event);
}

//----------------------------------------------------------------------
Session*
UniboCGRBundleRouter::get_session_for_bundle(Bundle* bundle)
{
    if (bundle->session_flags() != 0)
    {
        log_debug("get_session_for_bundle: bundle id %"PRIbid" is a subscription msg",
                  bundle->bundleid());
        return NULL;
    }

    if (bundle->sequence_id().empty()  &&
        bundle->obsoletes_id().empty() &&
        bundle->session_eid().length() == 0)
    {
        log_debug("get_session_for_bundle: bundle id %"PRIbid" not a session bundle",
                  bundle->bundleid());
        return NULL;
    }

    EndpointID session_eid = bundle->session_eid();
    if (session_eid.length() == 0)
    {
        session_eid.assign(std::string("dtn-unicast-session:") +
                           bundle->source().str() +
                           "," +
                           bundle->dest().str());
        ASSERT(session_eid.valid());
    }

    Session* session = sessions_.get_session(session_eid);
    log_debug("get_session_for_bundle: *%p *%p", bundle, session);
    return session;
}

//----------------------------------------------------------------------
bool
UniboCGRBundleRouter::add_bundle_to_session(Bundle* bundle, Session* session)
{
    // XXX/demmer is this the right deletion reason for obsoletes??
    static BundleProtocol::status_report_reason_t deletion_reason =
        BundleProtocol::REASON_DEPLETED_STORAGE;
    
    log_debug("adding *%p to *%p", bundle, session);

    if (! bundle->sequence_id().empty())
    {
        oasys::ScopeLock l(session->bundles()->lock(),
                           "UniboCGRBundleRouter::add_subscriber");
        BundleList::iterator iter = session->bundles()->begin();
        while (iter != session->bundles()->end())
        {
            Bundle* old_bundle = *iter;
            ++iter; // in case we remove the bundle from the list

            // make sure the old bundle has a sequence id
            if (old_bundle->sequence_id().empty()) {
                continue;
            }

            // first check if the newly arriving bundle causes an old one
            // to be obsolete
            if (bundle->obsoletes_id() >= old_bundle->sequence_id())
            {
                log_debug("*%p obsoletes *%p... removing old bundle",
                          bundle, old_bundle);
            
                bool ok = session->bundles()->erase(old_bundle);
                ASSERT(ok);
                BundleDaemon::post_at_head(
                    new BundleDeleteRequest(old_bundle, deletion_reason));
                continue;
            }

            // next check if the existing bundle obsoletes this one
            if (old_bundle->obsoletes_id() >= bundle->sequence_id())
            {
                log_debug("*%p obsoletes *%p... ignoring new arrival",
                          old_bundle, bundle);
                BundleDaemon::post_at_head(
                    new BundleDeleteRequest(bundle, deletion_reason));
                return false;
            }

            // now check if the new and existing bundles have the same
            // sequence id, in which case we discard the new arrival as
            // well
            if (bundle->sequence_id() == old_bundle->sequence_id())
            {
                log_debug("*%p and *%p have same sequence id... "
                          "ignoring new arrival",
                          old_bundle, bundle);
                BundleDaemon::post_at_head(
                    new BundleDeleteRequest(bundle, deletion_reason));
                return false;
            }
            
            log_debug("compared *%p and *%p, nothing is obsoleted",
                      old_bundle, bundle);
        }
    }

    session->bundles()->push_back(bundle);
    session->sequence_id()->update(bundle->sequence_id());

    return true;
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_bundle_received(BundleReceivedEvent* event)
{
    bool should_route = true;
    
    Bundle* bundle = event->bundleref_.object();
    log_debug("handle bundle received: *%p", bundle);

    EndpointID remote_eid(EndpointID::NULL_EID());

    if (event->link_ != NULL) {
        remote_eid = event->link_->remote_eid();
    }

    if (! reception_cache_.add_entry(bundle, remote_eid))
    {
        log_info("ignoring duplicate bundle: *%p", bundle);
        BundleDaemon::post_at_head(
            new BundleDeleteRequest(bundle, BundleProtocol::REASON_NO_ADDTL_INFO));
        return;
    }

    // check if the bundle is part of a session, either because it has
    // a sequence id and/or obsoletes id, or because it has an
    // explicit session eid. if it is part of the session, add it to
    // the session list
    Session* session = get_session_for_bundle(bundle);
    if (session != NULL)
    {
        // add the bundle to the session list, which checks whether 
        // it obsoletes any existing bundles on the session, as well
        // as whether the bundle itself is obsolete on arrival.
        should_route = add_bundle_to_session(bundle, session);
        if (! should_route) {
            log_debug("session bundle %"PRIbid" is DOA", bundle->bundleid());
            return; // don't route it 
        }
    }

    // check if the bundle is a session subscription management bundle
    // XXX/demmer maybe use a registration instead??
    if (bundle->session_flags() != 0) {
        should_route = handle_session_bundle(event);
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

        //dzdebug
        oasys::ScopeLock l(deferred->list()->lock(), 
                           "UniboCGRBundleRouter::remove_from_deferred");

        ForwardingInfo info;
        if (deferred->find(bundle, &info))
        {
            if (info.action() & actions) {
                log_debug("removing bundle *%p from link *%p deferred list",
                          bundle.object(), (*iter).object());
                deferred->del(bundle);
            }
        }
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_bundle_transmitted(BundleTransmittedEvent* event)
{
    const BundleRef& bundle = event->bundleref_;
    log_debug("handle bundle transmitted (%s): *%p", 
              event->success_?"success":"failure", bundle.object());

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
    log_debug("UniboCGRBundleRouter::can_delete_bundle: checking if we can delete *%p",
              bundle.object());

    // check if we haven't yet done anything with this bundle
    if (bundle->fwdlog()->get_count(ForwardingInfo::TRANSMITTED |
                                    ForwardingInfo::DELIVERED) == 0)
    {
        log_debug("UniboCGRBundleRouter::can_delete_bundle(%"PRIbid"): "
                  "not yet transmitted or delivered",
                  bundle->bundleid());
        return false;
    }

    // check if we have local custody
    if (bundle->local_custody()) {
        log_debug("UniboCGRBundleRouter::can_delete_bundle(%"PRIbid"): "
                  "not deleting because we have custody",
                  bundle->bundleid());
        return false;
    }

    // check if the bundle is part of a session with subscribers
    Session* session = get_session_for_bundle(bundle.object());
    if (session && !session->subscribers().empty())
    {
        log_debug("UniboCGRBundleRouter::can_delete_bundle(%"PRIbid"): "
                  "session has subscribers",
                  bundle->bundleid());
        return false;
    }

    return true;
}
    
//----------------------------------------------------------------------
void
UniboCGRBundleRouter::delete_bundle(const BundleRef& bundle)
{
    log_debug("delete *%p", bundle.object());

    remove_from_deferred(bundle, ForwardingInfo::ANY_ACTION);

    Session* session = get_session_for_bundle(bundle.object());
    if (session)
    {
        bool ok = session->bundles()->erase(bundle);
        (void)ok;
        
        log_debug("delete_bundle: removing *%p from *%p: %s",
                  bundle.object(), session, ok ? "success" : "not in session list");

        // XXX/demmer adjust sequence id for session??
    }


    // XXX/demmer clean up empty sessions?
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_bundle_cancelled(BundleSendCancelledEvent* event)
{
    Bundle* bundle = event->bundleref_.object();
    log_debug("handle bundle cancelled: *%p", bundle);

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
    if (reception_cache_.lookup(bundle, &prevhop))
    {
        if (prevhop == route->link()->remote_eid() &&
            prevhop != EndpointID::NULL_EID())
        {
            log_debug("should_fwd bundle %"PRIbid": "
                      "skip %s since bundle arrived from the same node",
                      bundle->bundleid(), route->link()->name());
            return false;
        }
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

    RerouteTimerMap::iterator iter = reroute_timers_.find(link->name_str());
    if (iter != reroute_timers_.end()) {
        log_debug("link %s reopened, cancelling reroute timer", link->name());
        RerouteTimer* t = iter->second;
        reroute_timers_.erase(iter);
        t->cancel();
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
            RerouteTimer* t = new RerouteTimer(this, link);
            t->schedule_in(link->params().potential_downtime_ * 1000);
            
            reroute_timers_[link->name_str()] = t;
        }
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::RerouteTimer::timeout(const struct timeval& now)
{
    (void)now;
    router_->reroute_bundles(link_);
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
        ASSERT(! bundle->is_queued_on(link->queue()));
    }

    // there should never have been any in flight since the link is
    // unavailable
    ASSERT(link->inflight()->empty());
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
        RerouteTimer* t = iter->second;
        reroute_timers_.erase(iter);
        t->cancel();
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::handle_link_check_deferred(LinkCheckDeferredEvent* event)
{
    log_debug("LinkCheckDeferred event for *%p", event->linkref_.object());
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

    if (!sessions_.empty())
    {
        buf->appendf("Session table (%zu sessions):\n", sessions_.size());
        sessions_.dump(buf);
        buf->appendf("\n");
    }

    if (!session_custodians_.empty())
    {
        buf->appendf("Session custodians (%zu registrations):\n",
                     session_custodians_.size());

        for (RegistrationList::iterator iter = session_custodians_.begin();
             iter != session_custodians_.end(); ++iter)
        {
            buf->appendf("    *%p\n", *iter);
        }
        buf->appendf("\n");
    }
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
    DeferredList* deferred = dtn::UniboCGRBundleRouter::deferred_list(link);
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
    } else {
        log_warn("bundle *%p already exists on deferred list of link *%p",
                 bundle, link.object());
    }
   
    // XXX/dz could check to see if there is space available on the queue 
    // and call check_next_hop here if needed...

    return false;
}

//----------------------------------------------------------------------
int
UniboCGRBundleRouter::getBacklogForNode(unsigned long long neighbor, int priority, long int* byteApp, long int * byteTot)
{
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
    log_debug("route_bundle: checking bundle %"PRIbid, bundle->bundleid());

    // check to see if forwarding is suppressed to all nodes
    if (bundle->fwdlog()->get_count(EndpointIDPattern::WILDCARD_EID(),
                                    ForwardingInfo::SUPPRESSED) > 0)
    {
        log_info("route_bundle: "
                 "ignoring bundle %"PRIbid" since forwarding is suppressed",
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
    log_debug("route_bundle bundle id %"PRIbid": checking %zu route entry matches",
              bundle->bundleid(), matches.size());
    bool forwarded;    
    unsigned int count = 0;
    for (iter = matches.begin(); iter != matches.end(); ++iter)
    {
        RouteEntry* route = *iter;
        log_debug("checking route entry %p link %s (%p)",
                  *iter, route->link()->name(), route->link().object());

        if (! should_fwd(bundle, *iter)) {
            continue;
        }

        DeferredList* dl = dtn::UniboCGRBundleRouter::deferred_list(route->link());

        if (dl == 0)
          continue;

        if ( bundle->is_queued_on(dl->list()) ) {
            log_debug("route_bundle bundle %"PRIbid": "
                      "ignoring link *%p since already deferred",
                      bundle->bundleid(), route->link().object());
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
        }
    }

    log_debug("route_bundle bundle id %"PRIbid": forwarded on %u links",
              bundle->bundleid(), count);
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
        log_debug("check_next_hop %s -> %s: link not open...",
                  next_hop->name(), next_hop->nexthop());
        return;
    }
    
    // if the link queue doesn't have space (based on the low water
    // mark) don't do anything
    if (! next_hop->queue_has_space()) {
        log_debug("check_next_hop %s -> %s: no space in queue...",
                  next_hop->name(), next_hop->nexthop());
        return;
    }

    log_debug("check_next_hop %s -> %s: checking deferred bundle list...",
              next_hop->name(), next_hop->nexthop());

    // because the loop below will remove the current bundle from
    // the deferred list, invalidating any iterators pointing to its
    // position, make sure to advance the iterator before processing
    // the current bundle
    DeferredList* deferred = dtn::UniboCGRBundleRouter::deferred_list(next_hop);

    // XXX/dz only move up to the Link high queue limit bundles in any one call
    // otherwise there is always space available in the queue and this loop
    // will not end until the entire list of bundles is transmitted.
    u_int bundles_moved = 0;
    u_int max_to_move = next_hop->params().qlimit_bundles_high_;
    BundleRef bundle("UniboCGRBundleRouter::check_next_hop");

    oasys::ScopeLock l(deferred->list()->lock(), 
                       "UniboCGRBundleRouter::check_next_hop");
    BundleList::iterator iter = deferred->list()->begin();
    while (iter != deferred->list()->end())
    {
        if (next_hop->queue_is_full()) {
            log_debug("check_next_hop %s: link queue is full, stopping loop",
                      next_hop->name());
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
            log_debug("check_next_hop: not forwarding to link %s",
                      next_hop->name());
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
    
        log_debug("check_next_hop: sending *%p to *%p",
                  bundle.object(), next_hop.object());
        actions_->queue_bundle(bundle.object() , next_hop,
                               info.action(), info.custody_spec());

        // break out if we have now moved the max
        if (++bundles_moved >= max_to_move) {
            log_debug("check_next_hop %s: moved max bundles to queue: %u",
                      next_hop->name(), max_to_move);
            break;
        }
    }
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::reroute_all_bundles()
{
    // XXX/dz Instead of locking the pending_bundles_ during this whole
    // process, the lock is now only taken when the iterator is being
    // updated.
 
#ifdef BDSTATS_ENABLED
    // useful info if working with BD Stats
    log_crit("reroute_all_bundles - entered");
    oasys::Time now;
    now.get_time();
#endif


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
        #ifdef PENDING_BUNDLES_IS_MAP
            route_bundle(iter->second, skip_check_next_hop); // for <map> lists
        #else
            route_bundle(*iter, skip_check_next_hop); // for <lists> lists
        #endif

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

#ifdef BDSTATS_ENABLED
    log_crit("reroute_all_bundles - exit - elapsed: %d", now.elapsed_ms());
#endif // BDSTATS_ENABLED
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

    log_debug("adding *%p to deferred (size: %zu, count: %xu)",
              bundle.object(), list_.size(), count_);

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
   
    log_debug("removed *%p from deferred (size: %zu, count: %zu)",
              bundle.object(), list_.size(), count_);

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

    log_debug("got new session registration %u", reg->regid());

    if (reg->session_flags() & Session::CUSTODY) {
        log_debug("session custodian registration %u", reg->regid());
        session_custodians_.push_back(reg);
    }

    else if (reg->session_flags() & Session::SUBSCRIBE) {
        log_debug("session subscription registration %u", reg->regid());
        Session* session = sessions_.get_session(reg->endpoint());
        session->add_subscriber(Subscriber(reg));
        subscribe_to_session(Session::SUBSCRIBE, session);
    }

    else if (reg->session_flags() & Session::PUBLISH) {
        log_debug("session publish registration %u", reg->regid());

        Session* session = sessions_.get_session(reg->endpoint());
        if (session->upstream().is_null()) {
            log_debug("unknown upstream for publish registration... "
                      "trying to find one");
            find_session_upstream(session);
        }

        // XXX/demmer do something about publish
    }
}

//----------------------------------------------------------------------
bool
UniboCGRBundleRouter::subscribe_to_session(int mode, Session* session)
{
    if (! session->upstream().is_local()) {
        // XXX/demmer should set replyto to handle upstream nodes that
        // don't understand the session block

        Bundle* bundle = new TempBundle();
        bundle->set_do_not_fragment(1);
        bundle->mutable_source()->assign(BundleDaemon::instance()->local_eid());
        bundle->mutable_dest()->assign("dtn-session:" + session->eid().str());
        bundle->mutable_replyto()->assign(EndpointID::NULL_EID());
        bundle->mutable_custodian()->assign(EndpointID::NULL_EID());
        bundle->set_expiration(config_.subscription_timeout_);
        bundle->set_singleton_dest(true);
        bundle->mutable_session_eid()->assign(session->eid());
        bundle->set_session_flags(mode);
        bundle->mutable_sequence_id()->assign(*session->sequence_id());

        log_debug("sending subscribe bundle to session %s (timeout %u seconds)",
                  session->eid().c_str(), config_.subscription_timeout_);
        
        BundleDaemon::post_at_head(
            new BundleReceivedEvent(bundle, EVENTSRC_ROUTER));

        if (session->resubscribe_timer() != NULL) {
            log_debug("cancelling old resubscribe timer");
            session->resubscribe_timer()->cancel();
        }
        
        u_int resubscribe_timeout = config_.subscription_timeout_ * 1000 / 2;
        log_debug("scheduling resubscribe timer in %u msecs",
                  resubscribe_timeout);
        ResubscribeTimer* timer = new ResubscribeTimer(this, session);
        timer->schedule_in(resubscribe_timeout);
        session->set_resubscribe_timer(timer);
        
    } else {
        // XXX/demmer todo
        log_debug("local upstream source: notifying registration");
    }

    return true;
}

//----------------------------------------------------------------------
UniboCGRBundleRouter::ResubscribeTimer::ResubscribeTimer(UniboCGRBundleRouter* router,
                                                     Session* session)
    : router_(router), session_(session)
{
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::ResubscribeTimer::timeout(const struct timeval& now)
{
    (void)now;
    router_->logf(oasys::LOG_DEBUG, "resubscribe timer fired for session *%p",
                  session_);
    router_->subscribe_to_session(Session::RESUBSCRIBE, session_);
    session_->set_resubscribe_timer(NULL);
    delete this;
}

//----------------------------------------------------------------------
bool
UniboCGRBundleRouter::handle_session_bundle(BundleReceivedEvent* event)
{
    Bundle* bundle = event->bundleref_.object();

    ASSERT(bundle->session_flags() != 0);
    ASSERT(bundle->session_eid() != EndpointID::NULL_EID());
    
    Session* session = sessions_.get_session(bundle->session_eid());

    log_debug("handle_session_bundle: got bundle *%p for session %d",
              bundle, session->id());
              
    // XXX/demmer handle reload from db...
    if (event->source_ == EVENTSRC_STORE) {
        log_err("handle_session_bundle: can't handle reload from db yet");
        return false;
    }

    bool should_route = true;
    switch (bundle->session_flags()) {
    case Session::SUBSCRIBE:
    case Session::RESUBSCRIBE:
    {
        // look for whether we have an upstream route yet. if not,
        // keep the bundle in queue to forward onwards towards the
        // session root
        if (session->upstream().is_null()) {
            log_debug("handle_session_bundle: "
                      "unknown upstream... trying to find one");
            
            if (find_session_upstream(session))
            {
                ASSERT(!session->upstream().is_null());
                
                const Subscriber& upstream = session->upstream();
                if (upstream.is_local())
                {
                    log_debug("handle_session_bundle: "
                              "forwarding %s bundle to upstream registration",
                              Session::flag_str(bundle->session_flags()));
                    upstream.reg()->session_notify(bundle);
                    should_route = false;
                }
                else
                {
                    log_debug("handle_session_bundle: "
                              "found upstream *%p... routing bundle",
                              &upstream);
                }
            }
            else
            {
                // XXX/demmer what to do here? maybe if we add
                // something to ack the subscription then this should
                // defer the ack?
                log_info("can't find an upstream for session %s... "
                         "waiting until route arrives",
                         session->eid().c_str());
            }
        }
        else
        {
            const Subscriber& upstream = session->upstream();
            log_debug("handle_session_bundle: "
                      "already subscribed to session through upstream *%p... "
                      "suppressing subscription bundle %"PRIbid,
                      &upstream, bundle->bundleid());

            bundle->fwdlog()->add_entry(EndpointIDPattern::WILDCARD_EID(),
                                        ForwardingInfo::FORWARD_ACTION,
                                        ForwardingInfo::SUPPRESSED);
            should_route = false;
        }
        
        // add the new subscriber to the session. if the downstream is
        // already subscribed, then add_subscriber doesn't do
        // anything. XXX/demmer it should reset the stale subscription
        // timer...
        if (event->source_ == EVENTSRC_PEER)
        {
            if (bundle->prevhop().str() != "" &&
                bundle->prevhop()       != EndpointID::NULL_EID())
            {
                log_debug("handle_session_bundle: "
                          "adding downstream subscriber %s (seqid *%p)",
                          bundle->prevhop().c_str(), &bundle->sequence_id());
                
                add_subscriber(session, bundle->prevhop(), bundle->sequence_id());
            }
            else
            {
                // XXX/demmer what to do here??
                log_err("handle_session_bundle: "
                        "downstream subscriber with no prevhop!!!!");
            }
        }
        break;
    }

    default:
    {
        log_err("session flags %x not implemented", bundle->session_flags());
    }
    }

    return should_route;
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::reroute_all_sessions()
{
    log_debug("reroute_all_bundles... %zu sessions",
              sessions_.size());

    for (SessionTable::iterator iter = sessions_.begin();
         iter != sessions_.end(); ++iter)
    {
        find_session_upstream(iter->second);
    }
}

//----------------------------------------------------------------------
bool
UniboCGRBundleRouter::find_session_upstream(Session* session)
{
    // first look for a local custody registration
    for (RegistrationList::iterator iter = session_custodians_.begin();
         iter != session_custodians_.end(); ++iter)
    {
        Registration* reg = *iter;
        if (reg->endpoint().match(session->eid())) {
            Subscriber new_upstream(reg);
            if (session->upstream() == new_upstream) {
                log_debug("find_session_upstream: "
                          "session %s upstream custody registration %d unchanged",
                          session->eid().c_str(), reg->regid());
            } else {
                log_debug("find_session_upstream: "
                          "session %s found new custody registration %d",
                          session->eid().c_str(), reg->regid());
                session->set_upstream(new_upstream);
            }
            return true;
        }
    }

    // XXX/demmer for now this just looks up the route for the
    // bundle destination (which should be in the dtn-session: scheme)
    // and extracts the next hop from that
    RouteEntryVec matches;
    RouteEntryVec::iterator iter;
    
    EndpointID subscribe_eid("dtn-session:" + session->eid().str());
    route_table_->get_matching(subscribe_eid, &matches);

    // XXX/demmer do something about this...
    // sort_routes(bundle, &matches);

    for (iter = matches.begin(); iter != matches.end(); ++iter)
    {
        const LinkRef& link = (*iter)->link();
        if (link->remote_eid().str() == "" ||
            link->remote_eid() == EndpointID::NULL_EID())
        {
            log_warn("find_session_upstream: "
                     "got route match with no remote eid");
            // XXX/demmer uh...
            continue;
        }

        Subscriber new_upstream(link->remote_eid());
        if (session->upstream() == new_upstream) {
            log_debug("find_session_upstream: "
                      "session %s found existing upstream %s",
                      session->eid().c_str(), link->remote_eid().c_str());
        } else {
            log_debug("find_session_upstream: session %s new upstream %s",
                      session->eid().c_str(), link->remote_eid().c_str());
            session->set_upstream(Subscriber(link->remote_eid()));
            add_subscriber(session, link->remote_eid(), SequenceID());
        }
        return true;
    }

    log_warn("find_session_upstream: can't find upstream for session %s",
             session->eid().c_str());
    return false;
}

//----------------------------------------------------------------------
void
UniboCGRBundleRouter::add_subscriber(Session*          session,
                                 const EndpointID& peer,
                                 const SequenceID& known_seqid)
{
    log_debug("adding new subscriber for session %s -> %s",
              session->eid().c_str(), peer.c_str());
    
    session->add_subscriber(Subscriber(peer));

    // XXX/demmer check for duplicates?
    
    RouteEntry *entry = new RouteEntry(session->eid(), peer);
    entry->set_action(ForwardingInfo::COPY_ACTION);
    route_table_->add_entry(entry);

    log_debug("routing %zu session bundles", session->bundles()->size());
    oasys::ScopeLock l(session->bundles()->lock(),
                       "UniboCGRBundleRouter::add_subscriber");
    for (BundleList::iterator iter = session->bundles()->begin();
         iter != session->bundles()->end(); ++iter)
    {
        Bundle* bundle = *iter;
        if (! bundle->sequence_id().empty() &&
            bundle->sequence_id() <= known_seqid)
        {
            log_debug("suppressing transmission of bundle %"PRIbid" (seqid *%p) "
                      "to subscriber %s since covered by seqid *%p",
                      bundle->bundleid(), &bundle->sequence_id(),
                      peer.c_str(), &known_seqid);
            bundle->fwdlog()->add_entry(peer, ForwardingInfo::COPY_ACTION,
                                        ForwardingInfo::SUPPRESSED);
            continue;
        }

        route_bundle(*iter);
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


} // namespace dtn
