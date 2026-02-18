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
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <memory>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/accessibility-bridge.h>
#include <accessibility/internal/bridge/bridge-platform.h>
#include <test/mock/mock-dbus-wrapper.h>
#include <test/test-accessible.h>
#include <tools/inspector/direct-query-engine.h>
#include <tools/inspector/web-inspector-server.h>

namespace
{
volatile std::sig_atomic_t gRunning = 1;

void SignalHandler(int)
{
  gRunning = 0;
}

/**
 * @brief Builds the same demo tree used by other inspectors.
 */
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

DemoTree BuildDemoTree()
{
  DemoTree t;

  auto makeStates = [](bool focusable = false, bool active = false, bool highlightable = false) {
    Accessibility::States s;
    s[Accessibility::State::ENABLED]   = true;
    s[Accessibility::State::VISIBLE]   = true;
    s[Accessibility::State::SHOWING]   = true;
    s[Accessibility::State::SENSITIVE] = true;
    if(focusable)
    {
      s[Accessibility::State::FOCUSABLE]     = true;
      s[Accessibility::State::HIGHLIGHTABLE] = true;
    }
    else if(highlightable)
    {
      s[Accessibility::State::HIGHLIGHTABLE] = true;
    }
    if(active) s[Accessibility::State::ACTIVE] = true;
    return s;
  };

  t.window = std::make_shared<TestAccessible>("Main Window", Accessibility::Role::WINDOW);
  t.window->SetStates(makeStates(false, true));
  t.window->SetExtents({0.0f, 0.0f, 480.0f, 800.0f});

  t.header = std::make_shared<TestAccessible>("Header", Accessibility::Role::PANEL);
  t.header->SetStates(makeStates());
  t.header->SetExtents({0.0f, 0.0f, 480.0f, 60.0f});

  t.menuBtn = std::make_shared<TestAccessible>("Menu", Accessibility::Role::PUSH_BUTTON);
  t.menuBtn->SetStates(makeStates(true));
  t.menuBtn->SetExtents({10.0f, 10.0f, 40.0f, 40.0f});

  t.titleLabel = std::make_shared<TestAccessible>("My Tizen App", Accessibility::Role::LABEL);
  t.titleLabel->SetStates(makeStates(false, false, true));
  t.titleLabel->SetExtents({60.0f, 10.0f, 360.0f, 40.0f});

  t.content = std::make_shared<TestAccessible>("Content", Accessibility::Role::PANEL);
  t.content->SetStates(makeStates());
  t.content->SetExtents({0.0f, 60.0f, 480.0f, 680.0f});

  t.playBtn = std::make_shared<TestAccessible>("Play", Accessibility::Role::PUSH_BUTTON);
  t.playBtn->SetStates(makeStates(true));
  t.playBtn->SetExtents({200.0f, 300.0f, 80.0f, 80.0f});

  t.volumeSlider = std::make_shared<TestAccessible>("Volume", Accessibility::Role::SLIDER);
  t.volumeSlider->SetStates(makeStates(true));
  t.volumeSlider->SetExtents({40.0f, 420.0f, 400.0f, 40.0f});

  t.nowPlayingLabel = std::make_shared<TestAccessible>("Now Playing: Bohemian Rhapsody", Accessibility::Role::LABEL);
  t.nowPlayingLabel->SetStates(makeStates(false, false, true));
  t.nowPlayingLabel->SetExtents({40.0f, 480.0f, 400.0f, 30.0f});

  t.footer = std::make_shared<TestAccessible>("Footer", Accessibility::Role::PANEL);
  t.footer->SetStates(makeStates());
  t.footer->SetExtents({0.0f, 740.0f, 480.0f, 60.0f});

  t.prevBtn = std::make_shared<TestAccessible>("Previous", Accessibility::Role::PUSH_BUTTON);
  t.prevBtn->SetStates(makeStates(true));
  t.prevBtn->SetExtents({100.0f, 750.0f, 80.0f, 40.0f});

  t.nextBtn = std::make_shared<TestAccessible>("Next", Accessibility::Role::PUSH_BUTTON);
  t.nextBtn->SetStates(makeStates(true));
  t.nextBtn->SetExtents({300.0f, 750.0f, 80.0f, 40.0f});

  t.header->AddChild(t.menuBtn);
  t.header->AddChild(t.titleLabel);
  t.content->AddChild(t.playBtn);
  t.content->AddChild(t.volumeSlider);
  t.content->AddChild(t.nowPlayingLabel);
  t.footer->AddChild(t.prevBtn);
  t.footer->AddChild(t.nextBtn);
  t.window->AddChild(t.header);
  t.window->AddChild(t.content);
  t.window->AddChild(t.footer);

  t.all = {t.window, t.header, t.menuBtn, t.titleLabel, t.content,
           t.playBtn, t.volumeSlider, t.nowPlayingLabel, t.footer,
           t.prevBtn, t.nextBtn};

  return t;
}

} // anonymous namespace

int main(int argc, char** argv)
{
  int port = 8080;
  if(argc > 1)
  {
    port = std::atoi(argv[1]);
    if(port <= 0 || port > 65535)
    {
      fprintf(stderr, "Invalid port: %s\n", argv[1]);
      return 1;
    }
  }

  printf("=== Direct Web Accessibility Inspector ===\n\n");

  // Step 1: Install MockDBusWrapper
  auto mockWrapper = std::make_unique<MockDBusWrapper>();
  DBusWrapper::Install(std::move(mockWrapper));

  // Step 2: Set PlatformCallbacks
  Accessibility::PlatformCallbacks callbacks;
  callbacks.addIdle = [](std::function<bool()> cb) -> uint32_t {
    if(cb) cb();
    return 1;
  };
  callbacks.removeIdle           = [](uint32_t) {};
  callbacks.getToolkitVersion    = []() -> std::string { return "inspector-direct-1.0.0"; };
  callbacks.getAppName           = []() -> std::string { return "DirectInspector"; };
  callbacks.isAdaptorAvailable   = []() -> bool { return true; };
  callbacks.onEnableAutoInit     = []() {};
  callbacks.createTimer          = [](uint32_t, std::function<bool()> cb) -> uint32_t {
    if(cb) cb();
    return 1;
  };
  callbacks.cancelTimer          = [](uint32_t) {};
  callbacks.isTimerRunning       = [](uint32_t) -> bool { return false; };
  Accessibility::SetPlatformCallbacks(callbacks);

  // Step 3: Build demo tree
  auto demo = BuildDemoTree();

  // Step 4: Get bridge and register accessible objects
  auto bridge = Accessibility::Bridge::GetCurrentBridge();
  if(!bridge)
  {
    fprintf(stderr, "FATAL: Bridge is null.\n");
    return 1;
  }

  bridge->SetApplicationName("DirectInspector");
  bridge->SetToolkitName("dali");

  for(auto& acc : demo.all)
  {
    bridge->AddAccessible(acc->GetId(), acc);
  }

  bridge->AddTopLevelWindow(demo.window.get());
  bridge->Initialize();
  bridge->ApplicationResumed();

  // Step 5: Build snapshot using DirectQueryEngine
  InspectorEngine::DirectQueryEngine engine;
  engine.BuildSnapshot(demo.window.get());

  printf("Snapshot built: root=%u, %zu elements\n", engine.GetRootId(), demo.all.size());

  // Step 6: Start web inspector server
  InspectorEngine::WebInspectorServer server;
  server.Start(engine, port);

  printf("Web inspector: http://localhost:%d\n", port);
  printf("Press Ctrl+C to stop.\n\n");

  // Step 7: Wait for signal
  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);
  while(gRunning)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  printf("\nShutting down...\n");
  server.Stop();
  bridge->Terminate();

  return 0;
}
