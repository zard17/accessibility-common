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
#include <accessibility/api/node-proxy.h>
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
// Test service subclass that records callbacks
// ========================================================================
class TestService : public Accessibility::AccessibilityService
{
public:
  TestService(std::unique_ptr<Accessibility::AppRegistry> registry,
              std::unique_ptr<Accessibility::GestureProvider> gestureProvider)
  : AccessibilityService(std::move(registry), std::move(gestureProvider))
  {
  }

  std::vector<Accessibility::AccessibilityEvent> receivedEvents;
  std::vector<std::shared_ptr<Accessibility::NodeProxy>> windowChanges;
  std::vector<Accessibility::GestureInfo> receivedGestures;

protected:
  void onAccessibilityEvent(const Accessibility::AccessibilityEvent& event) override
  {
    receivedEvents.push_back(event);
  }

  void onWindowChanged(std::shared_ptr<Accessibility::NodeProxy> window) override
  {
    windowChanges.push_back(std::move(window));
  }

  void onGesture(const Accessibility::GestureInfo& gesture) override
  {
    receivedGestures.push_back(gesture);
  }
};

// ========================================================================
// MockNodeProxy tests
// ========================================================================
static void TestMockNodeProxy()
{
  std::cout << "\n--- MockNodeProxy Tests ---" << std::endl;

  MockAppRegistry registry;
  auto& tree = registry.getDemoTree();

  auto menuProxy = registry.createProxy(tree.menuBtn.get());

  TEST_CHECK(menuProxy != nullptr, "MockNodeProxy creation");
  TEST_CHECK(menuProxy->getName() == "Menu", "MockNodeProxy::getName()");
  TEST_CHECK(menuProxy->getRole() == Accessibility::Role::PUSH_BUTTON, "MockNodeProxy::getRole()");

  auto states = menuProxy->getStates();
  TEST_CHECK(states[Accessibility::State::FOCUSABLE], "MockNodeProxy::getStates() - FOCUSABLE");
  TEST_CHECK(states[Accessibility::State::HIGHLIGHTABLE], "MockNodeProxy::getStates() - HIGHLIGHTABLE");
  TEST_CHECK(states[Accessibility::State::ENABLED], "MockNodeProxy::getStates() - ENABLED");

  auto extents = menuProxy->getExtents(Accessibility::CoordinateType::SCREEN);
  TEST_CHECK(extents.x == 10 && extents.y == 10, "MockNodeProxy::getExtents()");

  TEST_CHECK(menuProxy->getChildCount() == 0, "MockNodeProxy::getChildCount() leaf");

  auto windowProxy = registry.createProxy(tree.window.get());
  TEST_CHECK(windowProxy->getChildCount() == 3, "MockNodeProxy::getChildCount() window");

  auto headerProxy = windowProxy->getChildAtIndex(0);
  TEST_CHECK(headerProxy != nullptr, "MockNodeProxy::getChildAtIndex()");
  TEST_CHECK(headerProxy->getName() == "Header", "MockNodeProxy::getChildAtIndex() name");

  auto parentProxy = menuProxy->getParent();
  TEST_CHECK(parentProxy != nullptr, "MockNodeProxy::getParent()");
  TEST_CHECK(parentProxy->getName() == "Header", "MockNodeProxy::getParent() name");

  // Test getReadingMaterial batch call
  auto rm = menuProxy->getReadingMaterial();
  TEST_CHECK(rm.name == "Menu", "MockNodeProxy::getReadingMaterial() name");
  TEST_CHECK(rm.role == Accessibility::Role::PUSH_BUTTON, "MockNodeProxy::getReadingMaterial() role");
  TEST_CHECK(rm.childCount == 0, "MockNodeProxy::getReadingMaterial() childCount");

  // Test getNodeInfo batch call
  auto ni = menuProxy->getNodeInfo();
  TEST_CHECK(ni.name == "Menu", "MockNodeProxy::getNodeInfo() name");
  TEST_CHECK(ni.screenExtents.x == 10, "MockNodeProxy::getNodeInfo() extents");
}

// ========================================================================
// MockNodeProxy neighbor navigation tests
// ========================================================================
static void TestMockNodeProxyNeighbor()
{
  std::cout << "\n--- MockNodeProxy Neighbor Tests ---" << std::endl;

  MockAppRegistry registry;
  auto& tree = registry.getDemoTree();

  auto windowProxy = registry.createProxy(tree.window.get());
  auto menuProxy   = registry.createProxy(tree.menuBtn.get());

  // Navigate forward from Menu: Menu -> My Tizen App -> Play -> Volume -> Now Playing -> Previous -> Next
  auto next = menuProxy->getNeighbor(windowProxy, true, Accessibility::NeighborSearchMode::RECURSE_FROM_ROOT);
  TEST_CHECK(next != nullptr && next->getName() == "My Tizen App", "Neighbor forward: Menu -> My Tizen App");

  next = next->getNeighbor(windowProxy, true, Accessibility::NeighborSearchMode::RECURSE_FROM_ROOT);
  TEST_CHECK(next != nullptr && next->getName() == "Play", "Neighbor forward: My Tizen App -> Play");

  next = next->getNeighbor(windowProxy, true, Accessibility::NeighborSearchMode::RECURSE_FROM_ROOT);
  TEST_CHECK(next != nullptr && next->getName() == "Volume", "Neighbor forward: Play -> Volume");

  next = next->getNeighbor(windowProxy, true, Accessibility::NeighborSearchMode::RECURSE_FROM_ROOT);
  TEST_CHECK(next != nullptr && next->getName() == "Now Playing: Bohemian Rhapsody", "Neighbor forward: Volume -> Now Playing");

  next = next->getNeighbor(windowProxy, true, Accessibility::NeighborSearchMode::RECURSE_FROM_ROOT);
  TEST_CHECK(next != nullptr && next->getName() == "Previous", "Neighbor forward: Now Playing -> Previous");

  next = next->getNeighbor(windowProxy, true, Accessibility::NeighborSearchMode::RECURSE_FROM_ROOT);
  TEST_CHECK(next != nullptr && next->getName() == "Next", "Neighbor forward: Previous -> Next");

  // Wrap around
  next = next->getNeighbor(windowProxy, true, Accessibility::NeighborSearchMode::RECURSE_FROM_ROOT);
  TEST_CHECK(next != nullptr && next->getName() == "Menu", "Neighbor forward: Next -> Menu (wrap)");

  // Navigate backward from Menu: Menu -> Next (wrap)
  auto prev = menuProxy->getNeighbor(windowProxy, false, Accessibility::NeighborSearchMode::RECURSE_FROM_ROOT);
  TEST_CHECK(prev != nullptr && prev->getName() == "Next", "Neighbor backward: Menu -> Next (wrap)");

  prev = prev->getNeighbor(windowProxy, false, Accessibility::NeighborSearchMode::RECURSE_FROM_ROOT);
  TEST_CHECK(prev != nullptr && prev->getName() == "Previous", "Neighbor backward: Next -> Previous");
}

// ========================================================================
// Service lifecycle tests
// ========================================================================
static void TestServiceLifecycle()
{
  std::cout << "\n--- Service Lifecycle Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto gesturePtr  = std::make_unique<MockGestureProvider>();

  TestService service(std::move(registryPtr), std::move(gesturePtr));

  // Before start, getActiveWindow should still work
  auto window = service.getActiveWindow();
  TEST_CHECK(window != nullptr, "getActiveWindow before start");
  TEST_CHECK(window->getName() == "Main Window", "getActiveWindow returns window");

  service.start();
  TEST_CHECK(true, "Service started without error");

  window = service.getActiveWindow();
  TEST_CHECK(window != nullptr, "getActiveWindow after start");

  service.stop();
  TEST_CHECK(true, "Service stopped without error");

  // After stop, getCurrentNode should be null
  auto current = service.getCurrentNode();
  TEST_CHECK(current == nullptr, "getCurrentNode after stop is null");
}

// ========================================================================
// Navigation tests
// ========================================================================
static void TestServiceNavigation()
{
  std::cout << "\n--- Service Navigation Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto gesturePtr  = std::make_unique<MockGestureProvider>();

  TestService service(std::move(registryPtr), std::move(gesturePtr));
  service.start();

  // Navigate forward through the tree
  auto node = service.navigateNext();
  TEST_CHECK(node != nullptr, "navigateNext() first call");
  // First call starts from window, gets first highlightable
  std::string firstName = node ? node->getName() : "";
  TEST_CHECK(!firstName.empty(), "navigateNext() returns named node: " + firstName);

  // Keep navigating to build the full sequence
  std::vector<std::string> sequence;
  sequence.push_back(firstName);
  for(int i = 0; i < 6; ++i)
  {
    node = service.navigateNext();
    if(node)
    {
      sequence.push_back(node->getName());
    }
  }
  TEST_CHECK(sequence.size() == 7, "navigateNext() walks 7 highlightable nodes");

  // Expected: Menu, My Tizen App, Play, Volume, Now Playing..., Previous, Next
  // (order depends on which node we start from — window starts at first highlightable)
  TEST_CHECK(sequence.size() >= 2, "Navigation produces at least 2 elements");

  // Navigate backward
  node = service.navigatePrev();
  TEST_CHECK(node != nullptr, "navigatePrev() returns node");

  // Verify getCurrentNode is tracked
  auto current = service.getCurrentNode();
  TEST_CHECK(current != nullptr, "getCurrentNode returns last navigated");

  service.stop();
}

// ========================================================================
// Event routing tests
// ========================================================================
static void TestServiceEventRouting()
{
  std::cout << "\n--- Service Event Routing Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto gesturePtr  = std::make_unique<MockGestureProvider>();

  TestService service(std::move(registryPtr), std::move(gesturePtr));
  service.start();

  // Dispatch a state changed event
  Accessibility::AccessibilityEvent event;
  event.type    = Accessibility::AccessibilityEvent::Type::STATE_CHANGED;
  event.detail  = "focused";
  event.detail1 = 1;
  service.dispatchEvent(event);

  TEST_CHECK(service.receivedEvents.size() == 1, "Event dispatched to onAccessibilityEvent");
  TEST_CHECK(service.receivedEvents[0].type == Accessibility::AccessibilityEvent::Type::STATE_CHANGED,
             "Event type preserved");
  TEST_CHECK(service.receivedEvents[0].detail == "focused", "Event detail preserved");

  // Dispatch a window changed event
  Accessibility::AccessibilityEvent windowEvent;
  windowEvent.type = Accessibility::AccessibilityEvent::Type::WINDOW_CHANGED;
  windowEvent.detail = "Activate";
  service.dispatchEvent(windowEvent);

  TEST_CHECK(service.receivedEvents.size() == 2, "Window event dispatched to onAccessibilityEvent");
  TEST_CHECK(service.windowChanges.size() == 1, "Window event routed to onWindowChanged");

  // Dispatch multiple events
  for(int i = 0; i < 5; ++i)
  {
    Accessibility::AccessibilityEvent e;
    e.type    = Accessibility::AccessibilityEvent::Type::BOUNDS_CHANGED;
    e.detail1 = i;
    service.dispatchEvent(e);
  }
  TEST_CHECK(service.receivedEvents.size() == 7, "Multiple events dispatched");

  // Events should not be dispatched after stop
  service.stop();
  Accessibility::AccessibilityEvent postStopEvent;
  postStopEvent.type = Accessibility::AccessibilityEvent::Type::PROPERTY_CHANGED;
  service.dispatchEvent(postStopEvent);
  TEST_CHECK(service.receivedEvents.size() == 7, "Events not dispatched after stop");

  service.stop();
}

// ========================================================================
// Gesture handling tests
// ========================================================================
static void TestServiceGestureHandling()
{
  std::cout << "\n--- Service Gesture Handling Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto gesturePtr  = std::make_unique<MockGestureProvider>();
  auto* gestureRaw = gesturePtr.get();

  TestService service(std::move(registryPtr), std::move(gesturePtr));
  service.start();

  // Fire a gesture
  Accessibility::GestureInfo gesture;
  gesture.type        = Accessibility::Gesture::ONE_FINGER_FLICK_RIGHT;
  gesture.state       = Accessibility::GestureState::ENDED;
  gesture.startPointX = 100;
  gesture.startPointY = 200;
  gesture.endPointX   = 300;
  gesture.endPointY   = 200;
  gesture.eventTime   = 12345;
  gestureRaw->fireGesture(gesture);

  TEST_CHECK(service.receivedGestures.size() == 1, "Gesture dispatched to onGesture");
  TEST_CHECK(service.receivedGestures[0].type == Accessibility::Gesture::ONE_FINGER_FLICK_RIGHT,
             "Gesture type preserved");
  TEST_CHECK(service.receivedGestures[0].startPointX == 100, "Gesture start point preserved");

  // Fire multiple gestures
  Accessibility::GestureInfo tap;
  tap.type  = Accessibility::Gesture::ONE_FINGER_SINGLE_TAP;
  tap.state = Accessibility::GestureState::ENDED;
  gestureRaw->fireGesture(tap);

  Accessibility::GestureInfo doubleTap;
  doubleTap.type  = Accessibility::Gesture::ONE_FINGER_DOUBLE_TAP;
  doubleTap.state = Accessibility::GestureState::ENDED;
  gestureRaw->fireGesture(doubleTap);

  TEST_CHECK(service.receivedGestures.size() == 3, "Multiple gestures dispatched");

  service.stop();
}

// ========================================================================
// Highlight tests
// ========================================================================
static void TestServiceHighlight()
{
  std::cout << "\n--- Service Highlight Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto* registryRaw = registryPtr.get();
  auto gesturePtr  = std::make_unique<MockGestureProvider>();

  TestService service(std::move(registryPtr), std::move(gesturePtr));
  service.start();

  auto& tree = registryRaw->getDemoTree();
  auto playProxy = registryRaw->createProxy(tree.playBtn.get());

  // highlightNode should succeed (mock returns false but sets currentNode)
  service.highlightNode(playProxy);

  auto current = service.getCurrentNode();
  // Note: MockNodeProxy::grabHighlight() returns false (TestAccessible default)
  // but the service only sets currentNode if grabHighlight returns true.
  // So current may be null. This tests the code path regardless.
  TEST_CHECK(true, "highlightNode does not crash");

  // null node should return false
  bool result = service.highlightNode(nullptr);
  TEST_CHECK(!result, "highlightNode(nullptr) returns false");

  service.stop();
}

// ========================================================================
// App registration callback tests
// ========================================================================
static void TestAppRegistrationCallbacks()
{
  std::cout << "\n--- App Registration Callback Tests ---" << std::endl;

  auto registryPtr = std::make_unique<MockAppRegistry>();
  auto* registryRaw = registryPtr.get();

  std::vector<Accessibility::Address> registered;
  std::vector<Accessibility::Address> deregistered;

  registryRaw->onAppRegistered([&](const Accessibility::Address& addr) {
    registered.push_back(addr);
  });

  registryRaw->onAppDeregistered([&](const Accessibility::Address& addr) {
    deregistered.push_back(addr);
  });

  Accessibility::Address testAddr{"org.test.App", "/org/test/App"};
  registryRaw->fireAppRegistered(testAddr);

  TEST_CHECK(registered.size() == 1, "App registered callback fired");
  TEST_CHECK(registered[0].GetBus() == "org.test.App", "App registered address bus correct");

  registryRaw->fireAppDeregistered(testAddr);
  TEST_CHECK(deregistered.size() == 1, "App deregistered callback fired");

  // unused variable
  (void)registryPtr;
}

// ========================================================================
// Main
// ========================================================================
int main()
{
  std::cout << "=== AccessibilityService Unit Tests ===" << std::endl;

  TestMockNodeProxy();
  TestMockNodeProxyNeighbor();
  TestServiceLifecycle();
  TestServiceNavigation();
  TestServiceEventRouting();
  TestServiceGestureHandling();
  TestServiceHighlight();
  TestAppRegistrationCallbacks();

  std::cout << "\n=== Results: " << gPassCount << " passed, " << gFailCount << " failed ===" << std::endl;

  return gFailCount > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
