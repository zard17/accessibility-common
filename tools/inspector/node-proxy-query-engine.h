#ifndef ACCESSIBILITY_TOOLS_INSPECTOR_NODE_PROXY_QUERY_ENGINE_H
#define ACCESSIBILITY_TOOLS_INSPECTOR_NODE_PROXY_QUERY_ENGINE_H

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
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <tools/inspector/inspector-query-interface.h>
#include <tools/inspector/inspector-types.h>

namespace Accessibility
{
class NodeProxy;
} // namespace Accessibility

namespace InspectorEngine
{
/**
 * @brief Engine that queries NodeProxy objects to build a snapshot.
 *
 * Unlike DirectQueryEngine which uses Accessible* objects directly,
 * this engine uses NodeProxy (IPC-agnostic). This allows the InspectorService
 * to work over any transport (D-Bus, TIDL, in-process).
 *
 * Usage:
 * 1. Call BuildSnapshot(root) from the main thread to capture the tree.
 * 2. Call GetElementInfo/BuildTree from any thread (reads immutable snapshot).
 */
class NodeProxyQueryEngine : public InspectorQueryInterface
{
public:
  NodeProxyQueryEngine();
  ~NodeProxyQueryEngine();

  /**
   * @brief Traverses the tree from root, building an immutable snapshot.
   *
   * Must be called from the main thread. After this call, all query methods
   * read from the cached snapshot and are thread-safe.
   *
   * @param[in] root The root NodeProxy to traverse
   */
  void BuildSnapshot(std::shared_ptr<Accessibility::NodeProxy> root);

  /**
   * @brief Returns the root element ID.
   */
  uint32_t GetRootId() const override;

  /**
   * @brief Returns the currently focused element ID.
   */
  uint32_t GetFocusedId() const override;

  /**
   * @brief Sets the currently focused element ID.
   */
  void SetFocusedId(uint32_t id) override;

  /**
   * @brief Sets a callback invoked when the focused element changes.
   */
  void SetFocusChangedCallback(std::function<void(uint32_t)> callback);

  /**
   * @brief Returns detailed information about the element with the given ID.
   */
  ElementInfo GetElementInfo(uint32_t id) override;

  /**
   * @brief Builds a tree structure starting from the given root ID.
   */
  TreeNode BuildTree(uint32_t rootId) override;

  /**
   * @brief Navigates to the next or previous highlightable element.
   */
  uint32_t Navigate(uint32_t currentId, bool forward) override;

  /**
   * @brief Navigates to the first child of the current element.
   */
  uint32_t NavigateChild(uint32_t currentId) override;

  /**
   * @brief Navigates to the parent of the current element.
   */
  uint32_t NavigateParent(uint32_t currentId) override;

  /**
   * @brief Returns the number of elements in the snapshot.
   */
  size_t GetSnapshotSize() const;

private:
  struct CachedElement
  {
    uint32_t    id;
    std::string name;
    std::string role;
    std::string description;
    std::string states;
    float       boundsX{0.0f};
    float       boundsY{0.0f};
    float       boundsWidth{0.0f};
    float       boundsHeight{0.0f};
    int         childCount{0};
    std::vector<uint32_t> childIds;
    uint32_t    parentId{0};
  };

  void TraverseTree(std::shared_ptr<Accessibility::NodeProxy> node, uint32_t parentId, uint32_t& nextId);
  void BuildHighlightableOrder(uint32_t nodeId);
  static std::string RoleToString(Accessibility::Role role);
  static std::string StatesToString(Accessibility::States states);

  mutable std::mutex                              mMutex;
  std::unordered_map<uint32_t, CachedElement>     mSnapshot;
  std::vector<uint32_t>                           mHighlightableOrder;
  std::function<void(uint32_t)>                   mFocusChangedCallback;
  uint32_t                                        mRootId{0};
  uint32_t                                        mFocusedId{0};
};

} // namespace InspectorEngine

#endif // ACCESSIBILITY_TOOLS_INSPECTOR_NODE_PROXY_QUERY_ENGINE_H
