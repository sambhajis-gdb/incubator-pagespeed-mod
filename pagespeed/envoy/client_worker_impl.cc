 #include "client_worker_impl.h"
 #include "envoy_fetch.h"

namespace net_instaweb {

ClientWorkerImpl::ClientWorkerImpl(Envoy::Api::Api& api, Envoy::ThreadLocal::Instance& tls,Envoy::Stats::Store& store, 
                   Envoy::Upstream::ClusterManager& cluster_manager_,
                   const int worker_number,
                   const Envoy::MonotonicTime starting_time,envoy::api::v2::core::HttpUri http_uri,EnvoyFetch* fetcher)
    : WorkerImpl(api, tls, store), worker_number_(worker_number), starting_time_(starting_time),
     cluster_manager_(cluster_manager_),http_uri(http_uri),fetcher(fetcher){}

void ClientWorkerImpl::fetchData() {
    std::unique_ptr<PagespeedDataFetcherCallback> cb_ptr_ = std::make_unique<PagespeedDataFetcherCallback>(fetcher);
  std::unique_ptr<PagespeedRemoteDataFetcher> pagespeed_remote_data_fetch_ptr = 
      std::make_unique<PagespeedRemoteDataFetcher>(cluster_manager_, http_uri, *cb_ptr_);
  pagespeed_remote_data_fetch_ptr->fetch();
  dispatcher_->run(Envoy::Event::Dispatcher::RunType::Block);
}

void ClientWorkerImpl::work() {
  std::function<void()> fetch_fun_ptr = std::bind(&ClientWorkerImpl::fetchData, this);
  dispatcher_->post(fetch_fun_ptr);
  dispatcher_->run(Envoy::Event::Dispatcher::RunType::NonBlock);
  dispatcher_->exit();
}



} // namespace Nighthawk