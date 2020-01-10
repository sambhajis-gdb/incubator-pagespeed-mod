 #include "client_worker_impl.h"
 #include "envoy_fetch.h"

namespace net_instaweb {

ClientWorkerImpl::ClientWorkerImpl(Envoy::Api::Api& api, Envoy::ThreadLocal::Instance& tls,Envoy::Stats::Store& store, 
                   const int worker_number,
                   const Envoy::MonotonicTime starting_time,envoy::api::v2::core::HttpUri http_uri,EnvoyFetch* fetcher)
    : WorkerImpl(api, tls, store), worker_number_(worker_number), starting_time_(starting_time),
     http_uri(http_uri),fetcher(fetcher){
       
     }

void ClientWorkerImpl::work(Envoy::Upstream::ClusterManager& cm) {
  dispatcher_->post([&,this](){
    std::unique_ptr<PagespeedDataFetcherCallback> cb_ptr_ = std::make_unique<PagespeedDataFetcherCallback>(fetcher);
    std::unique_ptr<PagespeedRemoteDataFetcher> pagespeed_remote_data_fetch_ptr = 
      std::make_unique<PagespeedRemoteDataFetcher>(cm, http_uri, *cb_ptr_);
  pagespeed_remote_data_fetch_ptr->fetch();
  dispatcher_->run(Envoy::Event::Dispatcher::RunType::Block);
  });
  dispatcher_->run(Envoy::Event::Dispatcher::RunType::Block);
}



} // namespace Nighthawk