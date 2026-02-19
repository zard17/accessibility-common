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
#include <accessibility/internal/service/atspi-node-proxy.h>

// EXTERNAL INCLUDES
#include <array>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/accessibility-common.h>
#include <accessibility/internal/bridge/dbus/dbus-locators.h>

namespace Accessibility
{
namespace
{
static constexpr const char* ACCESSIBLE_IFACE = "org.a11y.atspi.Accessible";
static constexpr const char* COMPONENT_IFACE  = "org.a11y.atspi.Component";
static constexpr const char* ACTION_IFACE     = "org.a11y.atspi.Action";
static constexpr const char* VALUE_IFACE      = "org.a11y.atspi.Value";
static constexpr const char* TEXT_IFACE        = "org.a11y.atspi.Text";
} // namespace

AtSpiNodeProxy::AtSpiNodeProxy(Address address,
                               DBusWrapper::ConnectionPtr connection,
                               NodeProxyFactory factory)
: mAddress(std::move(address)),
  mConnection(std::move(connection)),
  mFactory(std::move(factory))
{
}

DBus::DBusClient AtSpiNodeProxy::createAccessibleClient()
{
  return DBus::DBusClient{mAddress.GetBus(), mAddress.GetPath(), ACCESSIBLE_IFACE, mConnection};
}

DBus::DBusClient AtSpiNodeProxy::createComponentClient()
{
  return DBus::DBusClient{mAddress.GetBus(), mAddress.GetPath(), COMPONENT_IFACE, mConnection};
}

DBus::DBusClient AtSpiNodeProxy::createActionClient()
{
  return DBus::DBusClient{mAddress.GetBus(), mAddress.GetPath(), ACTION_IFACE, mConnection};
}

DBus::DBusClient AtSpiNodeProxy::createValueClient()
{
  return DBus::DBusClient{mAddress.GetBus(), mAddress.GetPath(), VALUE_IFACE, mConnection};
}

DBus::DBusClient AtSpiNodeProxy::createTextClient()
{
  return DBus::DBusClient{mAddress.GetBus(), mAddress.GetPath(), TEXT_IFACE, mConnection};
}

// ========================================================================
// Accessible interface (19 methods)
// ========================================================================

std::string AtSpiNodeProxy::getName()
{
  auto client = createAccessibleClient();
  auto result = client.property<std::string>("Name").get();
  return result ? std::get<0>(result.getValues()) : "";
}

std::string AtSpiNodeProxy::getDescription()
{
  auto client = createAccessibleClient();
  auto result = client.property<std::string>("Description").get();
  return result ? std::get<0>(result.getValues()) : "";
}

Role AtSpiNodeProxy::getRole()
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
  return result ? static_cast<Role>(std::get<0>(result.getValues())) : Role::UNKNOWN;
}

std::string AtSpiNodeProxy::getRoleName()
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<std::string>()>("GetRoleName").call();
  return result ? std::get<0>(result.getValues()) : "";
}

std::string AtSpiNodeProxy::getLocalizedRoleName()
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<std::string>()>("GetLocalizedRoleName").call();
  return result ? std::get<0>(result.getValues()) : "";
}

States AtSpiNodeProxy::getStates()
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<std::array<uint32_t, 2>>()>("GetState").call();
  if(result)
  {
    return States{std::get<0>(result.getValues())};
  }
  return {};
}

Attributes AtSpiNodeProxy::getAttributes()
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<std::unordered_map<std::string, std::string>>()>("GetAttributes").call();
  return result ? std::get<0>(result.getValues()) : Attributes{};
}

std::vector<std::string> AtSpiNodeProxy::getInterfaces()
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<std::vector<std::string>>()>("GetInterfaces").call();
  return result ? std::get<0>(result.getValues()) : std::vector<std::string>{};
}

std::shared_ptr<NodeProxy> AtSpiNodeProxy::getParent()
{
  auto client = createAccessibleClient();
  auto result = client.property<Address>("Parent").get();
  if(result)
  {
    auto addr = std::get<0>(result.getValues());
    if(addr)
    {
      return mFactory(addr);
    }
  }
  return nullptr;
}

int32_t AtSpiNodeProxy::getChildCount()
{
  auto client = createAccessibleClient();
  auto result = client.property<int>("ChildCount").get();
  return result ? static_cast<int32_t>(std::get<0>(result.getValues())) : 0;
}

std::shared_ptr<NodeProxy> AtSpiNodeProxy::getChildAtIndex(int32_t index)
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<Address>(int)>("GetChildAtIndex").call(static_cast<int>(index));
  if(result)
  {
    auto addr = std::get<0>(result.getValues());
    if(addr)
    {
      return mFactory(addr);
    }
  }
  return nullptr;
}

std::vector<std::shared_ptr<NodeProxy>> AtSpiNodeProxy::getChildren()
{
  std::vector<std::shared_ptr<NodeProxy>> result;
  int32_t count = getChildCount();
  result.reserve(count);
  for(int32_t i = 0; i < count; ++i)
  {
    auto child = getChildAtIndex(i);
    if(child)
    {
      result.push_back(std::move(child));
    }
  }
  return result;
}

int32_t AtSpiNodeProxy::getIndexInParent()
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<int32_t>()>("GetIndexInParent").call();
  return result ? std::get<0>(result.getValues()) : 0;
}

std::vector<RemoteRelation> AtSpiNodeProxy::getRelationSet()
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<std::vector<std::tuple<uint32_t, std::vector<Address>>>>()>("GetRelationSet").call();
  if(!result)
  {
    return {};
  }

  std::vector<RemoteRelation> relations;
  auto& tuples = std::get<0>(result.getValues());
  relations.reserve(tuples.size());
  for(auto& [type, targets] : tuples)
  {
    RemoteRelation rel;
    rel.type = static_cast<RelationType>(type);
    rel.targets = std::move(targets);
    relations.push_back(std::move(rel));
  }
  return relations;
}

std::shared_ptr<NodeProxy> AtSpiNodeProxy::getNeighbor(std::shared_ptr<NodeProxy> root, bool forward, NeighborSearchMode searchMode)
{
  auto client = createAccessibleClient();
  std::string rootPath;
  if(root)
  {
    rootPath = root->getAddress().GetPath();
  }

  auto result = client.method<DBus::ValueOrError<Address, uint8_t>(std::string, int32_t, int32_t)>("GetNeighbor")
    .call(rootPath, forward ? 1 : 0, static_cast<int32_t>(searchMode));

  if(result)
  {
    auto addr = std::get<0>(result.getValues());
    if(addr)
    {
      return mFactory(addr);
    }
  }
  return nullptr;
}

std::shared_ptr<NodeProxy> AtSpiNodeProxy::getNavigableAtPoint(int32_t x, int32_t y, CoordinateType type)
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<Address, uint8_t, Address>(int32_t, int32_t, uint32_t)>("GetNavigableAtPoint")
    .call(x, y, static_cast<uint32_t>(type));

  if(result)
  {
    auto addr = std::get<0>(result.getValues());
    if(addr)
    {
      return mFactory(addr);
    }
  }
  return nullptr;
}

ReadingMaterial AtSpiNodeProxy::getReadingMaterial()
{
  auto client = createAccessibleClient();
  using RMType = DBus::ValueOrError<
    std::unordered_map<std::string, std::string>,
    std::string, std::string, std::string,
    uint32_t, States, std::string,
    int32_t, double, std::string,
    double, double, double,
    std::string, int32_t,
    bool, bool, int32_t, int32_t,
    Address, States, int32_t, uint32_t, int32_t, Address>;

  auto result = client.method<RMType()>("GetReadingMaterial").call();
  ReadingMaterial rm{};
  if(result)
  {
    auto& v = result.getValues();
    rm.attributes          = std::get<0>(v);
    rm.name                = std::get<1>(v);
    rm.labeledByName       = std::get<2>(v);
    rm.textIfceName        = std::get<3>(v);
    rm.role                = static_cast<Role>(std::get<4>(v));
    rm.states              = std::get<5>(v);
    rm.localizedName       = std::get<6>(v);
    rm.childCount          = std::get<7>(v);
    rm.currentValue        = std::get<8>(v);
    rm.formattedValue      = std::get<9>(v);
    rm.minimumIncrement    = std::get<10>(v);
    rm.maximumValue        = std::get<11>(v);
    rm.minimumValue        = std::get<12>(v);
    rm.description         = std::get<13>(v);
    rm.indexInParent       = std::get<14>(v);
    rm.isSelectedInParent  = std::get<15>(v);
    rm.hasCheckBoxChild    = std::get<16>(v);
    rm.listChildrenCount   = std::get<17>(v);
    rm.firstSelectedChildIndex = std::get<18>(v);
    rm.parentAddress       = std::get<19>(v);
    rm.parentStates        = std::get<20>(v);
    rm.parentChildCount    = std::get<21>(v);
    rm.parentRole          = static_cast<Role>(std::get<22>(v));
    rm.selectedChildCount  = std::get<23>(v);
    rm.describedByAddress  = std::get<24>(v);
  }
  return rm;
}

NodeInfo AtSpiNodeProxy::getNodeInfo()
{
  auto client = createAccessibleClient();
  using NIType = DBus::ValueOrError<
    std::string, std::string, std::string,
    std::unordered_map<std::string, std::string>,
    States,
    std::tuple<int32_t, int32_t, int32_t, int32_t>,
    std::tuple<int32_t, int32_t, int32_t, int32_t>,
    double, double, double, double, std::string>;

  auto result = client.method<NIType()>("GetNodeInfo").call();
  NodeInfo info{};
  if(result)
  {
    auto& v = result.getValues();
    info.roleName        = std::get<0>(v);
    info.name            = std::get<1>(v);
    info.toolkitName     = std::get<2>(v);
    info.attributes      = std::get<3>(v);
    info.states          = std::get<4>(v);
    auto& se             = std::get<5>(v);
    info.screenExtents   = Rect<int>{std::get<0>(se), std::get<1>(se), std::get<2>(se), std::get<3>(se)};
    auto& we             = std::get<6>(v);
    info.windowExtents   = Rect<int>{std::get<0>(we), std::get<1>(we), std::get<2>(we), std::get<3>(we)};
    info.currentValue    = std::get<7>(v);
    info.minimumIncrement = std::get<8>(v);
    info.maximumValue    = std::get<9>(v);
    info.minimumValue    = std::get<10>(v);
    info.formattedValue  = std::get<11>(v);
  }
  return info;
}

DefaultLabelInfo AtSpiNodeProxy::getDefaultLabelInfo()
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<Address, uint32_t, std::unordered_map<std::string, std::string>>()>("GetDefaultLabelInfo").call();
  DefaultLabelInfo info{};
  if(result)
  {
    auto& v = result.getValues();
    info.address    = std::get<0>(v);
    info.role       = static_cast<Role>(std::get<1>(v));
    info.attributes = std::get<2>(v);
  }
  return info;
}

// ========================================================================
// Component interface (7 methods)
// ========================================================================

Rect<int> AtSpiNodeProxy::getExtents(CoordinateType type)
{
  auto client = createComponentClient();
  auto result = client.method<DBus::ValueOrError<std::tuple<int32_t, int32_t, int32_t, int32_t>>(uint32_t)>("GetExtents")
    .call(static_cast<uint32_t>(type));
  if(result)
  {
    auto& ext = std::get<0>(result.getValues());
    return {std::get<0>(ext), std::get<1>(ext), std::get<2>(ext), std::get<3>(ext)};
  }
  return {};
}

ComponentLayer AtSpiNodeProxy::getLayer()
{
  auto client = createComponentClient();
  auto result = client.method<DBus::ValueOrError<uint32_t>()>("GetLayer").call();
  return result ? static_cast<ComponentLayer>(std::get<0>(result.getValues())) : ComponentLayer::INVALID;
}

double AtSpiNodeProxy::getAlpha()
{
  auto client = createComponentClient();
  auto result = client.method<DBus::ValueOrError<double>()>("GetAlpha").call();
  return result ? std::get<0>(result.getValues()) : 1.0;
}

bool AtSpiNodeProxy::grabFocus()
{
  auto client = createComponentClient();
  auto result = client.method<DBus::ValueOrError<bool>()>("GrabFocus").call();
  return result ? std::get<0>(result.getValues()) : false;
}

bool AtSpiNodeProxy::grabHighlight()
{
  auto client = createComponentClient();
  auto result = client.method<DBus::ValueOrError<bool>()>("GrabHighlight").call();
  return result ? std::get<0>(result.getValues()) : false;
}

bool AtSpiNodeProxy::clearHighlight()
{
  auto client = createComponentClient();
  auto result = client.method<DBus::ValueOrError<bool>()>("ClearHighlight").call();
  return result ? std::get<0>(result.getValues()) : false;
}

bool AtSpiNodeProxy::doGesture(const GestureInfo& gesture)
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<bool>(Gesture, int32_t, int32_t, int32_t, int32_t, GestureState, uint32_t)>("DoGesture")
    .call(gesture.type, gesture.startPointX, gesture.startPointY, gesture.endPointX, gesture.endPointY, gesture.state, gesture.eventTime);
  return result ? std::get<0>(result.getValues()) : false;
}

// ========================================================================
// Action interface (3 methods)
// ========================================================================

int32_t AtSpiNodeProxy::getActionCount()
{
  auto client = createActionClient();
  auto result = client.property<int>("NActions").get();
  return result ? static_cast<int32_t>(std::get<0>(result.getValues())) : 0;
}

std::string AtSpiNodeProxy::getActionName(int32_t index)
{
  auto client = createActionClient();
  auto result = client.method<DBus::ValueOrError<std::string>(int32_t)>("GetName").call(index);
  return result ? std::get<0>(result.getValues()) : "";
}

bool AtSpiNodeProxy::doActionByName(const std::string& name)
{
  auto client = createActionClient();
  auto result = client.method<DBus::ValueOrError<bool>(std::string)>("DoActionName").call(name);
  return result ? std::get<0>(result.getValues()) : false;
}

// ========================================================================
// Value interface (5 methods)
// ========================================================================

double AtSpiNodeProxy::getCurrentValue()
{
  auto client = createValueClient();
  auto result = client.property<double>("CurrentValue").get();
  return result ? std::get<0>(result.getValues()) : 0.0;
}

double AtSpiNodeProxy::getMaximumValue()
{
  auto client = createValueClient();
  auto result = client.property<double>("MaximumValue").get();
  return result ? std::get<0>(result.getValues()) : 0.0;
}

double AtSpiNodeProxy::getMinimumValue()
{
  auto client = createValueClient();
  auto result = client.property<double>("MinimumValue").get();
  return result ? std::get<0>(result.getValues()) : 0.0;
}

double AtSpiNodeProxy::getMinimumIncrement()
{
  auto client = createValueClient();
  auto result = client.property<double>("MinimumIncrement").get();
  return result ? std::get<0>(result.getValues()) : 0.0;
}

bool AtSpiNodeProxy::setCurrentValue(double value)
{
  auto client = createValueClient();
  client.property<double>("CurrentValue").set(value);
  return true;
}

// ========================================================================
// Text interface (5 methods)
// ========================================================================

std::string AtSpiNodeProxy::getText(int32_t startOffset, int32_t endOffset)
{
  auto client = createTextClient();
  auto result = client.method<DBus::ValueOrError<std::string>(int32_t, int32_t)>("GetText").call(startOffset, endOffset);
  return result ? std::get<0>(result.getValues()) : "";
}

int32_t AtSpiNodeProxy::getCharacterCount()
{
  auto client = createTextClient();
  auto result = client.property<int>("CharacterCount").get();
  return result ? static_cast<int32_t>(std::get<0>(result.getValues())) : 0;
}

int32_t AtSpiNodeProxy::getCursorOffset()
{
  auto client = createTextClient();
  auto result = client.property<int>("CaretOffset").get();
  return result ? static_cast<int32_t>(std::get<0>(result.getValues())) : 0;
}

Range AtSpiNodeProxy::getTextAtOffset(int32_t offset, TextBoundary boundary)
{
  auto client = createTextClient();
  auto result = client.method<DBus::ValueOrError<std::string, int32_t, int32_t>(int32_t, uint32_t)>("GetTextAtOffset")
    .call(offset, static_cast<uint32_t>(boundary));
  if(result)
  {
    auto& v = result.getValues();
    return Range{static_cast<size_t>(std::get<1>(v)), static_cast<size_t>(std::get<2>(v)), std::get<0>(v)};
  }
  return {};
}

Range AtSpiNodeProxy::getRangeOfSelection(int32_t selectionIndex)
{
  auto client = createTextClient();
  auto result = client.method<DBus::ValueOrError<int32_t, int32_t>(int32_t)>("GetSelection").call(selectionIndex);
  if(result)
  {
    auto& v = result.getValues();
    return Range{static_cast<size_t>(std::get<0>(v)), static_cast<size_t>(std::get<1>(v))};
  }
  return {};
}

// ========================================================================
// Utility (3 methods)
// ========================================================================

Address AtSpiNodeProxy::getAddress()
{
  return mAddress;
}

std::string AtSpiNodeProxy::getStringProperty(const std::string& propertyName)
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<std::string>(std::string)>("GetStringProperty").call(propertyName);
  return result ? std::get<0>(result.getValues()) : "";
}

std::string AtSpiNodeProxy::dumpTree(int32_t detailLevel)
{
  auto client = createAccessibleClient();
  auto result = client.method<DBus::ValueOrError<std::string>(int32_t)>("DumpTree").call(detailLevel);
  return result ? std::get<0>(result.getValues()) : "";
}

} // namespace Accessibility
