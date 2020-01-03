#pragma once

#include <memory>

#include "envoy/common/pure.h"
#include "worker.h"


namespace net_instaweb {


/**
 * Interface for a threaded benchmark client worker.
 */
class ClientWorker : virtual public Worker {
public:

  /**
   * @return bool True iff the worker ran and completed successfully.
   */
  virtual bool success() const PURE;
};

using ClientWorkerPtr = std::unique_ptr<ClientWorker>;

} // namespace net_instaweb
