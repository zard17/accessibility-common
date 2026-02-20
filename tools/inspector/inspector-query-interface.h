#ifndef ACCESSIBILITY_TOOLS_INSPECTOR_QUERY_INTERFACE_H
#define ACCESSIBILITY_TOOLS_INSPECTOR_QUERY_INTERFACE_H

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

// INTERNAL INCLUDES
#include <tools/inspector/inspector-types.h>

namespace InspectorEngine
{
/**
 * @brief Abstract interface for querying an accessibility tree snapshot.
 *
 * Both DirectQueryEngine (Accessible*-based) and NodeProxyQueryEngine
 * (NodeProxy-based) implement this interface, allowing WebInspectorServer
 * to work with either engine type.
 */
class InspectorQueryInterface
{
public:
  virtual ~InspectorQueryInterface() = default;

  /**
   * @brief Returns the root element ID.
   */
  virtual uint32_t GetRootId() const = 0;

  /**
   * @brief Returns the currently focused element ID.
   */
  virtual uint32_t GetFocusedId() const = 0;

  /**
   * @brief Sets the currently focused element ID.
   */
  virtual void SetFocusedId(uint32_t id) = 0;

  /**
   * @brief Returns detailed information about the element with the given ID.
   */
  virtual ElementInfo GetElementInfo(uint32_t id) = 0;

  /**
   * @brief Builds a tree structure starting from the given root ID.
   */
  virtual TreeNode BuildTree(uint32_t rootId) = 0;

  /**
   * @brief Navigates to the next or previous highlightable element.
   *
   * @param[in] currentId The current element ID
   * @param[in] forward true for next, false for previous
   * @return The new element ID, or currentId if no navigation possible
   */
  virtual uint32_t Navigate(uint32_t currentId, bool forward) = 0;

  /**
   * @brief Navigates to the first child of the current element.
   *
   * @param[in] currentId The current element ID
   * @return The child element ID, or currentId if no child
   */
  virtual uint32_t NavigateChild(uint32_t currentId) = 0;

  /**
   * @brief Navigates to the parent of the current element.
   *
   * @param[in] currentId The current element ID
   * @return The parent element ID, or currentId if no parent
   */
  virtual uint32_t NavigateParent(uint32_t currentId) = 0;
};

} // namespace InspectorEngine

#endif // ACCESSIBILITY_TOOLS_INSPECTOR_QUERY_INTERFACE_H
