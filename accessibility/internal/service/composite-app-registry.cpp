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
#include <accessibility/internal/service/composite-app-registry.h>

namespace Accessibility
{
CompositeAppRegistry::CompositeAppRegistry(std::unique_ptr<AppRegistry> atspiRegistry,
                                           std::unique_ptr<AppRegistry> tidlRegistry)
: mAtspiRegistry(std::move(atspiRegistry)),
  mTidlRegistry(std::move(tidlRegistry))
{
}

CompositeAppRegistry::~CompositeAppRegistry() = default;

std::shared_ptr<NodeProxy> CompositeAppRegistry::getDesktop()
{
  // Prefer the AT-SPI registry desktop as the primary source.
  // In a full implementation, this would create a virtual desktop node
  // that merges children from both registries.
  if(mAtspiRegistry)
  {
    auto desktop = mAtspiRegistry->getDesktop();
    if(desktop) return desktop;
  }
  if(mTidlRegistry)
  {
    return mTidlRegistry->getDesktop();
  }
  return nullptr;
}

std::shared_ptr<NodeProxy> CompositeAppRegistry::getActiveWindow()
{
  // Check both registries for the active window.
  // The one that returns a non-null result wins.
  if(mAtspiRegistry)
  {
    auto window = mAtspiRegistry->getActiveWindow();
    if(window) return window;
  }
  if(mTidlRegistry)
  {
    return mTidlRegistry->getActiveWindow();
  }
  return nullptr;
}

void CompositeAppRegistry::onAppRegistered(AppCallback callback)
{
  // Register the callback on both registries
  if(mAtspiRegistry)
  {
    mAtspiRegistry->onAppRegistered(callback);
  }
  if(mTidlRegistry)
  {
    mTidlRegistry->onAppRegistered(callback);
  }
}

void CompositeAppRegistry::onAppDeregistered(AppCallback callback)
{
  if(mAtspiRegistry)
  {
    mAtspiRegistry->onAppDeregistered(callback);
  }
  if(mTidlRegistry)
  {
    mTidlRegistry->onAppDeregistered(callback);
  }
}

} // namespace Accessibility
