#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_STATUS_MONITOR_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_STATUS_MONITOR_H

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

// EXTERNAL INCLUDES
#include <functional>
#include <string>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/ipc/ipc-status-monitor.h>

namespace Ipc
{
/**
 * @brief TIDL implementation of AccessibilityStatusMonitor.
 *
 * Uses TIDL proxy to communicate with the accessibility status service
 * via rpc_port direct P2P connection instead of D-Bus.
 *
 * Scaffold: Returns stub values. Real implementation will use
 * tidlc-generated proxy code.
 */
class TidlStatusMonitor : public AccessibilityStatusMonitor
{
public:
  /**
   * @brief Constructs a TIDL status monitor.
   *
   * @param[in] appId Target application/service ID
   * @param[in] portName TIDL port name for the status service
   */
  TidlStatusMonitor(std::string appId, std::string portName)
  : mAppId(std::move(appId)),
    mPortName(std::move(portName)),
    mConnected(true) // Scaffold: assume connected
  {
  }

  ~TidlStatusMonitor() override = default;

  bool isConnected() const override
  {
    return mConnected;
  }

  void readIsEnabled(std::function<void(ValueOrError<bool>)> callback) override
  {
    // Scaffold: report enabled by default (no real TIDL proxy yet)
    // Real implementation: mProxy->GetIsEnabled() via generated proxy
    if(callback)
    {
      callback(ValueOrError<bool>(true));
    }
  }

  void listenIsEnabled(std::function<void(bool)> callback) override
  {
    // Scaffold: store callback for when TIDL delegate fires
    // Real implementation: mProxy->setOnIsEnabledChanged(callback)
    mIsEnabledCallback = std::move(callback);
  }

  void readScreenReaderEnabled(std::function<void(ValueOrError<bool>)> callback) override
  {
    // Scaffold: report disabled by default
    // Real implementation: mProxy->GetScreenReaderEnabled()
    if(callback)
    {
      callback(ValueOrError<bool>(false));
    }
  }

  void listenScreenReaderEnabled(std::function<void(bool)> callback) override
  {
    // Scaffold: store callback
    // Real implementation: mProxy->setOnScreenReaderEnabledChanged(callback)
    mScreenReaderEnabledCallback = std::move(callback);
  }

private:
  std::string mAppId;
  std::string mPortName;
  bool        mConnected;

  std::function<void(bool)> mIsEnabledCallback;
  std::function<void(bool)> mScreenReaderEnabledCallback;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_STATUS_MONITOR_H
