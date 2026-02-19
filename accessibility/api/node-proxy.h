#ifndef ACCESSIBILITY_API_NODE_PROXY_H
#define ACCESSIBILITY_API_NODE_PROXY_H

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
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/types.h>

namespace Accessibility
{
/**
 * @brief Batch reading material fetched in a single IPC round-trip.
 *
 * Fields mirror BridgeAccessible::ReadingMaterialType (24-field tuple).
 */
struct ReadingMaterial
{
  Attributes  attributes;
  std::string name;
  std::string labeledByName;
  std::string textIfceName;
  Role        role{Role::UNKNOWN};
  States      states;
  std::string localizedName;
  int32_t     childCount{0};
  double      currentValue{0.0};
  std::string formattedValue;
  double      minimumIncrement{0.0};
  double      maximumValue{0.0};
  double      minimumValue{0.0};
  std::string description;
  int32_t     indexInParent{0};
  bool        isSelectedInParent{false};
  bool        hasCheckBoxChild{false};
  int32_t     listChildrenCount{0};
  int32_t     firstSelectedChildIndex{-1};
  Address     parentAddress;
  States      parentStates;
  int32_t     parentChildCount{0};
  Role        parentRole{Role::UNKNOWN};
  int32_t     selectedChildCount{0};
  Address     describedByAddress;
};

/**
 * @brief Node information fetched in a single IPC round-trip.
 *
 * Fields mirror BridgeAccessible::NodeInfoType (12-field tuple).
 */
struct NodeInfo
{
  std::string roleName;
  std::string name;
  std::string toolkitName;
  Attributes  attributes;
  States      states;
  Rect<int>   screenExtents;
  Rect<int>   windowExtents;
  double      currentValue{0.0};
  double      minimumIncrement{0.0};
  double      maximumValue{0.0};
  double      minimumValue{0.0};
  std::string formattedValue;
};

/**
 * @brief Remote relation (relation type + list of target addresses).
 */
struct RemoteRelation
{
  RelationType             type{RelationType::NULL_OF};
  std::vector<Address>     targets;
};

/**
 * @brief Default label information for a context root.
 */
struct DefaultLabelInfo
{
  Address     address;
  Role        role{Role::UNKNOWN};
  Attributes  attributes;
};

/**
 * @brief Enumeration for neighbor search mode.
 */
enum class NeighborSearchMode
{
  NORMAL                          = 0,
  RECURSE_FROM_ROOT               = 1,
  CONTINUE_AFTER_FAILED_RECURSION = 2,
  RECURSE_TO_OUTSIDE              = 3,
};

/**
 * @brief Abstract proxy interface for querying a single accessible node.
 *
 * Each method corresponds to an IPC call to the app-side bridge.
 * Concrete implementations: AtSpiNodeProxy (D-Bus), TidlNodeProxy (rpc_port).
 * For testing: MockNodeProxy (direct C++ calls, no IPC).
 */
class NodeProxy
{
public:
  virtual ~NodeProxy() = default;

  // --- Accessible interface (19 methods) ---

  /**
   * @brief Gets the accessible name.
   */
  virtual std::string getName() = 0;

  /**
   * @brief Gets the accessible description.
   */
  virtual std::string getDescription() = 0;

  /**
   * @brief Gets the accessibility role.
   */
  virtual Role getRole() = 0;

  /**
   * @brief Gets the role name as a human-readable string.
   */
  virtual std::string getRoleName() = 0;

  /**
   * @brief Gets the localized role name.
   */
  virtual std::string getLocalizedRoleName() = 0;

  /**
   * @brief Gets the current accessibility states.
   */
  virtual States getStates() = 0;

  /**
   * @brief Gets the accessibility attributes.
   */
  virtual Attributes getAttributes() = 0;

  /**
   * @brief Gets the list of implemented AT-SPI interface names.
   */
  virtual std::vector<std::string> getInterfaces() = 0;

  /**
   * @brief Gets the parent node proxy.
   */
  virtual std::shared_ptr<NodeProxy> getParent() = 0;

  /**
   * @brief Gets the number of children.
   */
  virtual int32_t getChildCount() = 0;

  /**
   * @brief Gets the child at the given index.
   */
  virtual std::shared_ptr<NodeProxy> getChildAtIndex(int32_t index) = 0;

  /**
   * @brief Gets all children.
   */
  virtual std::vector<std::shared_ptr<NodeProxy>> getChildren() = 0;

  /**
   * @brief Gets the index of this node in its parent's child list.
   */
  virtual int32_t getIndexInParent() = 0;

  /**
   * @brief Gets the relation set.
   */
  virtual std::vector<RemoteRelation> getRelationSet() = 0;

  /**
   * @brief Gets the neighboring node in navigation order.
   *
   * @param[in] root The root node for navigation scope
   * @param[in] forward true for next, false for previous
   * @param[in] searchMode The search mode
   * @return The neighbor node proxy
   */
  virtual std::shared_ptr<NodeProxy> getNeighbor(std::shared_ptr<NodeProxy> root, bool forward, NeighborSearchMode searchMode) = 0;

  /**
   * @brief Gets the navigable node at the given screen point.
   *
   * @param[in] x The X coordinate
   * @param[in] y The Y coordinate
   * @param[in] type The coordinate type
   * @return The node proxy at the point, or nullptr
   */
  virtual std::shared_ptr<NodeProxy> getNavigableAtPoint(int32_t x, int32_t y, CoordinateType type) = 0;

  /**
   * @brief Gets reading material in a single batch call.
   */
  virtual ReadingMaterial getReadingMaterial() = 0;

  /**
   * @brief Gets node info in a single batch call.
   */
  virtual NodeInfo getNodeInfo() = 0;

  /**
   * @brief Gets the default label information.
   */
  virtual DefaultLabelInfo getDefaultLabelInfo() = 0;

  // --- Component interface (7 methods) ---

  /**
   * @brief Gets the screen extents.
   */
  virtual Rect<int> getExtents(CoordinateType type) = 0;

  /**
   * @brief Gets the component layer.
   */
  virtual ComponentLayer getLayer() = 0;

  /**
   * @brief Gets the alpha value.
   */
  virtual double getAlpha() = 0;

  /**
   * @brief Requests focus for this node.
   */
  virtual bool grabFocus() = 0;

  /**
   * @brief Highlights this node.
   */
  virtual bool grabHighlight() = 0;

  /**
   * @brief Clears the highlight from this node.
   */
  virtual bool clearHighlight() = 0;

  /**
   * @brief Performs a gesture on this node.
   */
  virtual bool doGesture(const GestureInfo& gesture) = 0;

  // --- Action interface (3 methods) ---

  /**
   * @brief Gets the number of available actions.
   */
  virtual int32_t getActionCount() = 0;

  /**
   * @brief Gets the name of the action at the given index.
   */
  virtual std::string getActionName(int32_t index) = 0;

  /**
   * @brief Performs the action with the given name.
   */
  virtual bool doActionByName(const std::string& name) = 0;

  // --- Value interface (5 methods) ---

  /**
   * @brief Gets the current value.
   */
  virtual double getCurrentValue() = 0;

  /**
   * @brief Gets the maximum value.
   */
  virtual double getMaximumValue() = 0;

  /**
   * @brief Gets the minimum value.
   */
  virtual double getMinimumValue() = 0;

  /**
   * @brief Gets the minimum increment.
   */
  virtual double getMinimumIncrement() = 0;

  /**
   * @brief Sets the current value.
   */
  virtual bool setCurrentValue(double value) = 0;

  // --- Text interface (5 methods) ---

  /**
   * @brief Gets all text content.
   */
  virtual std::string getText(int32_t startOffset, int32_t endOffset) = 0;

  /**
   * @brief Gets the total character count.
   */
  virtual int32_t getCharacterCount() = 0;

  /**
   * @brief Gets the cursor offset.
   */
  virtual int32_t getCursorOffset() = 0;

  /**
   * @brief Gets text at the given offset with boundary type.
   */
  virtual Range getTextAtOffset(int32_t offset, TextBoundary boundary) = 0;

  /**
   * @brief Gets the range of the current selection.
   */
  virtual Range getRangeOfSelection(int32_t selectionIndex) = 0;

  // --- Utility (3 methods) ---

  /**
   * @brief Gets the unique address of this node on the accessibility bus.
   */
  virtual Address getAddress() = 0;

  /**
   * @brief Gets an arbitrary string property.
   */
  virtual std::string getStringProperty(const std::string& propertyName) = 0;

  /**
   * @brief Dumps the subtree rooted at this node.
   */
  virtual std::string dumpTree(int32_t detailLevel) = 0;
};

} // namespace Accessibility

#endif // ACCESSIBILITY_API_NODE_PROXY_H
