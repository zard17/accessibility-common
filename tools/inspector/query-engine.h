#ifndef ACCESSIBILITY_TOOLS_INSPECTOR_QUERY_ENGINE_H
#define ACCESSIBILITY_TOOLS_INSPECTOR_QUERY_ENGINE_H

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
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility-bridge.h>
#include <accessibility/internal/bridge/dbus/dbus.h>
#include <test/test-accessible.h>

namespace InspectorEngine
{
/**
 * @brief Information about an accessible element.
 */
struct ElementInfo
{
  uint32_t    id;
  std::string name;
  std::string role;
  std::string description;
  std::string states;
  float       boundsX;
  float       boundsY;
  float       boundsWidth;
  float       boundsHeight;
  int         childCount;
  std::vector<uint32_t> childIds;
  uint32_t    parentId;
};

/**
 * @brief A node in the accessibility tree.
 */
struct TreeNode
{
  uint32_t    id;
  std::string name;
  std::string role;
  int         childCount;
  std::vector<TreeNode> children;
};

/**
 * @brief Engine that initializes the accessibility bridge and provides
 *        query methods for navigating and inspecting the accessible tree.
 */
class AccessibilityQueryEngine
{
public:
  AccessibilityQueryEngine();
  ~AccessibilityQueryEngine();

  /**
   * @brief Initializes the bridge with MockDBusWrapper and builds the demo tree.
   * @return true on success
   */
  bool Initialize();

  /**
   * @brief Shuts down the bridge.
   */
  void Shutdown();

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
   * @brief Navigates forward or backward from the current element.
   * @return The new element ID, or currentId if navigation failed
   */
  uint32_t Navigate(uint32_t currentId, bool forward);

  /**
   * @brief Navigates to the first child of the current element.
   * @return The child element ID, or currentId if no children
   */
  uint32_t NavigateChild(uint32_t currentId);

  /**
   * @brief Navigates to the parent of the current element.
   * @return The parent element ID, or currentId if already at root
   */
  uint32_t NavigateParent(uint32_t currentId);

  /**
   * @brief Converts a Role enum to its string name.
   */
  static std::string RoleToString(Accessibility::Role role);

private:
  struct DemoTree
  {
    std::shared_ptr<TestAccessible> window;
    std::shared_ptr<TestAccessible> header;
    std::shared_ptr<TestAccessible> menuBtn;
    std::shared_ptr<TestAccessible> titleLabel;
    std::shared_ptr<TestAccessible> content;
    std::shared_ptr<TestAccessible> playBtn;
    std::shared_ptr<TestAccessible> volumeSlider;
    std::shared_ptr<TestAccessible> nowPlayingLabel;
    std::shared_ptr<TestAccessible> footer;
    std::shared_ptr<TestAccessible> prevBtn;
    std::shared_ptr<TestAccessible> nextBtn;

    std::vector<std::shared_ptr<TestAccessible>> all;
  };

  DemoTree BuildDemoTree();

  std::string MakeObjectPath(uint32_t id);

  DBus::DBusClient CreateClient(uint32_t id, const std::string& iface);

  DemoTree                       mDemo;
  std::shared_ptr<Accessibility::Bridge> mBridge;
  std::string                    mBusName;
  DBusWrapper::ConnectionPtr     mConnection;
  uint32_t                       mRootId{0};
  uint32_t                       mFocusedId{0};
};

} // namespace InspectorEngine

#endif // ACCESSIBILITY_TOOLS_INSPECTOR_QUERY_ENGINE_H
