#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_DIRECT_READING_CLIENT_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_DIRECT_READING_CLIENT_H

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
#include <accessibility/internal/bridge/ipc/ipc-direct-reading-client.h>

namespace Ipc
{
/**
 * @brief TIDL implementation of DirectReadingClient.
 *
 * Uses TIDL proxy to communicate with the screen reader's direct
 * reading (TTS) service via rpc_port direct P2P connection.
 *
 * Scaffold: Returns errors for all operations.
 */
class TidlDirectReadingClient : public DirectReadingClient
{
public:
  TidlDirectReadingClient() = default;

  ~TidlDirectReadingClient() override = default;

  void readCommand(const std::string&                                           text,
                   bool                                                         discardable,
                   std::function<void(ValueOrError<std::string, bool, int32_t>)> callback) override
  {
    // Scaffold: return error (no real TIDL proxy yet)
    // Real implementation: mProxy->ReadCommand(text, discardable)
    if(callback)
    {
      callback(Error{"TIDL direct reading not yet implemented"});
    }
  }

  void pauseResume(bool                                    pause,
                   std::function<void(ValueOrError<void>)> callback) override
  {
    // Scaffold: return error
    // Real implementation: mProxy->PauseResume(pause)
    if(callback)
    {
      callback(Error{"TIDL direct reading not yet implemented"});
    }
  }

  void stopReading(bool                                    alsoNonDiscardable,
                   std::function<void(ValueOrError<void>)> callback) override
  {
    // Scaffold: return error
    // Real implementation: mProxy->StopReading(alsoNonDiscardable)
    if(callback)
    {
      callback(Error{"TIDL direct reading not yet implemented"});
    }
  }

  void listenReadingStateChanged(std::function<void(int32_t, std::string)> callback) override
  {
    // Scaffold: store callback
    // Real implementation: mProxy->setOnReadingStateChanged(callback)
    mReadingStateCallback = std::move(callback);
  }

private:
  std::function<void(int32_t, std::string)> mReadingStateCallback;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_TIDL_DIRECT_READING_CLIENT_H
