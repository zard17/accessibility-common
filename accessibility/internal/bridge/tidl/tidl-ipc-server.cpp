/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// CLASS HEADER
#include <accessibility/internal/bridge/tidl/tidl-ipc-server.h>

// EXTERNAL INCLUDES
#include <utility>

// INTERNAL INCLUDES
#include <accessibility/api/log.h>

namespace Ipc
{
TidlIpcServer::TidlIpcServer(std::string appId)
: mAppId(std::move(appId))
{
  ACCESSIBILITY_LOG_DEBUG_INFO("TidlIpcServer created for app: %s\n", mAppId.c_str());
}

TidlIpcServer::~TidlIpcServer()
{
  ACCESSIBILITY_LOG_DEBUG_INFO("TidlIpcServer destroyed\n");
}

void TidlIpcServer::addInterface(const std::string&     pathName,
                                 InterfaceDescription&  desc,
                                 bool                   fallback)
{
  auto& tidlDesc = static_cast<Tidl::TidlInterfaceDescription&>(desc);

  ACCESSIBILITY_LOG_DEBUG_INFO("TidlIpcServer::addInterface path=%s interface=%s fallback=%d\n",
                               pathName.c_str(), tidlDesc.getInterfaceName().c_str(), fallback);

  // Clone the description into our storage
  auto stored = std::make_unique<Tidl::TidlInterfaceDescription>(tidlDesc.getInterfaceName());

  if(fallback)
  {
    mFallbackInterfaces.push_back(std::move(stored));
  }
  else
  {
    InterfaceKey key{pathName, tidlDesc.getInterfaceName()};
    mInterfaces[key] = std::move(stored);
  }

  // In the real TIDL implementation, this would register the interface
  // with the TIDL stub so incoming calls get dispatched to the stored
  // method handlers. For now (scaffold), we just store them.
}

std::string TidlIpcServer::getBusName() const
{
  // TIDL uses app ID as the service identifier instead of D-Bus bus names
  return mAppId;
}

std::string TidlIpcServer::getCurrentObjectPath() const
{
  // During TIDL dispatch, this returns the objectPath parameter
  // that was passed by the client in the current method call.
  return mCurrentObjectPath;
}

void TidlIpcServer::emitSignal(const std::string&             objectPath,
                               const std::string&             interfaceName,
                               const std::string&             signalName,
                               const std::string&             detail,
                               int                            detail1,
                               int                            detail2,
                               const SignalVariant&            data,
                               const Accessibility::Address&  sender)
{
  // In the real TIDL implementation, this would invoke the registered
  // delegate callback to notify connected clients of the event.
  // For now (scaffold), log and discard.
  ACCESSIBILITY_LOG_DEBUG_INFO("TidlIpcServer::emitSignal path=%s interface=%s signal=%s\n",
                               objectPath.c_str(), interfaceName.c_str(), signalName.c_str());
}

std::unique_ptr<InterfaceDescription> TidlIpcServer::createInterfaceDescription(const std::string& interfaceName)
{
  return std::make_unique<Tidl::TidlInterfaceDescription>(interfaceName);
}

} // namespace Ipc
