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
#include <accessibility/internal/service/atspi-app-registry.h>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/dbus/dbus-locators.h>

namespace Accessibility
{
AtSpiAppRegistry::AtSpiAppRegistry(DBusWrapper::ConnectionPtr connection)
: mConnection(std::move(connection))
{
}

std::shared_ptr<NodeProxy> AtSpiAppRegistry::createNodeProxy(const Address& address)
{
  NodeProxyFactory factory = [this](const Address& addr) -> std::shared_ptr<NodeProxy>
  {
    return createNodeProxy(addr);
  };

  return std::make_shared<AtSpiNodeProxy>(address, mConnection, std::move(factory));
}

std::shared_ptr<NodeProxy> AtSpiAppRegistry::getDesktop()
{
  if(!mDesktop)
  {
    // The AT-SPI registry desktop is at org.a11y.atspi.Registry:/org/a11y/atspi/accessible/root
    Address desktopAddr{"org.a11y.atspi.Registry", "/org/a11y/atspi/accessible/root"};
    mDesktop = createNodeProxy(desktopAddr);
  }
  return mDesktop;
}

std::shared_ptr<NodeProxy> AtSpiAppRegistry::getActiveWindow()
{
  // Query the desktop's children to find the active window
  auto desktop = getDesktop();
  if(!desktop)
  {
    return nullptr;
  }

  auto children = desktop->getChildren();
  for(auto& child : children)
  {
    auto states = child->getStates();
    if(states[State::ACTIVE])
    {
      return child;
    }
  }

  // Fallback: return first child if no active window found
  if(!children.empty())
  {
    return children[0];
  }
  return nullptr;
}

void AtSpiAppRegistry::onAppRegistered(AppCallback callback)
{
  mRegisteredCallbacks.push_back(std::move(callback));
}

void AtSpiAppRegistry::onAppDeregistered(AppCallback callback)
{
  mDeregisteredCallbacks.push_back(std::move(callback));
}

} // namespace Accessibility
