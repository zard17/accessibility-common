#ifndef ACCESSIBILITY_API_ACCESSIBILITY_SERVICE_H
#define ACCESSIBILITY_API_ACCESSIBILITY_SERVICE_H

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
#include <accessibility/api/accessibility.h>
#include <accessibility/api/accessibility-event.h>
#include <accessibility/api/app-registry.h>
#include <accessibility/api/gesture-provider.h>
#include <accessibility/api/node-proxy.h>
#include <accessibility/api/types.h>

namespace Accessibility
{
/**
 * @brief Base class for assistive technology services.
 *
 * Provides common navigation, highlight, and event routing infrastructure.
 * Concrete services (ScreenReaderService, InspectorService, AurumService)
 * extend this class and implement the virtual callbacks.
 *
 * Usage pattern (Android-inspired):
 *   class ScreenReaderService : public AccessibilityService
 *   {
 *   protected:
 *     void onAccessibilityEvent(const AccessibilityEvent& event) override;
 *     void onWindowChanged(std::shared_ptr<NodeProxy> window) override;
 *     void onGesture(const GestureInfo& gesture) override;
 *   };
 */
class AccessibilityService
{
public:
  /**
   * @brief Constructor.
   *
   * @param[in] registry The app registry for discovering accessible applications
   * @param[in] gestureProvider The gesture provider for receiving platform gestures
   */
  AccessibilityService(std::unique_ptr<AppRegistry> registry,
                       std::unique_ptr<GestureProvider> gestureProvider);

  virtual ~AccessibilityService();

  // Non-copyable, non-movable
  AccessibilityService(const AccessibilityService&)            = delete;
  AccessibilityService& operator=(const AccessibilityService&) = delete;
  AccessibilityService(AccessibilityService&&)                 = delete;
  AccessibilityService& operator=(AccessibilityService&&)      = delete;

  /**
   * @brief Starts the service: subscribes to events, begins gesture listening.
   */
  void start();

  /**
   * @brief Stops the service: tears down all subscriptions.
   */
  void stop();

  /**
   * @brief Gets the currently active window.
   */
  std::shared_ptr<NodeProxy> getActiveWindow();

  /**
   * @brief Navigates to the next highlightable node.
   *
   * @return The newly focused node, or nullptr if navigation failed
   */
  std::shared_ptr<NodeProxy> navigateNext();

  /**
   * @brief Navigates to the previous highlightable node.
   *
   * @return The newly focused node, or nullptr if navigation failed
   */
  std::shared_ptr<NodeProxy> navigatePrev();

  /**
   * @brief Highlights the given node.
   *
   * @param[in] node The node to highlight
   * @return true on success
   */
  bool highlightNode(std::shared_ptr<NodeProxy> node);

  /**
   * @brief Gets the currently focused node.
   */
  std::shared_ptr<NodeProxy> getCurrentNode() const;

  /**
   * @brief Dispatches an accessibility event to the service.
   *
   * This is called by event routers (AtSpiEventRouter, TidlEventRouter)
   * when they receive events from applications.
   */
  void dispatchEvent(const AccessibilityEvent& event);

protected:
  /**
   * @brief Called when an accessibility event is received from an application.
   */
  virtual void onAccessibilityEvent(const AccessibilityEvent& event) = 0;

  /**
   * @brief Called when the active window changes.
   */
  virtual void onWindowChanged(std::shared_ptr<NodeProxy> window) = 0;

  /**
   * @brief Called when a gesture is received from the platform.
   */
  virtual void onGesture(const GestureInfo& gesture) = 0;

  /**
   * @brief Called when a key event is received.
   *
   * @return true if the key event was consumed
   */
  virtual bool onKeyEvent(const KeyEvent& key);

  /**
   * @brief Gets the app registry.
   */
  AppRegistry& getRegistry();

private:
  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_ACCESSIBILITY_SERVICE_H
