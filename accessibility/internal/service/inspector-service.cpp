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
#include <accessibility/internal/service/inspector-service.h>

namespace Accessibility
{
InspectorService::InspectorService(std::unique_ptr<AppRegistry> registry,
                                   std::unique_ptr<GestureProvider> gestureProvider,
                                   Config config)
: AccessibilityService(std::move(registry), std::move(gestureProvider)),
  mConfig(config)
{
}

InspectorService::~InspectorService()
{
  if(mInspectorRunning)
  {
    stopInspector();
  }
}

void InspectorService::startInspector()
{
  if(mInspectorRunning)
  {
    return;
  }

  // Start base service
  start();

  // Build initial snapshot from current window
  refreshSnapshot();

  // Start HTTP server (skip when port is 0 — test mode)
  if(mConfig.port > 0)
  {
    mServer.Start(mQueryEngine, mConfig.port);
  }
  mInspectorRunning = true;
}

void InspectorService::stopInspector()
{
  if(!mInspectorRunning)
  {
    return;
  }

  if(mServer.IsRunning())
  {
    mServer.Stop();
  }
  mInspectorRunning = false;

  // Stop base service
  stop();
}

void InspectorService::refreshSnapshot()
{
  auto window = getActiveWindow();
  if(window)
  {
    mQueryEngine.BuildSnapshot(window);
  }
}

InspectorEngine::NodeProxyQueryEngine& InspectorService::getQueryEngine()
{
  return mQueryEngine;
}

bool InspectorService::isInspectorRunning() const
{
  return mInspectorRunning;
}

int InspectorService::getPort() const
{
  return mConfig.port;
}

void InspectorService::onAccessibilityEvent(const AccessibilityEvent& /*event*/)
{
  // Inspector is passive — no action on events
}

void InspectorService::onWindowChanged(std::shared_ptr<NodeProxy> /*window*/)
{
  // Auto-refresh snapshot when window changes
  if(mInspectorRunning)
  {
    refreshSnapshot();
  }
}

void InspectorService::onGesture(const GestureInfo& /*gesture*/)
{
  // Inspector does not handle gestures
}

} // namespace Accessibility
