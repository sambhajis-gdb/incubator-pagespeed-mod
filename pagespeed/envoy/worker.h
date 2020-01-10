#pragma once

#include <memory>

#include "envoy/common/pure.h"
#include "external/envoy/source/common/upstream/cluster_manager_impl.h"


namespace net_instaweb {

/**
 * Interface for a threaded worker.
 */
class Worker {
public:
  virtual ~Worker() = default;

  /**
   * Start the worker thread.
   */
  virtual void start(Envoy::Upstream::ClusterManager& cm) PURE;

  /**
   * Wait for the worker thread to complete its work.
   */
  virtual void waitForCompletion() PURE;
};

using WorkerPtr = std::unique_ptr<Worker>;

} // namespace net_instaweb
