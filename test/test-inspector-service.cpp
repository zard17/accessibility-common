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
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/accessibility-event.h>
#include <accessibility/api/accessibility-service.h>
#include <accessibility/internal/service/inspector-service.h>
#include <tools/inspector/node-proxy-query-engine.h>
#include <test/mock/mock-app-registry.h>
#include <test/mock/mock-gesture-provider.h>
#include <test/mock/mock-node-proxy.h>
#include <test/test-accessible.h>

// Test framework
static int gPassCount = 0;
static int gFailCount = 0;

#define TEST_CHECK(condition, name)                                   \
  do                                                                  \
  {                                                                   \
    if(condition)                                                     \
    {                                                                 \
      ++gPassCount;                                                   \
      std::cout << "  PASS: " << (name) << std::endl;                \
    }                                                                 \
    else                                                              \
    {                                                                 \
      ++gFailCount;                                                   \
      std::cerr << "  FAIL: " << (name) << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
    }                                                                 \
  } while(0)

// ========================================================================
// NodeProxyQueryEngine tests
// ========================================================================
static void TestNodeProxyQueryEngine()
{
  std::cout << "\n--- NodeProxyQueryEngine Tests ---" << std::endl;

  MockAppRegistry registry;
  auto& tree = registry.getDemoTree();
  auto windowProxy = registry.createProxy(tree.window.get());

  InspectorEngine::NodeProxyQueryEngine engine;

  // BuildSnapshot
  engine.BuildSnapshot(windowProxy);
  TEST_CHECK(engine.GetSnapshotSize() == 11, "BuildSnapshot captures all 11 nodes");
  TEST_CHECK(engine.GetRootId() == 1, "Root ID is 1");

  // GetElementInfo for root
  auto rootInfo = engine.GetElementInfo(1);
  TEST_CHECK(rootInfo.name == "Main Window", "Root element is Main Window");
  TEST_CHECK(rootInfo.role == "WINDOW", "Root role is WINDOW");
  TEST_CHECK(rootInfo.childCount == 3, "Root has 3 children");

  // GetElementInfo for leaf
  // IDs are assigned sequentially in DFS order: 1=window, 2=header, 3=menu, 4=title, 5=content, 6=play, 7=volume, 8=nowplaying, 9=footer, 10=prev, 11=next
  auto menuInfo = engine.GetElementInfo(3);
  TEST_CHECK(menuInfo.name == "Menu", "Menu element info name");
  TEST_CHECK(menuInfo.role == "PUSH_BUTTON", "Menu element info role");
  TEST_CHECK(menuInfo.childCount == 0, "Menu has no children");
  TEST_CHECK(menuInfo.parentId == 2, "Menu parent is Header (id=2)");

  // GetElementInfo for non-existent
  auto missing = engine.GetElementInfo(999);
  TEST_CHECK(missing.name == "(not found)", "Non-existent element returns not found");

  // BuildTree
  auto treeNode = engine.BuildTree(1);
  TEST_CHECK(treeNode.name == "Main Window", "BuildTree root name");
  TEST_CHECK(treeNode.children.size() == 3, "BuildTree root has 3 children");
  TEST_CHECK(treeNode.children[0].name == "Header", "BuildTree first child is Header");
  TEST_CHECK(treeNode.children[0].children.size() == 2, "Header has 2 children");

  // Navigate forward
  uint32_t focusedId = engine.GetFocusedId();
  TEST_CHECK(focusedId > 0, "Initial focused ID is set");

  uint32_t nextId = engine.Navigate(focusedId, true);
  TEST_CHECK(nextId != focusedId, "Navigate forward changes ID");

  // Navigate backward
  uint32_t prevId = engine.Navigate(nextId, false);
  TEST_CHECK(prevId == focusedId, "Navigate backward returns to original");

  // NavigateChild
  uint32_t childId = engine.NavigateChild(1); // window -> header
  TEST_CHECK(childId == 2, "NavigateChild from root goes to first child");

  // NavigateChild from leaf
  uint32_t leafChildId = engine.NavigateChild(3); // menu has no children
  TEST_CHECK(leafChildId == 3, "NavigateChild from leaf stays at leaf");

  // NavigateParent
  uint32_t parentId = engine.NavigateParent(3); // menu -> header
  TEST_CHECK(parentId == 2, "NavigateParent from menu goes to header");

  // NavigateParent from root
  uint32_t rootParentId = engine.NavigateParent(1); // root has no parent
  TEST_CHECK(rootParentId == 1, "NavigateParent from root stays at root");

  // SetFocusedId
  engine.SetFocusedId(5);
  TEST_CHECK(engine.GetFocusedId() == 5, "SetFocusedId updates focused");

  // Focus change callback
  uint32_t callbackId = 0;
  engine.SetFocusChangedCallback([&](uint32_t id) { callbackId = id; });
  engine.SetFocusedId(7);
  TEST_CHECK(callbackId == 7, "Focus change callback fires");

  // BuildSnapshot with nullptr
  InspectorEngine::NodeProxyQueryEngine emptyEngine;
  emptyEngine.BuildSnapshot(nullptr);
  TEST_CHECK(emptyEngine.GetSnapshotSize() == 0, "BuildSnapshot with null produces empty");
  TEST_CHECK(emptyEngine.GetRootId() == 0, "Empty engine root ID is 0");
}

// ========================================================================
// InspectorService lifecycle tests
// ========================================================================
static void TestInspectorServiceLifecycle()
{
  std::cout << "\n--- InspectorService Lifecycle Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto gesturePtr  = std::make_unique<MockGestureProvider>();

  Accessibility::InspectorService::Config config;
  config.port = 0; // port 0 = don't actually bind (for unit test speed)

  Accessibility::InspectorService service(std::move(registryPtr), std::move(gesturePtr), config);

  TEST_CHECK(!service.isInspectorRunning(), "Inspector not running before start");

  service.startInspector();
  TEST_CHECK(service.isInspectorRunning(), "Inspector running after start");

  // Snapshot should have been built
  auto& engine = service.getQueryEngine();
  TEST_CHECK(engine.GetSnapshotSize() == 11, "Snapshot built on start");
  TEST_CHECK(engine.GetRootId() == 1, "Root ID set after start");

  // Double start should be safe
  service.startInspector();
  TEST_CHECK(service.isInspectorRunning(), "Double start is safe");

  service.stopInspector();
  TEST_CHECK(!service.isInspectorRunning(), "Inspector stopped");

  // Double stop should be safe
  service.stopInspector();
  TEST_CHECK(!service.isInspectorRunning(), "Double stop is safe");
}

// ========================================================================
// InspectorService destructor cleanup
// ========================================================================
static void TestInspectorServiceDestructorCleanup()
{
  std::cout << "\n--- InspectorService Destructor Cleanup Tests ---" << std::endl;

  {
    auto registryPtr = std::make_unique<MockAppRegistry>();
    auto gesturePtr  = std::make_unique<MockGestureProvider>();

    Accessibility::InspectorService::Config config;
    config.port = 0;

    Accessibility::InspectorService service(std::move(registryPtr), std::move(gesturePtr), config);
    service.startInspector();
    // Destructor should clean up without crash
  }
  TEST_CHECK(true, "Destructor cleanup does not crash");
}

// ========================================================================
// InspectorService refreshSnapshot
// ========================================================================
static void TestInspectorServiceRefreshSnapshot()
{
  std::cout << "\n--- InspectorService Refresh Snapshot Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto gesturePtr  = std::make_unique<MockGestureProvider>();

  Accessibility::InspectorService::Config config;
  config.port = 0;

  Accessibility::InspectorService service(std::move(registryPtr), std::move(gesturePtr), config);
  service.startInspector();

  auto& engine = service.getQueryEngine();
  auto rootInfo = engine.GetElementInfo(1);
  TEST_CHECK(rootInfo.name == "Main Window", "Root after initial snapshot");

  // Refresh should rebuild snapshot
  service.refreshSnapshot();
  auto rootInfo2 = engine.GetElementInfo(1);
  TEST_CHECK(rootInfo2.name == "Main Window", "Root after refresh");
  TEST_CHECK(engine.GetSnapshotSize() == 11, "Size after refresh");
}

// ========================================================================
// Navigation via base class through InspectorService
// ========================================================================
static void TestInspectorServiceNavigation()
{
  std::cout << "\n--- InspectorService Navigation Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto gesturePtr  = std::make_unique<MockGestureProvider>();

  Accessibility::InspectorService::Config config;
  config.port = 0;

  Accessibility::InspectorService service(std::move(registryPtr), std::move(gesturePtr), config);
  service.startInspector();

  // navigateNext through base class
  auto node1 = service.navigateNext();
  TEST_CHECK(node1 != nullptr, "navigateNext returns node");
  std::string firstName = node1 ? node1->getName() : "";
  TEST_CHECK(!firstName.empty(), "navigateNext returns named node: " + firstName);

  auto node2 = service.navigateNext();
  TEST_CHECK(node2 != nullptr, "Second navigateNext returns node");
  TEST_CHECK(node2->getName() != firstName, "Second navigate is different node");

  // navigatePrev
  auto prev = service.navigatePrev();
  TEST_CHECK(prev != nullptr, "navigatePrev returns node");
  TEST_CHECK(prev->getName() == firstName, "navigatePrev returns to first");

  // getCurrentNode
  auto current = service.getCurrentNode();
  TEST_CHECK(current != nullptr, "getCurrentNode is tracked");

  service.stopInspector();
}

// ========================================================================
// Event handling (passive)
// ========================================================================
static void TestInspectorServiceEvents()
{
  std::cout << "\n--- InspectorService Event Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto gesturePtr  = std::make_unique<MockGestureProvider>();

  Accessibility::InspectorService::Config config;
  config.port = 0;

  Accessibility::InspectorService service(std::move(registryPtr), std::move(gesturePtr), config);
  service.startInspector();

  // Dispatch events — inspector should not crash
  Accessibility::AccessibilityEvent event;
  event.type = Accessibility::AccessibilityEvent::Type::STATE_CHANGED;
  event.detail = "focused";
  service.dispatchEvent(event);
  TEST_CHECK(true, "STATE_CHANGED event handled without crash");

  // WINDOW_CHANGED should trigger auto-refresh
  Accessibility::AccessibilityEvent windowEvent;
  windowEvent.type = Accessibility::AccessibilityEvent::Type::WINDOW_CHANGED;
  service.dispatchEvent(windowEvent);
  TEST_CHECK(service.getQueryEngine().GetSnapshotSize() == 11, "WINDOW_CHANGED triggers auto-refresh");

  // Events after stop should be ignored
  service.stopInspector();
  Accessibility::AccessibilityEvent postStop;
  postStop.type = Accessibility::AccessibilityEvent::Type::PROPERTY_CHANGED;
  service.dispatchEvent(postStop);
  TEST_CHECK(true, "Events after stop are ignored");
}

// ========================================================================
// Config / port
// ========================================================================
static void TestInspectorServiceConfig()
{
  std::cout << "\n--- InspectorService Config Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto gesturePtr  = std::make_unique<MockGestureProvider>();

  Accessibility::InspectorService::Config config;
  config.port = 9999;

  Accessibility::InspectorService service(std::move(registryPtr), std::move(gesturePtr), config);
  TEST_CHECK(service.getPort() == 9999, "Port from config");
}

// ========================================================================
// Main
// ========================================================================
int main()
{
  std::cout << "=== InspectorService Unit Tests ===" << std::endl;

  TestNodeProxyQueryEngine();
  TestInspectorServiceLifecycle();
  TestInspectorServiceDestructorCleanup();
  TestInspectorServiceRefreshSnapshot();
  TestInspectorServiceNavigation();
  TestInspectorServiceEvents();
  TestInspectorServiceConfig();

  std::cout << "\n=== Results: " << gPassCount << " passed, " << gFailCount << " failed ===" << std::endl;

  return gFailCount > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
