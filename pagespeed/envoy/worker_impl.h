#pragma once

#include "envoy/api/api.h"
#include "envoy/common/time.h"
#include "envoy/stats/store.h"

#include "external/envoy/source/common/common/logger.h"
#include "worker.h"

namespace net_instaweb {

class EnvoyFetch;

class WorkerImpl : virtual public Worker {
public:
  WorkerImpl(Envoy::Api::Api& api, Envoy::ThreadLocal::Instance& tls, Envoy::Stats::Store& store);
  ~WorkerImpl();

  void start(Envoy::Upstream::ClusterManager& cluster_manager_,envoy::api::v2::core::HttpUri http_uri,EnvoyFetch* fetch);
  void waitForCompletion();

protected:
  /**
   * Perform the actual work on the associated thread initiated by start().
   */
  virtual void work(Envoy::Upstream::ClusterManager& cluster_manager_,envoy::api::v2::core::HttpUri http_uri, EnvoyFetch* fetch) PURE;

  Envoy::Thread::ThreadFactory& thread_factory_;
  Envoy::Event::DispatcherPtr dispatcher_;
  Envoy::ThreadLocal::Instance& tls_;
  Envoy::Stats::Store& store_;
  Envoy::TimeSource& time_source_;

private:
  Envoy::Thread::ThreadPtr thread_;
  bool started_{};
  bool completed_{};
};

} // namespace net_instaweb