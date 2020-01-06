#pragma once

// #include "envoy/api/api.h"
// #include "envoy/event/dispatcher.h"
// #include "envoy/stats/store.h"
// #include "envoy/thread_local/thread_local.h"
// #include "envoy/upstream/cluster_manager.h"

// #include "nighthawk/client/benchmark_client.h"

// #include "nighthawk/client/factories.h"
// #include "nighthawk/common/sequencer.h"
// #include "nighthawk/common/uri.h"

#include "external/envoy/source/common/common/logger.h"
#include "external/envoy/source/common/http/context_impl.h"
#include "worker_impl.h"
#include "client_worker.h"
#include "pagespeed_remote_data_fetcher.h"
#include "envoy/api/api.h"
#include "envoy/event/dispatcher.h"
#include "envoy/stats/store.h"
#include "envoy/thread_local/thread_local.h"


namespace net_instaweb {



class ClientWorkerImpl : public WorkerImpl,
                         virtual public ClientWorker,
                         Envoy::Logger::Loggable<Envoy::Logger::Id::main> {
public:
  ClientWorkerImpl(Envoy::Api::Api& api, Envoy::ThreadLocal::Instance& tls,Envoy::Stats::Store& store, 
                   Envoy::Upstream::ClusterManager& cluster_manager_,
                   Envoy::Event::DispatcherPtr& dispatcher, const int worker_number,
                   const Envoy::MonotonicTime starting_time, envoy::api::v2::core::HttpUri http_uri, EnvoyFetch* fetcher);

  bool success() const override { return success_; }
  void work();
  void fetchData();
  

private:
  
  const int worker_number_;
  const Envoy::MonotonicTime starting_time_;
  Envoy::Upstream::ClusterManager& cluster_manager_;
  Envoy::Event::DispatcherPtr& dispatcher_;
  envoy::api::v2::core::HttpUri http_uri;
  EnvoyFetch* fetcher;
  bool success_{};
};

using ClientWorkerImplPtr = std::unique_ptr<ClientWorkerImpl>;

} // namespace net_instaweb