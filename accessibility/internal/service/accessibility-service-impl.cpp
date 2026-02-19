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
#include <accessibility/api/accessibility-service.h>

namespace Accessibility
{
struct AccessibilityService::Impl
{
  std::unique_ptr<AppRegistry>     registry;
  std::unique_ptr<GestureProvider> gestureProvider;
  std::shared_ptr<NodeProxy>       currentNode;
  std::shared_ptr<NodeProxy>       currentWindow;
  bool                             running{false};
};

AccessibilityService::AccessibilityService(std::unique_ptr<AppRegistry> registry,
                                           std::unique_ptr<GestureProvider> gestureProvider)
: mImpl(std::make_unique<Impl>())
{
  mImpl->registry        = std::move(registry);
  mImpl->gestureProvider = std::move(gestureProvider);
}

AccessibilityService::~AccessibilityService()
{
  if(mImpl && mImpl->running)
  {
    stop();
  }
}

void AccessibilityService::start()
{
  if(mImpl->running)
  {
    return;
  }

  mImpl->running = true;

  // Subscribe to gestures
  if(mImpl->gestureProvider)
  {
    mImpl->gestureProvider->onGestureReceived([this](const GestureInfo& gesture)
    {
      onGesture(gesture);
    });
  }

  // Get the initial active window
  if(mImpl->registry)
  {
    mImpl->currentWindow = mImpl->registry->getActiveWindow();
  }
}

void AccessibilityService::stop()
{
  mImpl->running       = false;
  mImpl->currentNode   = nullptr;
  mImpl->currentWindow = nullptr;
}

std::shared_ptr<NodeProxy> AccessibilityService::getActiveWindow()
{
  if(mImpl->registry)
  {
    mImpl->currentWindow = mImpl->registry->getActiveWindow();
  }
  return mImpl->currentWindow;
}

std::shared_ptr<NodeProxy> AccessibilityService::navigateNext()
{
  auto window = getActiveWindow();
  if(!window)
  {
    return nullptr;
  }

  std::shared_ptr<NodeProxy> startNode = mImpl->currentNode ? mImpl->currentNode : window;
  auto next = startNode->getNeighbor(window, true, NeighborSearchMode::RECURSE_FROM_ROOT);
  if(next)
  {
    mImpl->currentNode = next;
    next->grabHighlight();
  }
  return next;
}

std::shared_ptr<NodeProxy> AccessibilityService::navigatePrev()
{
  auto window = getActiveWindow();
  if(!window)
  {
    return nullptr;
  }

  std::shared_ptr<NodeProxy> startNode = mImpl->currentNode ? mImpl->currentNode : window;
  auto prev = startNode->getNeighbor(window, false, NeighborSearchMode::RECURSE_FROM_ROOT);
  if(prev)
  {
    mImpl->currentNode = prev;
    prev->grabHighlight();
  }
  return prev;
}

bool AccessibilityService::highlightNode(std::shared_ptr<NodeProxy> node)
{
  if(!node)
  {
    return false;
  }

  bool result = node->grabHighlight();
  if(result)
  {
    mImpl->currentNode = node;
  }
  return result;
}

std::shared_ptr<NodeProxy> AccessibilityService::getCurrentNode() const
{
  return mImpl->currentNode;
}

void AccessibilityService::dispatchEvent(const AccessibilityEvent& event)
{
  if(!mImpl->running)
  {
    return;
  }

  // Route window change events
  if(event.type == AccessibilityEvent::Type::WINDOW_CHANGED)
  {
    auto window = getActiveWindow();
    if(window)
    {
      mImpl->currentWindow = window;
      onWindowChanged(window);
    }
  }

  onAccessibilityEvent(event);
}

bool AccessibilityService::onKeyEvent(const KeyEvent& /*key*/)
{
  return false;
}

AppRegistry& AccessibilityService::getRegistry()
{
  return *mImpl->registry;
}

} // namespace Accessibility
