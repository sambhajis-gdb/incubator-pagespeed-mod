#pragma once

#include <memory>

#include "envoy/common/pure.h"
#include "external/envoy/source/common/upstream/cluster_manager_impl.h"
#include "external/envoy_api/envoy/api/v2/core/http_uri.pb.h"
// #include "envoy_fetch.h"


namespace net_instaweb {

class EnvoyFetch;
/**
 * Interface for a threaded worker.
 */
class Worker {
public:
  virtual ~Worker() = default;

  /**
   * Start the worker thread.
   */
  virtual void start(Envoy::Upstream::ClusterManager& cm,envoy::api::v2::core::HttpUri http_uri,EnvoyFetch* fetch) PURE;

  /**
   * Wait for the worker thread to complete its work.
   */
  virtual void waitForCompletion() PURE;
};

using WorkerPtr = std::unique_ptr<Worker>;

} // namespace net_instaweb
