#ifndef ACCESSIBILITY_API_DIRECT_READING_SERVICE_H
#define ACCESSIBILITY_API_DIRECT_READING_SERVICE_H

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

namespace Accessibility
{
class TtsEngine;

/**
 * @brief Abstract interface for direct reading (app-initiated TTS).
 *
 * Exposes org.tizen.DirectReading D-Bus interface for external reading requests.
 */
class DirectReadingService
{
public:
  virtual ~DirectReadingService() = default;

  /**
   * @brief Starts the direct reading service, registering on D-Bus.
   *
   * @param[in] tts The TTS engine to use for reading
   */
  virtual void start(TtsEngine& tts) = 0;

  /**
   * @brief Stops the direct reading service.
   */
  virtual void stop() = 0;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_DIRECT_READING_SERVICE_H
