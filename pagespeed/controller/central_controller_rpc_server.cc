// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: cheesy@google.com (Steve Hill)

#include "pagespeed/controller/central_controller_rpc_server.h"

#include <memory>

#include "base/logging.h"
#include "pagespeed/controller/controller.grpc.pb.h"
#include "pagespeed/kernel/base/function.h"
#include "pagespeed/kernel/base/string.h"
#include "pagespeed/kernel/base/string_util.h"

namespace net_instaweb {

CentralControllerRpcServer::CentralControllerRpcServer(int listen_port)
    : listen_port_(listen_port) {}

int CentralControllerRpcServer::Setup() {
  ::grpc::ServerBuilder builder;

  GoogleString listen_address(
      StrCat("localhost:", IntegerToString(listen_port_)));
  // InsecureServerCredentials means unencrytped, unauthenticated. In future
  // we may wish to look into different Credentials which would allow us to
  // encrypt and/or authenticate.
  builder.AddListeningPort(listen_address, ::grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  queue_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();
  if (server_ == nullptr) {
    LOG(ERROR) << "CentralControllerRpcServer failed to start";
    return 1;
  }

  // TODO(cheesy): Subsequent changes will actually attach handlers to the
  // service.

  return 0;
}

int CentralControllerRpcServer::Run() {
  LOG(INFO) << "CentralControllerRpcServer waiting for events";

  MainLoop(queue_.get());

  LOG(INFO) << "CentralControllerRpcServer terminated.";
  return 0;
}

void CentralControllerRpcServer::MainLoop(::grpc::CompletionQueue* queue) {
  void* tag;
  bool succeeded;
  while (queue->Next(&tag, &succeeded)) {
    Function* function = static_cast<Function*>(tag);
    if (succeeded) {
      function->CallRun();
    } else {
      function->CallCancel();
    }
  }
}

void CentralControllerRpcServer::Stop() {
  LOG(INFO) << "Shutting down CentralControllerRpcServer.";
  // This blocks indefinitely for all outstanding RPCs to complete.
  server_->Shutdown();
  // This should terminate the event loop immediately.
  queue_->Shutdown();
}

}  // namespace net_instaweb
