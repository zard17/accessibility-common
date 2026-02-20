#ifndef ACCESSIBILITY_TOOLS_SCREEN_READER_DIRECT_APP_REGISTRY_H
#define ACCESSIBILITY_TOOLS_SCREEN_READER_DIRECT_APP_REGISTRY_H

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
#include <unordered_map>

// INTERNAL INCLUDES
#include <accessibility/api/app-registry.h>
#include <tools/screen-reader/direct-node-proxy.h>

/**
 * @brief AppRegistry backed by a real DALi accessible tree root.
 *
 * Wraps an in-process Accessible* root (typically from the bridge or
 * a window's root layer) and returns DirectNodeProxy instances.
 * No IPC is needed -- all queries go through the C++ Accessible interface.
 */
class DirectAppRegistry : public Accessibility::AppRegistry
{
public:
  /**
   * @brief Constructs a DirectAppRegistry wrapping the given root accessible.
   *
   * @param[in] root The root accessible (e.g. from Bridge::GetApplication() or Window root layer)
   */
  explicit DirectAppRegistry(Accessibility::Accessible* root)
  : mRoot(root),
    mFactory([this](Accessibility::Accessible* acc) -> std::shared_ptr<DirectNodeProxy>
    {
      if(!acc) return nullptr;
      auto it = mProxyCache.find(acc);
      if(it != mProxyCache.end())
      {
        auto locked = it->second.lock();
        if(locked) return locked;
      }
      auto proxy = std::make_shared<DirectNodeProxy>(acc, mFactory);
      mProxyCache[acc] = proxy;
      return proxy;
    })
  {
  }

  std::shared_ptr<Accessibility::NodeProxy> getDesktop() override
  {
    return mFactory(mRoot);
  }

  std::shared_ptr<Accessibility::NodeProxy> getActiveWindow() override
  {
    if(!mRoot) return nullptr;

    // Find first WINDOW child, or return the root itself
    for(auto* child : mRoot->GetChildren())
    {
      if(child && child->GetRole() == Accessibility::Role::WINDOW)
      {
        return mFactory(child);
      }
    }
    return mFactory(mRoot);
  }

  void onAppRegistered(Accessibility::AppCallback /*callback*/) override
  {
    // In-process: no dynamic app registration
  }

  void onAppDeregistered(Accessibility::AppCallback /*callback*/) override
  {
    // In-process: no dynamic app deregistration
  }

private:
  Accessibility::Accessible*                                              mRoot;
  DirectNodeProxy::ProxyFactory                                           mFactory;
  std::unordered_map<Accessibility::Accessible*, std::weak_ptr<DirectNodeProxy>> mProxyCache;
};

#endif // ACCESSIBILITY_TOOLS_SCREEN_READER_DIRECT_APP_REGISTRY_H
