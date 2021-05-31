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
This header is a modification of UniboCGRBundleRouter.h made by Giacomo Gori
in Spring 2021 to make it compatible with DTNME 1.0 beta under supervision of Carlo Caini
*/

#ifndef _UNIBO_CGR_BUNDLE_ROUTER_H_
#define _UNIBO_CGR_BUNDLE_ROUTER_H_

#include <third_party/oasys/util/StringUtils.h>

#include "BundleRouter.h"
#include "RouterInfo.h"
#include "bundling/BundleInfoCache.h"
#include "reg/Registration.h"

namespace dtn {

class BundleList;
class RouteEntryVec;
class RouteTable;

class UniboCGRBundleRouter : public BundleRouter {
public:
    UniboCGRBundleRouter(const char* classname, const std::string& name);
    UniboCGRBundleRouter();
   

    /**
     * Destructor.
     */
    virtual ~UniboCGRBundleRouter();
    virtual void shutdown();

    /**
     * Event handler overridden from BundleRouter / BundleEventHandler
     * that dispatches to the type specific handlers where
     * appropriate.
     */
    virtual void handle_event(BundleEvent* event);
    
    /// @{ Event handlers
    virtual void handle_bundle_received(BundleReceivedEvent* event);
    virtual void handle_bundle_transmitted(BundleTransmittedEvent* event);
    virtual void handle_bundle_cancelled(BundleSendCancelledEvent* event);
    virtual void handle_route_add(RouteAddEvent* event);
    virtual void handle_route_del(RouteDelEvent* event);
    virtual void handle_contact_up(ContactUpEvent* event);
    virtual void handle_contact_down(ContactDownEvent* event);
    virtual void handle_link_available(LinkAvailableEvent* event);
    virtual void handle_link_created(LinkCreatedEvent* event);
    virtual void handle_link_deleted(LinkDeletedEvent* event);
    virtual void handle_link_check_deferred(LinkCheckDeferredEvent* event);
    virtual void handle_custody_timeout(CustodyTimeoutEvent* event);
    virtual void handle_registration_added(RegistrationAddedEvent* event);
    virtual void handle_registration_removed(RegistrationRemovedEvent* event);
    virtual void handle_registration_expired(RegistrationExpiredEvent* event);
    /// @}


    /**
     * Dump the routing state.
     */
    void get_routing_state(oasys::StringBuffer* buf);

    /**
     * Get a tcl version of the routing state.
     */
    void tcl_dump_state(oasys::StringBuffer* buf);

    /**
     * Add a route entry to the routing table. 
     * Set skip_changed_routes to true to skip the call to 
     * handle_changed_routes if the initiating method is going to call it.
     */
    void add_route(RouteEntry *entry, bool skip_changed_routes=true);

    /**
     * Remove matrhing route entry(s) from the routing table. 
     */
    void del_route(const EndpointIDPattern& id);

    /**
     * Update forwarding state due to changed routes.
     */
    void handle_changed_routes();

    /**
     * Try to forward a bundle to a next hop route.
     */
    virtual bool fwd_to_nexthop(Bundle* bundle, RouteEntry* route);

    virtual int
   getBacklogForNode(unsigned long long neighbor, int priority, long int* byteApp, long int * byteTot);

    /**
     * Check if the bundle should be forwarded to the given next hop.
     * Reasons why it would not be forwarded include that it was
     * already transmitted or is currently in flight on the link, or
     * that the route indicates ForwardingInfo::FORWARD_ACTION and it
     * is already in flight on another route.
     */
    using BundleRouter::should_fwd;
    virtual bool should_fwd(const Bundle* bundle, RouteEntry* route);
    
    /**
     * Call the UniboCGR and get next hops that match the given bundle and
     * have not already been found in the bundle history. If a match
     * is found, call fwd_to_nexthop on it.
     * Set skip_check_next_hop to true to skip the call to 
     * check_next_hop().
     *
     * @param bundle		the bundle to forward
     *
     * Returns the number of links on which the bundle was queued
     * (i.e. the number of matching route entries.
     */
    virtual int route_bundle(Bundle* bundle, bool skip_check_next_hop=false);

    /**
     * Once a vector of matching routes has been found, sort the
     * vector. The default uses the route priority, breaking ties by
     * using the number of bytes queued.
     */
    virtual void sort_routes(Bundle* bundle, RouteEntryVec* routes);
    
    /**
     * Called when the next hop link is available for transmission
     * (i.e. either when it first arrives and the contact is brought
     * up or when a bundle is completed and it's no longer busy).
     *
     * Loops through the bundle list and calls fwd_to_matching on all
     * bundles.
     */
    virtual void check_next_hop(const LinkRef& next_hop);

    /**
     * Go through all known bundles in the system and try to re-route them.
     */
    virtual void reroute_all_bundles();

    /**
     * Generic hook in response to the command line indication that we
     * should reroute all bundles.
     */
    virtual void recompute_routes();

    /**
     * When new links are added or opened, and if we're configured to
     * add nexthop routes, try to add a new route for the given link.
     * Set skip_changed_routes to true to skip the call to 
     * handle_changed_routes if the initiating method is going to call it.
     */
    void add_nexthop_route(const LinkRef& link, bool skip_changed_routes=false);

    /**
     * Hook to ask the router if the bundle can be deleted.
     */
    bool can_delete_bundle(const BundleRef& bundle);
    
    /**
     * Hook to tell the router that the bundle should be deleted.
     */
    void delete_bundle(const BundleRef& bundle);

    /**
     * Remove matching deferred transmission entries.
     */
    void remove_from_deferred(const BundleRef& bundle, int actions);
    
    /// Cache to check for duplicates and to implement a simple RPF check
    BundleInfoCache reception_cache_;

    /// The routing table
    RouteTable* route_table_;

    /// Timer class used to cancel transmission on down links after
    /// waiting for them to potentially reopen
    class RerouteTimer;
    typedef std::shared_ptr<RerouteTimer> SPtr_RerouteTimer;

    class RerouteTimer : public oasys::SharedTimer {
    public:
        RerouteTimer(UniboCGRBundleRouter* router, const LinkRef& link);
         

        virtual ~RerouteTimer();

        virtual void timeout(const struct timeval& now) override;

    protected:
        UniboCGRBundleRouter* router_;
        LinkRef link_;
        oasys::SPtr_Timer sptr_;
        uint32_t seconds_ = 30;
    };

    friend class RerouteTimer;

    /// Helper function for rerouting
    void reroute_bundles(const LinkRef& link);
    
    /// Table of reroute timers, indexed by the link name
    typedef oasys::StringMap<SPtr_RerouteTimer> RerouteTimerMap;
    RerouteTimerMap reroute_timers_;

    /// Per-link class used to store deferred transmission bundles
    /// that helps cache route computations
    class DeferredList : public RouterInfo, public oasys::Logger {
    public:
        DeferredList(const char* logpath, const LinkRef& link);

        /// Accessor for the bundle list
        BundleList* list() { return &list_; }

        /// Accessor for the forwarding info associated with the
        /// bundle, which must be on the list
        const ForwardingInfo& info(const BundleRef& bundle);

        /// Check if the bundle is on the list. If so, return its
        /// forwarding info.
        bool find(const BundleRef& bundle, ForwardingInfo* info);

        /// Add a new bundle/info pair to the deferred list
        bool add(const BundleRef& bundle, const ForwardingInfo& info);

        /// Remove the bundle and its associated forwarding info from
        /// the list
        bool del(const BundleRef& bundle);

        /// Print out the stats, called from Link::dump_stats
        void dump_stats(oasys::StringBuffer* buf);
        
    protected:
        typedef std::map<bundleid_t, ForwardingInfo> InfoMap;
        BundleList list_;
        InfoMap    info_;
        size_t     count_;
    };

    /// Helper accessor to return the deferred queue for a link
    DeferredList* deferred_list(const LinkRef& link);

};

} // namespace dtn

#endif /* _UNIBO_CGR_BUNDLE_ROUTER_H_ */
