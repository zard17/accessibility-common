#ifndef ACCESSIBILITY_INTERNAL_SERVICE_TIDL_NODE_PROXY_H
#define ACCESSIBILITY_INTERNAL_SERVICE_TIDL_NODE_PROXY_H

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

// INTERNAL INCLUDES
#include <accessibility/api/node-proxy.h>

namespace Accessibility
{
/**
 * @brief TIDL scaffold implementation of NodeProxy.
 *
 * All methods return defaults. Will be replaced with rpc_port::proxy::AccessibilityBridge
 * calls in Phase 2.6 Stage B when Tizen device is available.
 */
class TidlNodeProxy : public NodeProxy
{
public:
  TidlNodeProxy() = default;

  // --- Accessible interface ---
  std::string getName() override { return ""; }
  std::string getDescription() override { return ""; }
  Role getRole() override { return Role::UNKNOWN; }
  std::string getRoleName() override { return ""; }
  std::string getLocalizedRoleName() override { return ""; }
  States getStates() override { return {}; }
  Attributes getAttributes() override { return {}; }
  std::vector<std::string> getInterfaces() override { return {}; }
  std::shared_ptr<NodeProxy> getParent() override { return nullptr; }
  int32_t getChildCount() override { return 0; }
  std::shared_ptr<NodeProxy> getChildAtIndex(int32_t /*index*/) override { return nullptr; }
  std::vector<std::shared_ptr<NodeProxy>> getChildren() override { return {}; }
  int32_t getIndexInParent() override { return 0; }
  std::vector<RemoteRelation> getRelationSet() override { return {}; }
  std::shared_ptr<NodeProxy> getNeighbor(std::shared_ptr<NodeProxy> /*root*/, bool /*forward*/, NeighborSearchMode /*searchMode*/) override { return nullptr; }
  std::shared_ptr<NodeProxy> getNavigableAtPoint(int32_t /*x*/, int32_t /*y*/, CoordinateType /*type*/) override { return nullptr; }
  ReadingMaterial getReadingMaterial() override { return {}; }
  NodeInfo getNodeInfo() override { return {}; }
  DefaultLabelInfo getDefaultLabelInfo() override { return {}; }

  // --- Component interface ---
  Rect<int> getExtents(CoordinateType /*type*/) override { return {}; }
  ComponentLayer getLayer() override { return ComponentLayer::INVALID; }
  double getAlpha() override { return 1.0; }
  bool grabFocus() override { return false; }
  bool grabHighlight() override { return false; }
  bool clearHighlight() override { return false; }
  bool doGesture(const GestureInfo& /*gesture*/) override { return false; }

  // --- Action interface ---
  int32_t getActionCount() override { return 0; }
  std::string getActionName(int32_t /*index*/) override { return ""; }
  bool doActionByName(const std::string& /*name*/) override { return false; }

  // --- Value interface ---
  double getCurrentValue() override { return 0.0; }
  double getMaximumValue() override { return 0.0; }
  double getMinimumValue() override { return 0.0; }
  double getMinimumIncrement() override { return 0.0; }
  bool setCurrentValue(double /*value*/) override { return false; }

  // --- Text interface ---
  std::string getText(int32_t /*startOffset*/, int32_t /*endOffset*/) override { return ""; }
  int32_t getCharacterCount() override { return 0; }
  int32_t getCursorOffset() override { return 0; }
  Range getTextAtOffset(int32_t /*offset*/, TextBoundary /*boundary*/) override { return {}; }
  Range getRangeOfSelection(int32_t /*selectionIndex*/) override { return {}; }

  // --- Utility ---
  Address getAddress() override { return {}; }
  std::string getStringProperty(const std::string& /*propertyName*/) override { return ""; }
  std::string dumpTree(int32_t /*detailLevel*/) override { return ""; }
};

} // namespace Accessibility

#endif // ACCESSIBILITY_INTERNAL_SERVICE_TIDL_NODE_PROXY_H
