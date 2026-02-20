#ifndef ACCESSIBILITY_API_SCREEN_READER_SERVICE_H
#define ACCESSIBILITY_API_SCREEN_READER_SERVICE_H

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
#include <memory>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility-service.h>
#include <accessibility/api/direct-reading-service.h>
#include <accessibility/api/feedback-provider.h>
#include <accessibility/api/reading-composer.h>
#include <accessibility/api/screen-reader-switch.h>
#include <accessibility/api/settings-provider.h>
#include <accessibility/api/tts-engine.h>

namespace Accessibility
{
/**
 * @brief Full-featured screen reader service for mobile/wearable profiles.
 *
 * Extends AccessibilityService with TTS, auditory/haptic feedback,
 * reading composition, settings management, and direct reading support.
 *
 * Usage:
 * @code
 *   auto service = std::make_unique<ScreenReaderService>(
 *     std::move(registry),
 *     std::move(gestureProvider),
 *     std::move(ttsEngine),
 *     std::move(feedbackProvider),
 *     std::move(settingsProvider),
 *     std::move(screenReaderSwitch),
 *     std::move(directReadingService));
 *   service->startScreenReader();
 * @endcode
 */
class ScreenReaderService : public AccessibilityService
{
public:
  /**
   * @brief Constructor.
   *
   * @param[in] registry The app registry for discovering accessible applications
   * @param[in] gestureProvider The gesture provider for receiving platform gestures
   * @param[in] ttsEngine The text-to-speech engine
   * @param[in] feedbackProvider The auditory/haptic feedback provider
   * @param[in] settingsProvider The settings provider
   * @param[in] screenReaderSwitch The screen reader on/off switch
   * @param[in] directReadingService The direct reading service
   */
  ScreenReaderService(std::unique_ptr<AppRegistry>          registry,
                      std::unique_ptr<GestureProvider>      gestureProvider,
                      std::unique_ptr<TtsEngine>            ttsEngine,
                      std::unique_ptr<FeedbackProvider>     feedbackProvider,
                      std::unique_ptr<SettingsProvider>     settingsProvider,
                      std::unique_ptr<ScreenReaderSwitch>   screenReaderSwitch,
                      std::unique_ptr<DirectReadingService> directReadingService);

  ~ScreenReaderService() override;

  /**
   * @brief Starts the screen reader, enabling TTS and event processing.
   */
  void startScreenReader();

  /**
   * @brief Stops the screen reader, disabling TTS and event processing.
   */
  void stopScreenReader();

  /**
   * @brief Reads the given node aloud via TTS.
   *
   * @param[in] node The node to read
   */
  void readNode(std::shared_ptr<NodeProxy> node);

  /**
   * @brief Gets the TTS engine.
   *
   * @return A reference to the TTS engine
   */
  TtsEngine& getTtsEngine();

  /**
   * @brief Gets the feedback provider.
   *
   * @return A reference to the feedback provider
   */
  FeedbackProvider& getFeedbackProvider();

  /**
   * @brief Gets the settings provider.
   *
   * @return A reference to the settings provider
   */
  SettingsProvider& getSettingsProvider();

  /**
   * @brief Checks whether the screen reader is currently running.
   *
   * @return true if the screen reader is running
   */
  bool isScreenReaderRunning() const;

protected:
  /**
   * @brief Called when an accessibility event is received from an application.
   */
  void onAccessibilityEvent(const AccessibilityEvent& event) override;

  /**
   * @brief Called when the active window changes.
   */
  void onWindowChanged(std::shared_ptr<NodeProxy> window) override;

  /**
   * @brief Called when a gesture is received from the platform.
   */
  void onGesture(const GestureInfo& gesture) override;

  /**
   * @brief Called when a key event is received.
   *
   * @return true if the key event was consumed
   */
  bool onKeyEvent(const KeyEvent& key) override;

private:
  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

/**
 * @brief Lightweight screen reader service for the TV profile.
 *
 * TV screen readers typically do not use touch gestures or haptic feedback.
 * This service provides TTS and settings support without FeedbackProvider,
 * ScreenReaderSwitch, or DirectReadingService.
 */
class TvScreenReaderService : public AccessibilityService
{
public:
  /**
   * @brief Constructor.
   *
   * @param[in] registry The app registry for discovering accessible applications
   * @param[in] gestureProvider The gesture provider for receiving platform gestures
   * @param[in] ttsEngine The text-to-speech engine
   * @param[in] settingsProvider The settings provider
   */
  TvScreenReaderService(std::unique_ptr<AppRegistry>     registry,
                        std::unique_ptr<GestureProvider>  gestureProvider,
                        std::unique_ptr<TtsEngine>        ttsEngine,
                        std::unique_ptr<SettingsProvider>  settingsProvider);

  ~TvScreenReaderService() override;

  /**
   * @brief Starts the screen reader, enabling TTS and event processing.
   */
  void startScreenReader();

  /**
   * @brief Stops the screen reader, disabling TTS and event processing.
   */
  void stopScreenReader();

  /**
   * @brief Reads the given node aloud via TTS.
   *
   * @param[in] node The node to read
   */
  void readNode(std::shared_ptr<NodeProxy> node);

  /**
   * @brief Gets the TTS engine.
   *
   * @return A reference to the TTS engine
   */
  TtsEngine& getTtsEngine();

  /**
   * @brief Checks whether the screen reader is currently running.
   *
   * @return true if the screen reader is running
   */
  bool isScreenReaderRunning() const;

protected:
  /**
   * @brief Called when an accessibility event is received from an application.
   */
  void onAccessibilityEvent(const AccessibilityEvent& event) override;

  /**
   * @brief Called when the active window changes.
   */
  void onWindowChanged(std::shared_ptr<NodeProxy> window) override;

  /**
   * @brief Called when a gesture is received from the platform.
   */
  void onGesture(const GestureInfo& gesture) override;

private:
  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_SCREEN_READER_SERVICE_H
