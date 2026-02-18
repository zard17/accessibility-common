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
#include <accessibility/internal/bridge/bridge-base.h>

// EXTERNAL INCLUDES
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <limits>
#include <memory>

// INTERNAL INCLUDES
#include <accessibility/api/accessible.h>
#include <accessibility/api/log.h>
#include <accessibility/api/types.h>
#include <accessibility/internal/bridge/bridge-platform.h>
#include <accessibility/internal/bridge/collection-impl.h>
#include <accessibility/internal/bridge/dbus/dbus-ipc-server.h>
#include <accessibility/internal/bridge/dbus/dbus-ipc-client.h>

using namespace Accessibility;

namespace
{
static RepeatingTimer gTickTimer;
}

namespace Accessibility
{

// ApplicationAccessible implementation
std::string ApplicationAccessible::GetName() const
{
  return mName;
}

std::string ApplicationAccessible::GetDescription() const
{
  return {};
}

std::string ApplicationAccessible::GetValue() const
{
  return {};
}

Accessibility::Accessible* ApplicationAccessible::GetParent()
{
  return &mParent;
}

size_t ApplicationAccessible::GetChildCount() const
{
  return mChildren.size();
}

std::vector<Accessibility::Accessible*> ApplicationAccessible::GetChildren()
{
  return mChildren;
}

Accessibility::Accessible* ApplicationAccessible::GetChildAtIndex(size_t index)
{
  auto size = mChildren.size();
  if(index >= size)
  {
    throw std::domain_error{"invalid index " + std::to_string(index) + " for object with " + std::to_string(size) + " children"};
  }
  return mChildren[index];
}

size_t ApplicationAccessible::GetIndexInParent()
{
  if(mIsEmbedded)
  {
    return 0u;
  }
  throw std::domain_error{"can't call GetIndexInParent on application object"};
}

Accessibility::Role ApplicationAccessible::GetRole() const
{
  return Accessibility::Role::APPLICATION;
}

Accessibility::States ApplicationAccessible::GetStates()
{
  Accessibility::States result;

  for(auto* child : mChildren)
  {
    result = result | child->GetStates();
  }

  // The Application object should never have the SENSITIVE state
  result[Accessibility::State::SENSITIVE] = false;

  return result;
}

Accessibility::Attributes ApplicationAccessible::GetAttributes() const
{
  return {};
}

bool ApplicationAccessible::DoGesture(const Accessibility::GestureInfo& gestureInfo)
{
  return false;
}

std::vector<Accessibility::Relation> ApplicationAccessible::GetRelationSet()
{
  return {};
}

Accessibility::Address ApplicationAccessible::GetAddress() const
{
  return {"", "root"};
}

std::string ApplicationAccessible::GetStringProperty(std::string propertyName) const
{
  return {};
}

// Application interface implementation
std::string ApplicationAccessible::GetToolkitName() const
{
  return mToolkitName;
}

std::string ApplicationAccessible::GetVersion() const
{
  auto& callbacks = Accessibility::GetPlatformCallbacks();
  if(callbacks.getToolkitVersion)
  {
    return callbacks.getToolkitVersion();
  }
  return {};
}

bool ApplicationAccessible::GetIncludeHidden() const
{
  return mShouldIncludeHidden;
}

bool ApplicationAccessible::SetIncludeHidden(bool includeHidden)
{
  if(mShouldIncludeHidden != includeHidden)
  {
    mShouldIncludeHidden = includeHidden;
    return true;
  }
  return false;
}

// Socket interface implementation
Accessibility::Address ApplicationAccessible::Embed(Accessibility::Address plug)
{
  mIsEmbedded = true;
  mParent.SetAddress(plug);
  return GetAddress();
}

void ApplicationAccessible::Unembed(Accessibility::Address plug)
{
  if(mParent.GetAddress() == plug)
  {
    mIsEmbedded = false;
    mParent.SetAddress({});
    if(auto bridge = Accessibility::Bridge::GetCurrentBridge())
    {
      bridge->SetExtentsOffset(0, 0);
    }
  }
}

void ApplicationAccessible::SetOffset(std::int32_t x, std::int32_t y)
{
  if(!mIsEmbedded)
  {
    return;
  }
  if(auto bridge = Accessibility::Bridge::GetCurrentBridge())
  {
    bridge->SetExtentsOffset(x, y);
  }
}

// Component interface implementation
Accessibility::Rect<float> ApplicationAccessible::GetExtents(Accessibility::CoordinateType type) const
{
  using limits = std::numeric_limits<float>;

  float minX = limits::max();
  float minY = limits::max();
  float maxX = limits::min();
  float maxY = limits::min();

  for(Accessibility::Accessible* child : mChildren)
  {
    auto extents = child->GetExtents(type);

    minX = std::min(minX, extents.x);
    minY = std::min(minY, extents.y);
    maxX = std::max(maxX, extents.x + extents.width);
    maxY = std::max(maxY, extents.y + extents.height);
  }

  return {minX, minY, maxX - minX, maxY - minY};
}

Accessibility::ComponentLayer ApplicationAccessible::GetLayer() const
{
  return Accessibility::ComponentLayer::WINDOW;
}

std::int16_t ApplicationAccessible::GetMdiZOrder() const
{
  return 0;
}

bool ApplicationAccessible::GrabFocus()
{
  return false;
}

double ApplicationAccessible::GetAlpha() const
{
  return 0.0;
}

bool ApplicationAccessible::GrabHighlight()
{
  return false;
}

bool ApplicationAccessible::ClearHighlight()
{
  return false;
}

bool ApplicationAccessible::IsScrollable() const
{
  return false;
}

void ApplicationAccessible::InitDefaultFeatures()
{
  mCollection = std::make_shared<CollectionImpl>(weak_from_this());
  AddFeature<Application>(shared_from_this());
  AddFeature<Collection>(shared_from_this());
  AddFeature<Socket>(shared_from_this());
}

std::vector<Accessible*> ApplicationAccessible::GetMatches(MatchRule rule, uint32_t sortBy, size_t maxCount)
{
  if(mCollection)
  {
    return mCollection->GetMatches(rule, sortBy, maxCount);
  }

  return {};
}

std::vector<Accessible*> ApplicationAccessible::GetMatchesInMatches(MatchRule firstRule, MatchRule secondRule, uint32_t sortBy, int32_t firstCount, int32_t secondCount)
{
  if(mCollection)
  {
    return mCollection->GetMatchesInMatches(firstRule, secondRule, sortBy, firstCount, secondCount);
  }

  return {};
}

} //namespace Accessibility

// BridgeBase implementation
BridgeBase::BridgeBase()
: mApplication{std::make_shared<ApplicationAccessible>()}
{
  mApplication->InitDefaultFeatures();
}

BridgeBase::~BridgeBase()
{
}

void BridgeBase::AddCoalescableMessage(CoalescableMessages kind, Accessibility::Accessible* obj, float delay, std::function<void()> functor)
{
  if(delay < 0)
  {
    delay = 0;
  }

  auto countdownBase = static_cast<unsigned int>(delay * 10);
  auto it            = mCoalescableMessages.insert({{kind, obj}, {countdownBase, countdownBase, {}}});

  if(it.second)
  {
    functor();
  }
  else
  {
    std::get<1>(it.first->second) = countdownBase;
    std::get<2>(it.first->second) = std::move(functor);
  }

  if(!gTickTimer.IsRunning())
  {
    gTickTimer.Start(100, [this]() { return TickCoalescableMessages(); });
  }
}

bool BridgeBase::TickCoalescableMessages()
{
  for(auto it = mCoalescableMessages.begin(); it != mCoalescableMessages.end();)
  {
    auto& countdown     = std::get<0>(it->second);
    auto  countdownBase = std::get<1>(it->second);
    auto& functor       = std::get<2>(it->second);

    if(countdown)
    {
      --countdown;
    }
    else
    {
      if(functor)
      {
        functor();
        functor   = {};
        countdown = countdownBase;
      }
      else
      {
        it = mCoalescableMessages.erase(it);
        continue;
      }
    }

    ++it;
  }

  return !mCoalescableMessages.empty();
}

void BridgeBase::UpdateRegisteredEvents()
{
  using ReturnType = std::vector<std::tuple<std::string, std::string>>;

  mRegistry.method<DBus::ValueOrError<ReturnType>()>("GetRegisteredEvents").asyncCall([this](DBus::ValueOrError<ReturnType> msg)
  {
    if(!msg)
    {
      LOG() << "Get registered events failed";
      return;
    }

    IsBoundsChangedEventAllowed = false;
    ReturnType values           = std::get<ReturnType>(msg.getValues());

    for(long unsigned int i = 0; i < values.size(); i++)
    {
      if(!std::get<1>(values[i]).compare("Object:BoundsChanged"))
      {
        IsBoundsChangedEventAllowed = true;
      }
    }
  });
}

BridgeBase::ForceUpResult BridgeBase::ForceUp()
{
  //TODO: checking mBusName is enough? or a new variable to check bridge state?
  if(Bridge::ForceUp() == ForceUpResult::ALREADY_UP && !GetBusName().empty())
  {
    return ForceUpResult::ALREADY_UP;
  }

  if(!DBusWrapper::Installed())
  {
    // No IPC transport. Bridge is up for local accessibility.
    return ForceUpResult::JUST_STARTED;
  }

  auto proxy = DBus::DBusClient{dbusLocators::atspi::BUS, dbusLocators::atspi::OBJ_PATH, dbusLocators::atspi::BUS_INTERFACE, DBus::ConnectionType::SESSION};
  auto addr  = proxy.method<std::string()>(dbusLocators::atspi::GET_ADDRESS).call();

  if(!addr)
  {
    ACCESSIBILITY_LOG_ERROR("failed at call '%s': %s\n", dbusLocators::atspi::GET_ADDRESS, addr.getError().message.c_str());
    return ForceUpResult::FAILED;
  }

  auto connectionPtr = DBusWrapper::Installed()->eldbus_address_connection_get_impl(std::get<0>(addr));
  mData->mBusName    = DBus::getConnectionName(connectionPtr);
  mIpcServer         = std::make_unique<Ipc::DbusIpcServer>(connectionPtr);

  {
    DBus::DBusInterfaceDescription desc{Accessible::GetInterfaceName(AtspiInterface::CACHE)};
    AddFunctionToInterface(desc, "GetItems", &BridgeBase::GetItems);
    mIpcServer->addInterface(AtspiDbusPathCache, desc);
  }

  {
    DBus::DBusInterfaceDescription desc{Accessible::GetInterfaceName(AtspiInterface::APPLICATION)};
    AddGetSetPropertyToInterface(desc, "Id", &BridgeBase::GetId, &BridgeBase::SetId);
    mIpcServer->addInterface(AtspiPath, desc);
  }

  mRegistry = {AtspiDbusNameRegistry, AtspiDbusPathRegistry, Accessible::GetInterfaceName(AtspiInterface::REGISTRY), getConnection()};
  UpdateRegisteredEvents();

  mRegistry.addSignal<void(void)>("EventListenerRegistered", [this](void)
  {
    UpdateRegisteredEvents();
  });

  mRegistry.addSignal<void(void)>("EventListenerDeregistered", [this](void)
  {
    UpdateRegisteredEvents();
  });

  return ForceUpResult::JUST_STARTED;
}

void BridgeBase::ForceDown()
{
  Bridge::ForceDown();
  gTickTimer.Stop();
  mCoalescableMessages.clear();
  if(auto* wrapper = DBusWrapper::Installed())
  {
    wrapper->Strings.clear();
  }
  mRegistry   = {};
  mIpcServer.reset();
}

const std::string& BridgeBase::GetBusName() const
{
  static std::string empty;
  return mData ? mData->mBusName : empty;
}

Accessible* BridgeBase::FindByPath(const std::string& name) const
{
  try
  {
    return Find(name);
  }
  catch(std::domain_error&)
  {
    return nullptr;
  }
}

void BridgeBase::AddTopLevelWindow(Accessible* windowAccessible)
{
  if(!windowAccessible)
  {
    return;
  }

  // Prevent adding the same window accessible twice.
  for(auto* child : mApplication->mChildren)
  {
    if(child == windowAccessible)
    {
      return;
    }
  }

  // Adds Window to a list of Windows.
  mApplication->mChildren.push_back(windowAccessible);
  SetIsOnRootLevel(windowAccessible);
}

void BridgeBase::RemoveTopLevelWindow(Accessible* windowAccessible)
{
  for(auto i = 0u; i < mApplication->mChildren.size(); ++i)
  {
    if(mApplication->mChildren[i] == windowAccessible)
    {
      mApplication->mChildren.erase(mApplication->mChildren.begin() + i);
      Emit(windowAccessible, WindowEvent::DESTROY);
      break;
    }
  }
}

void BridgeBase::CompressDefaultLabels()
{
  // With raw pointers there is no expiry detection; nothing to compress.
}

void BridgeBase::RegisterDefaultLabel(Accessible* accessible)
{
  if(!accessible)
  {
    ACCESSIBILITY_LOG_ERROR("Cannot register default label: accessible is null");
    return;
  }

  // Walk up the parent chain to find the window root (top-most accessible whose parent is the application).
  Accessible* windowRoot = accessible;
  while(windowRoot)
  {
    auto* parent = windowRoot->GetParent();
    if(!parent || parent == mApplication.get())
    {
      break;
    }
    windowRoot = parent;
  }

  if(!windowRoot)
  {
    ACCESSIBILITY_LOG_ERROR("Cannot register default label: unable to find window root");
    return;
  }

  auto it = std::find_if(mDefaultLabels.begin(), mDefaultLabels.end(), [accessible](const DefaultLabelType& label)
  {
    return label.second == accessible;
  });

  if(it == mDefaultLabels.end())
  {
    mDefaultLabels.push_back({windowRoot, accessible});
  }
  else if(it->first != windowRoot)
  {
    // TODO: Tentative implementation. It is yet to be specified what should happen
    // when the same object is re-registered as a default label for another window.
    *it = {windowRoot, accessible};
  }
  else // it->first == windowRoot && it->second == accessible
  {
    // Nothing to do
  }
}

void BridgeBase::UnregisterDefaultLabel(Accessible* accessible)
{
  if(!accessible)
  {
    return;
  }

  mDefaultLabels.remove_if([accessible](const DefaultLabelType& label)
  {
    return label.second == accessible;
  });
}

Accessible* BridgeBase::GetDefaultLabel(Accessible* root)
{
  if(!root)
  {
    ACCESSIBILITY_LOG_ERROR("Cannot get defaultLabel as given root accessible is null.");
    return nullptr;
  }

  auto it = std::find_if(mDefaultLabels.rbegin(), mDefaultLabels.rend(), [root](const DefaultLabelType& label)
  {
    return label.first == root;
  });

  if(it != mDefaultLabels.rend() && it->second)
  {
    return it->second;
  }

  return root;
}

std::string BridgeBase::StripPrefix(const std::string& path)
{
  auto size = strlen(AtspiPath);
  return path.substr(size + 1);
}

Accessible* BridgeBase::Find(const std::string& path) const
{
  if(path == "root")
  {
    return mApplication.get();
  }

  auto accessible = GetAccessible(path);
  if(!accessible || (!mApplication->mShouldIncludeHidden && accessible->IsHidden()))
  {
    throw std::domain_error{"unknown object '" + path + "'"};
  }

  return accessible.get();
}

Accessible* BridgeBase::Find(const Address& ptr) const
{
  assert(ptr.GetBus() == mData->mBusName);
  return Find(ptr.GetPath());
}

Accessible* BridgeBase::FindCurrentObject() const
{
  auto path = mIpcServer ? mIpcServer->getCurrentObjectPath() : std::string{};
  auto size = strlen(AtspiPath);

  if(path.size() <= size)
  {
    throw std::domain_error{"invalid path '" + path + "'"};
  }

  if(path.substr(0, size) != AtspiPath)
  {
    throw std::domain_error{"invalid path '" + path + "'"};
  }

  if(path[size] != '/')
  {
    throw std::domain_error{"invalid path '" + path + "'"};
  }

  return Find(StripPrefix(path));
}

void BridgeBase::SetId(int id)
{
  this->mId = id;
}

int BridgeBase::GetId()
{
  return this->mId;
}

// TODO: Remove or make it usable
auto BridgeBase::GetItems() -> DBus::ValueOrError<std::vector<CacheElementType>>
{
  return {};
}

DBus::DBusServer& BridgeBase::getDbusServer()
{
  return static_cast<Ipc::DbusIpcServer&>(*mIpcServer).getDbusServer();
}

const DBus::DBusServer& BridgeBase::getDbusServer() const
{
  return static_cast<const Ipc::DbusIpcServer&>(*mIpcServer).getDbusServer();
}

const DBusWrapper::ConnectionPtr& BridgeBase::getConnection() const
{
  return static_cast<const Ipc::DbusIpcServer&>(*mIpcServer).getConnection();
}
