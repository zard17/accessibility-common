#ifndef ACCESSIBILITY_INTERNAL_SERVICE_ATSPI_NODE_PROXY_H
#define ACCESSIBILITY_INTERNAL_SERVICE_ATSPI_NODE_PROXY_H

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
#include <functional>
#include <memory>
#include <string>

// INTERNAL INCLUDES
#include <accessibility/api/node-proxy.h>
#include <accessibility/internal/bridge/dbus/dbus.h>

namespace Accessibility
{
/**
 * @brief Factory type for creating AtSpiNodeProxy from an address.
 */
using NodeProxyFactory = std::function<std::shared_ptr<NodeProxy>(const Address&)>;

/**
 * @brief D-Bus implementation of NodeProxy.
 *
 * Each method creates a DBus::DBusClient and calls the corresponding
 * bridge method via D-Bus IPC. Pattern follows query-engine.cpp.
 */
class AtSpiNodeProxy : public NodeProxy
{
public:
  /**
   * @brief Constructs an AtSpiNodeProxy.
   *
   * @param[in] address The bus name and object path of the target accessible
   * @param[in] connection The D-Bus connection to use
   * @param[in] factory Factory for creating child/parent/neighbor proxies
   */
  AtSpiNodeProxy(Address address,
                 DBusWrapper::ConnectionPtr connection,
                 NodeProxyFactory factory);

  // --- Accessible interface ---
  std::string getName() override;
  std::string getDescription() override;
  Role getRole() override;
  std::string getRoleName() override;
  std::string getLocalizedRoleName() override;
  States getStates() override;
  Attributes getAttributes() override;
  std::vector<std::string> getInterfaces() override;
  std::shared_ptr<NodeProxy> getParent() override;
  int32_t getChildCount() override;
  std::shared_ptr<NodeProxy> getChildAtIndex(int32_t index) override;
  std::vector<std::shared_ptr<NodeProxy>> getChildren() override;
  int32_t getIndexInParent() override;
  std::vector<RemoteRelation> getRelationSet() override;
  std::shared_ptr<NodeProxy> getNeighbor(std::shared_ptr<NodeProxy> root, bool forward, NeighborSearchMode searchMode) override;
  std::shared_ptr<NodeProxy> getNavigableAtPoint(int32_t x, int32_t y, CoordinateType type) override;
  ReadingMaterial getReadingMaterial() override;
  NodeInfo getNodeInfo() override;
  DefaultLabelInfo getDefaultLabelInfo() override;

  // --- Component interface ---
  Rect<int> getExtents(CoordinateType type) override;
  ComponentLayer getLayer() override;
  double getAlpha() override;
  bool grabFocus() override;
  bool grabHighlight() override;
  bool clearHighlight() override;
  bool doGesture(const GestureInfo& gesture) override;

  // --- Action interface ---
  int32_t getActionCount() override;
  std::string getActionName(int32_t index) override;
  bool doActionByName(const std::string& name) override;

  // --- Value interface ---
  double getCurrentValue() override;
  double getMaximumValue() override;
  double getMinimumValue() override;
  double getMinimumIncrement() override;
  bool setCurrentValue(double value) override;

  // --- Text interface ---
  std::string getText(int32_t startOffset, int32_t endOffset) override;
  int32_t getCharacterCount() override;
  int32_t getCursorOffset() override;
  Range getTextAtOffset(int32_t offset, TextBoundary boundary) override;
  Range getRangeOfSelection(int32_t selectionIndex) override;

  // --- Utility ---
  Address getAddress() override;
  std::string getStringProperty(const std::string& propertyName) override;
  std::string dumpTree(int32_t detailLevel) override;

private:
  DBus::DBusClient createAccessibleClient();
  DBus::DBusClient createComponentClient();
  DBus::DBusClient createActionClient();
  DBus::DBusClient createValueClient();
  DBus::DBusClient createTextClient();

  Address                    mAddress;
  DBusWrapper::ConnectionPtr mConnection;
  NodeProxyFactory           mFactory;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_ATSPI_NODE_PROXY_H
