#ifndef ACCESSIBILITY_INTERNAL_SERVICE_INSPECTOR_SERVICE_H
#define ACCESSIBILITY_INTERNAL_SERVICE_INSPECTOR_SERVICE_H

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
#include <tools/inspector/node-proxy-query-engine.h>
#include <tools/inspector/web-inspector-server.h>

namespace Accessibility
{
/**
 * @brief Inspector service that extends AccessibilityService.
 *
 * Provides a web-based inspector UI via HTTP. Uses NodeProxyQueryEngine
 * to build tree snapshots from the base class's NodeProxy tree.
 *
 * start()/stop() are non-virtual in the base class, so this service
 * uses startInspector()/stopInspector() which wrap base start/stop
 * plus HTTP server lifecycle.
 */
class InspectorService : public AccessibilityService
{
public:
  struct Config
  {
    int port = 8080;
  };

  /**
   * @brief Constructor.
   *
   * @param[in] registry The app registry
   * @param[in] gestureProvider The gesture provider
   * @param[in] config Configuration options
   */
  InspectorService(std::unique_ptr<AppRegistry> registry,
                   std::unique_ptr<GestureProvider> gestureProvider,
                   Config config);

  ~InspectorService() override;

  /**
   * @brief Starts the service and the HTTP inspector server.
   */
  void startInspector();

  /**
   * @brief Stops the HTTP server and the service.
   */
  void stopInspector();

  /**
   * @brief Refreshes the NodeProxy tree snapshot.
   */
  void refreshSnapshot();

  /**
   * @brief Gets the query engine (for testing).
   */
  InspectorEngine::NodeProxyQueryEngine& getQueryEngine();

  /**
   * @brief Checks if the inspector HTTP server is running.
   */
  bool isInspectorRunning() const;

  /**
   * @brief Gets the HTTP port.
   */
  int getPort() const;

protected:
  void onAccessibilityEvent(const AccessibilityEvent& event) override;
  void onWindowChanged(std::shared_ptr<NodeProxy> window) override;
  void onGesture(const GestureInfo& gesture) override;

private:
  InspectorEngine::NodeProxyQueryEngine mQueryEngine;
  InspectorEngine::WebInspectorServer   mServer;
  Config                                mConfig;
  bool                                  mInspectorRunning{false};
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_INSPECTOR_SERVICE_H
