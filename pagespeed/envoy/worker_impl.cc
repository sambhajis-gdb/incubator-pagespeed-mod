#include "worker_impl.h"

#include "envoy/runtime/runtime.h"
#include "envoy/thread_local/thread_local.h"

namespace net_instaweb {

WorkerImpl::WorkerImpl(Envoy::Api::Api& api, Envoy::ThreadLocal::Instance& tls,
                       Envoy::Stats::Store& store)
    : thread_factory_(api.threadFactory()), dispatcher_(api.allocateDispatcher()), tls_(tls),
      store_(store), time_source_(api.timeSource()) {
  tls.registerThread(*dispatcher_, false);
}

WorkerImpl::~WorkerImpl() { tls_.shutdownThread(); }

void WorkerImpl::start(Envoy::Upstream::ClusterManager& cluster_manager_, envoy::api::v2::core::HttpUri http_uri, EnvoyFetch* fetch) {
  ASSERT(!started_ && !completed_);
  started_ = true;
  thread_ = thread_factory_.createThread([&,http_uri, fetch, this]() {
    dispatcher_->run(Envoy::Event::Dispatcher::RunType::NonBlock);
    work(cluster_manager_,http_uri,fetch);
  });
}

void WorkerImpl::waitForCompletion() {
  ASSERT(started_ && !completed_);
  completed_ = true;
  thread_->join();
}

} // namespace net_instaweb