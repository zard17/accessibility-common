#ifndef ACCESSIBILITY_INTERNAL_BRIDGE_IPC_DIRECT_READING_CLIENT_H
#define ACCESSIBILITY_INTERNAL_BRIDGE_IPC_DIRECT_READING_CLIENT_H

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
#include <cstdint>
#include <functional>
#include <string>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/ipc/ipc-result.h>

namespace Ipc
{
/**
 * @brief Abstract interface for direct reading (TTS) commands.
 *
 * Replaces direct DBus::DBusClient usage for ReadCommand, PauseResume,
 * StopReading, and ReadingStateChanged on the screen reader's direct
 * reading service.
 */
class DirectReadingClient
{
public:
  virtual ~DirectReadingClient() = default;

  /**
   * @brief Sends a text reading command.
   *
   * @param[in] text Text to read
   * @param[in] discardable Whether the reading can be discarded
   * @param[in] callback Called with (text, discardable, readingId) or error
   */
  virtual void readCommand(const std::string&                                           text,
                           bool                                                         discardable,
                           std::function<void(ValueOrError<std::string, bool, int32_t>)> callback) = 0;

  /**
   * @brief Pauses or resumes reading.
   *
   * @param[in] pause True to pause, false to resume
   * @param[in] callback Called with success or error
   */
  virtual void pauseResume(bool                                    pause,
                           std::function<void(ValueOrError<void>)> callback) = 0;

  /**
   * @brief Stops reading.
   *
   * @param[in] alsoNonDiscardable If true, also stops non-discardable readings
   * @param[in] callback Called with success or error
   */
  virtual void stopReading(bool                                    alsoNonDiscardable,
                           std::function<void(ValueOrError<void>)> callback) = 0;

  /**
   * @brief Listens for reading state changes.
   *
   * @param[in] callback Called with (readingId, readingState) on each state change
   */
  virtual void listenReadingStateChanged(std::function<void(int32_t, std::string)> callback) = 0;
};

} // namespace Ipc

#endif // ACCESSIBILITY_INTERNAL_BRIDGE_IPC_DIRECT_READING_CLIENT_H
