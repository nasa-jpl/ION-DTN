/*
 *    Copyright 2004-2006 Intel Corporation
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

#ifdef HAVE_CONFIG_H
#  include <dtn-config.h>
#endif

#include <stdlib.h>

#include "BundleRouter.h"
#include "bundling/BundleDaemon.h"
#include "bundling/BundleActions.h"
#include "bundling/BundleList.h"
#include "storage/BundleStore.h"

#include "StaticBundleRouter.h"
#include "ExternalRouter.h"
#include "UniboCGRBundleRouter.h"

namespace dtn {

//----------------------------------------------------------------------
BundleRouter::Config::Config()
    : type_("static"),
      add_nexthop_routes_(true),
      open_discovered_links_(true),
      default_priority_(0),
      max_route_to_chain_(10),
      storage_quota_(0),
      subscription_timeout_(600),
      static_router_prefer_always_on_(true),
      auto_deliver_bundles_(true)
{}

BundleRouter::Config BundleRouter::config_;

//----------------------------------------------------------------------
BundleRouter*
BundleRouter::create_router(const char* type)
{
    if (strcmp(type, "static") == 0) {
        return new StaticBundleRouter();
    }
    else if (strcmp(type, "uniboCGR") == 0) {
        return new UniboCGRBundleRouter();
    }
    else if (strcmp(type, "external") == 0) {
        return new ExternalRouter();
    }    
    else {
        PANIC("unknown type %s for router", type);
    }
}

//----------------------------------------------------------------------
BundleRouter::BundleRouter(const char* classname, const std::string& name)
    : BundleEventHandler(classname, "/dtn/route"),
      name_(name)
{
    logpathf("/dtn/route/%s", name.c_str());
    
    actions_ = BundleDaemon::instance()->actions();
    
    // XXX/demmer maybe change this?
    pending_bundles_ = BundleDaemon::instance()->pending_bundles();
    custody_bundles_ = BundleDaemon::instance()->custody_bundles();
}

//----------------------------------------------------------------------
bool
BundleRouter::should_fwd(const Bundle* bundle, const LinkRef& link,
                         ForwardingInfo::action_t action)
{
    ForwardingInfo info;
    bool found = bundle->fwdlog()->get_latest_entry(link, &info);

    if (found) {
        ASSERT(info.state() != ForwardingInfo::NONE);
    } else {
        ASSERT(info.state() == ForwardingInfo::NONE);
    }

    // check if we've already sent or are in the process of sending
    // the bundle on this link
    if (info.state() == ForwardingInfo::TRANSMITTED)
    {
        log_debug("should_fwd bundle %" PRIbid ": "
                  "skip %s due to forwarding log entry %s",
                  bundle->bundleid(), link->name(),
                  ForwardingInfo::state_to_str(info.state()));
        return false;
    }

    if (info.state() == ForwardingInfo::QUEUED)
    {
        // verify that the bundle is actually queued - we might be reloading from storage at startup
        if (const_cast<Bundle*>(bundle)->is_queued_on(link->queue())) {
            log_debug("should_fwd bundle %" PRIbid ": "
                      "skip %s due to confirmed (in queue) forwarding log entry %s",
                      bundle->bundleid(), link->name(),
                      ForwardingInfo::state_to_str(info.state()));
            return false;
        } else if (const_cast<Bundle*>(bundle)->is_queued_on(link->inflight())) {
            log_debug("should_fwd bundle %" PRIbid ": "
                      "skip %s due to confirmed (inflight) forwarding log entry %s",
                      bundle->bundleid(), link->name(),
                      ForwardingInfo::state_to_str(info.state()));
            return false;
        } else {
            // add entry indicating previous transmit attempt failed and continue processing 
            const_cast<Bundle*>(bundle)->fwdlog()->update(link, ForwardingInfo::TRANSMIT_FAILED);
            if (const_cast<Bundle*>(bundle)->xmit_blocks()->find_blocks(link) != NULL) {
                BundleProtocol::delete_blocks(const_cast<Bundle*>(bundle), link);
            }
            bundle->fwdlog()->get_latest_entry(link, &info);

            log_debug("should_fwd bundle %" PRIbid ": "
                      "not queued as fwdlog indicates - updated to re-route on %s state now: %s",
                      bundle->bundleid(), link->name(),
                      ForwardingInfo::state_to_str(info.state()));
        }
    }

    // check if we've already sent or are in the process of sending
    // the bundle to the node via some other link
    if (link->remote_eid() != EndpointID::NULL_EID())
    {
        size_t count = bundle->fwdlog()->get_count(
            link->remote_eid(),
            ForwardingInfo::TRANSMITTED | ForwardingInfo::QUEUED);
        
        if (count > 0)
        {
            log_debug("should_fwd bundle %" PRIbid ": "
                      "skip %s since already sent or queued %zu times for remote eid %s",
                      bundle->bundleid(), link->name(),
                      count, link->remote_eid().c_str());
            return false;
        }

        // check whether transmission was suppressed. this could be
        // coupled with the previous one but it's better to have a
        // separate log message
        count = bundle->fwdlog()->get_count(
            link->remote_eid(),
            ForwardingInfo::SUPPRESSED);
        
        if (count > 0)
        {
            log_debug("should_fwd bundle %" PRIbid ": "
                      "skip %s since transmission suppressed to remote eid %s",
                      bundle->bundleid(), link->name(),
                      link->remote_eid().c_str());
            return false;
        }
    }
    
    // if the bundle has a a singleton destination endpoint, then
    // check if we already forwarded it or are planning to forward it
    // somewhere else. if so, we shouldn't forward it again
    if (bundle->singleton_dest() && action == ForwardingInfo::FORWARD_ACTION)
    {
        size_t count = bundle->fwdlog()->get_count(
            ForwardingInfo::TRANSMITTED |
            ForwardingInfo::QUEUED,
            action);

        if (count > 0) {
            log_debug("should_fwd bundle %" PRIbid ": "
                      "skip %s since already transmitted or queued (count %zu)",
                      bundle->bundleid(), link->name(), count);
            return false;
        } else {
//            log_debug("should_fwd bundle %" PRIbid ": "
//                      "link %s ok since transmission count=%zu",
//                      bundle->bundleid(), link->name(), count);
        }
    }

    // otherwise log the reason why we should send it
//    log_debug("should_fwd bundle %" PRIbid ": "
//              "match %s: forwarding log entry %s",
//              bundle->bundleid(), link->name(),
//              ForwardingInfo::state_to_str(info.state()));

    return true;
}

//----------------------------------------------------------------------
void
BundleRouter::initialize()
{
}

//----------------------------------------------------------------------
BundleRouter::~BundleRouter()
{
}

//----------------------------------------------------------------------
bool
BundleRouter::accept_bundle(Bundle* bundle, int* errp)
{
    // XXX/demmer this decision should be abstracted into a
    // StoragePolicy class of some sort. for now just use a
    // statically-configured payload limit
    BundleStore* bs = BundleStore::instance();
    if (!bundle->payload_space_reserved()) {
        if (bs->payload_quota() != 0 &&
            (bs->total_size() + bundle->payload().length() > bs->payload_quota()))
        {
            log_info("accept_bundle: rejecting bundle *%p since "
                     "cur size %llu + bundle size %zu > quota %llu",
                     bundle, U64FMT(bs->total_size()), bundle->payload().length(),
                     U64FMT(bs->payload_quota()));
            *errp = BundleProtocol::REASON_DEPLETED_STORAGE;
            return false;
        } 
    }

    *errp = 0;
    return true;
}

//----------------------------------------------------------------------
bool
BundleRouter::accept_custody(Bundle* bundle)
{
    (void)bundle;
    return true;
}

//----------------------------------------------------------------------
bool
BundleRouter::can_delete_bundle(const BundleRef& bundle)
{
    size_t num_mappings = bundle->num_mappings();
    if (num_mappings > 1) {
        log_debug("can_delete_bundle(*%p): not deleting because "
                  "bundle has %zu list mappings",
                  bundle.object(), num_mappings);
        return false;
    }
    
    return true;
}

//----------------------------------------------------------------------
void
BundleRouter::delete_bundle(const BundleRef& bundle)
{
    (void)bundle;
}

//----------------------------------------------------------------------
void
BundleRouter::tcl_dump_state(oasys::StringBuffer* buf)
{
    buf->append("{}");
}

//----------------------------------------------------------------------
void
BundleRouter::recompute_routes()
{
}

//----------------------------------------------------------------------
void
BundleRouter::shutdown()
{
}

} // namespace dtn
