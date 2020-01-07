/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "envoy_cluster_manager.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>

#include "envoy/stats/store.h"

#include "external/envoy/include/envoy/event/dispatcher.h"
#include "external/envoy/source/common/api/api_impl.h"
#include "external/envoy/source/common/config/utility.h"
#include "external/envoy/source/common/event/real_time_system.h"
#include "external/envoy/source/common/init/manager_impl.h"
#include "external/envoy/source/common/local_info/local_info_impl.h"
#include "external/envoy/source/common/protobuf/message_validator_impl.h"
#include "external/envoy/source/common/runtime/runtime_impl.h"
#include "external/envoy/source/common/singleton/manager_impl.h"
#include "external/envoy/source/common/stats/allocator_impl.h"
#include "external/envoy/source/common/stats/thread_local_store.h"
#include "external/envoy/source/common/thread_local/thread_local_impl.h"
#include "external/envoy/source/common/upstream/cluster_manager_impl.h"
#include "external/envoy/source/exe/platform_impl.h"
#include "external/envoy/source/exe/process_wide.h"
#include "external/envoy/source/extensions/transport_sockets/tls/context_manager_impl.h"
#include "external/envoy/source/extensions/transport_sockets/well_known_names.h"
#include "external/envoy/source/server/config_validation/admin.h"
#include "external/envoy/source/server/options_impl_platform.h"

using namespace std::chrono_literals;
namespace net_instaweb {

EnvoyClusterManager::EnvoyClusterManager()
    : init_watcher_("envoyfetcher", []() {}), secret_manager_(config_tracker_),
      validation_context_(false, false), init_manager_("init_manager"),
      stats_allocator_(symbol_table_), store_root_(stats_allocator_),
      http_context_(store_root_.symbolTable()) {
  
}

EnvoyClusterManager::~EnvoyClusterManager() {
  tls_.shutdownGlobalThreading();
  store_root_.shutdownThreading();
  if (cluster_manager_ != nullptr) {
    cluster_manager_->shutdown();
  }
  tls_.shutdownThread();
}

void configureComponentLogLevels(spdlog::level::level_enum level) {
  Envoy::Logger::Registry::setLogLevel(level);
  Envoy::Logger::Logger* logger_to_change = Envoy::Logger::Registry::logger(logger_str);
  logger_to_change->setLevel(level);
}

void EnvoyClusterManager::initClusterManager() {

  configureComponentLogLevels(spdlog::level::from_str("error"));

  local_info_ = std::make_unique<Envoy::LocalInfo::LocalInfoImpl>(
      envoy_node_, Envoy::Network::Utility::getLocalAddress(Envoy::Network::Address::IpVersion::v4),
      "envoyfetcher_service_zone", "envoyfetcher_service_cluster", "envoyfetcher_service_node");

  api_ = std::make_unique<Envoy::Api::Impl>(platform_impl_.threadFactory(), store_root_,
                                            time_system_, platform_impl_.fileSystem());
  dispatcher_ = api_->allocateDispatcher();
  
  access_log_manager_ = new Envoy::AccessLog::AccessLogManagerImpl(
      std::chrono::milliseconds(1000), *api_, *dispatcher_, access_log_lock_, store_root_);
  
  singleton_manager_ = std::make_unique<Envoy::Singleton::ManagerImpl>(api_->threadFactory());

}

Envoy::Upstream::ClusterManager&
EnvoyClusterManager::getClusterManager(const GoogleString str_url_) {
tls_.registerThread(*dispatcher_, true);
  store_root_.initializeThreading(*dispatcher_, tls_);
  runtime_singleton_ = std::make_unique<Envoy::Runtime::ScopedLoaderSingleton>(
      Envoy::Runtime::LoaderPtr{new Envoy::Runtime::LoaderImpl(
          *dispatcher_, tls_, {}, *local_info_, init_manager_, store_root_, generator_,
          Envoy::ProtobufMessage::getStrictValidationVisitor(), *api_)});

 

  ssl_context_manager_ =
      std::make_unique<Envoy::Extensions::TransportSockets::Tls::ContextManagerImpl>(time_system_);

  cluster_manager_factory_ = std::make_unique<Envoy::Upstream::ProdClusterManagerFactory>(
      admin_, Envoy::Runtime::LoaderSingleton::get(), store_root_, tls_, generator_,
      dispatcher_->createDnsResolver({}), *ssl_context_manager_, *dispatcher_, *local_info_,
      secret_manager_, validation_context_, *api_, http_context_, *access_log_manager_,
      *singleton_manager_);
  const std::string host_name = "127.0.0.1";
  const std::string scheme = "http";
  auto port = 80;

  cluster_manager_ = cluster_manager_factory_->clusterManagerFromProto(
      createBootstrapConfiguration(scheme, host_name, port));
  cluster_manager_->setInitializedCb([this]() -> void { init_manager_.initialize(init_watcher_); });
  Envoy::Runtime::LoaderSingleton::get().initialize(*cluster_manager_);
  return *cluster_manager_;
}

const envoy::config::bootstrap::v2::Bootstrap
EnvoyClusterManager::createBootstrapConfiguration(const std::string scheme, const std::string host_name,
                                                  const int port) const {
  envoy::config::bootstrap::v2::Bootstrap bootstrap;
  auto* cluster = bootstrap.mutable_static_resources()->add_clusters();
  cluster->set_name(getClusterName());
  cluster->mutable_connect_timeout()->set_seconds(15);
  cluster->set_type(envoy::api::v2::Cluster::DiscoveryType::Cluster_DiscoveryType_STATIC);
  auto* host = cluster->add_hosts();
  auto* socket_address = host->mutable_socket_address();

  socket_address->set_address(host_name);
  socket_address->set_port_value(port);

  return bootstrap;
}

const std::vector<ClientWorkerPtr>& EnvoyClusterManager::createWorkers(const std::string str_url_, EnvoyFetch* fetcher){
  initClusterManager();
  envoy::api::v2::core::HttpUri http_uri;
  http_uri.set_uri(str_url_);
  http_uri.set_cluster(getClusterName());
  
  // TODO(oschaaf): Expose kMinimalDelay in configuration.
  const std::chrono::milliseconds kMinimalWorkerDelay = 500ms;
  ASSERT(workers_.empty());
  // We try to offset the start of each thread so that workers will execute tasks evenly spaced in
  // time. Let's assume we have two workers w0/w1, which should maintain a combined global pace of
  // 1000Hz. w0 and w1 both run at 500Hz, but ideally their execution is evenly spaced in time,
  // and not overlapping. Workers start offsets can be computed like
  // "worker_number*(1/global_frequency))", which would yield T0+[0ms, 1ms]. This helps reduce
  // batching/queueing effects, both initially, but also by calibrating the linear rate limiter we
  // currently have to a precise starting time, which helps later on.
  // TODO(oschaaf): Arguably, this ought to be the job of a rate limiter with awareness of the
  // global status quo, which we do not have right now. This has been noted in the
  // track-for-future issue.
  const auto first_worker_start = time_system_.monotonicTime() + kMinimalWorkerDelay;
  const double inter_worker_delay_usec =
      (1./5) * 1000000 / 1;
  int worker_number = 0;
  while (workers_.size() < 1) {
    const auto worker_delay = std::chrono::duration_cast<std::chrono::nanoseconds>(
        ((inter_worker_delay_usec * worker_number) * 1us));
    workers_.push_back(std::make_unique<ClientWorkerImpl>(
        *api_, tls_, store_root_,  getClusterManager(str_url_), worker_number,
        first_worker_start + worker_delay,http_uri,fetcher));
    worker_number++;
  }
  return workers_;
}

} // namespace net_instaweb