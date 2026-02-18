#ifndef ACCESSIBILITY_TOOLS_INSPECTOR_DIRECT_QUERY_ENGINE_H
#define ACCESSIBILITY_TOOLS_INSPECTOR_DIRECT_QUERY_ENGINE_H

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
#include <mutex>
#include <string>
#include <unordered_map>

// INTERNAL INCLUDES
#include <tools/inspector/inspector-types.h>

namespace Accessibility
{
class Accessible;
enum class Role : uint32_t;
} // namespace Accessibility

namespace InspectorEngine
{
/**
 * @brief Engine that queries Accessible objects directly via their C++ interface.
 *
 * Unlike AccessibilityQueryEngine which routes through D-Bus, this engine
 * calls GetName(), GetRole(), GetStates(), etc. on Accessible* objects directly.
 * This works on any platform without requiring a D-Bus daemon.
 *
 * Usage:
 * 1. Call BuildSnapshot(root) from the main thread to capture the tree.
 * 2. Call GetElementInfo/BuildTree from any thread (reads immutable snapshot).
 */
class DirectQueryEngine
{
public:
  DirectQueryEngine();
  ~DirectQueryEngine();

  /**
   * @brief Traverses the tree from root, building an immutable snapshot.
   *
   * Must be called from the main thread. After this call, all query methods
   * read from the cached snapshot and are thread-safe.
   *
   * @param[in] root The root accessible to traverse
   */
  void BuildSnapshot(Accessibility::Accessible* root);

  /**
   * @brief Returns the root element ID.
   */
  uint32_t GetRootId() const;

  /**
   * @brief Returns the currently focused element ID.
   */
  uint32_t GetFocusedId() const;

  /**
   * @brief Sets the currently focused element ID.
   */
  void SetFocusedId(uint32_t id);

  /**
   * @brief Returns detailed information about the element with the given ID.
   */
  ElementInfo GetElementInfo(uint32_t id);

  /**
   * @brief Builds a tree structure starting from the given root ID.
   */
  TreeNode BuildTree(uint32_t rootId);

  /**
   * @brief Converts a Role enum to its string name.
   */
  static std::string RoleToString(Accessibility::Role role);

  /**
   * @brief Converts States bitset to a comma-separated string.
   */
  static std::string StatesToString(Accessibility::Accessible* accessible);

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

  void TraverseTree(Accessibility::Accessible* node, uint32_t parentId);

  static uint32_t ExtractId(Accessibility::Accessible* accessible);

  std::unordered_map<uint32_t, CachedElement> mSnapshot;
  uint32_t                                    mRootId{0};
  uint32_t                                    mFocusedId{0};
};

} // namespace InspectorEngine

#endif // ACCESSIBILITY_TOOLS_INSPECTOR_DIRECT_QUERY_ENGINE_H
