#ifndef ACCESSIBILITY_TEST_MOCK_APP_REGISTRY_H
#define ACCESSIBILITY_TEST_MOCK_APP_REGISTRY_H

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
#include <memory>
#include <unordered_map>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/app-registry.h>
#include <test/mock/mock-node-proxy.h>
#include <test/test-accessible.h>

/**
 * @brief Mock AppRegistry that builds a demo tree and returns MockNodeProxy instances.
 *
 * Uses the same demo tree structure as the inspector's BuildDemoTree().
 */
class MockAppRegistry : public Accessibility::AppRegistry
{
public:
  /**
   * @brief Demo tree holding shared_ptrs to all TestAccessible nodes.
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

  MockAppRegistry()
  {
    buildDemoTree();
  }

  std::shared_ptr<Accessibility::NodeProxy> getDesktop() override
  {
    return createProxy(mTree.window.get());
  }

  std::shared_ptr<Accessibility::NodeProxy> getActiveWindow() override
  {
    return createProxy(mTree.window.get());
  }

  void onAppRegistered(Accessibility::AppCallback callback) override
  {
    mRegisteredCallbacks.push_back(std::move(callback));
  }

  void onAppDeregistered(Accessibility::AppCallback callback) override
  {
    mDeregisteredCallbacks.push_back(std::move(callback));
  }

  /**
   * @brief Gets the demo tree for test assertions.
   */
  const DemoTree& getDemoTree() const { return mTree; }

  /**
   * @brief Creates a MockNodeProxy for the given accessible.
   */
  std::shared_ptr<MockNodeProxy> createProxy(Accessibility::Accessible* acc)
  {
    if(!acc) return nullptr;
    return std::make_shared<MockNodeProxy>(acc, [this](Accessibility::Accessible* a) -> std::shared_ptr<MockNodeProxy>
    {
      return createProxy(a);
    });
  }

  /**
   * @brief Fires app registered callbacks for testing.
   */
  void fireAppRegistered(const Accessibility::Address& addr)
  {
    for(auto& cb : mRegisteredCallbacks)
    {
      cb(addr);
    }
  }

  /**
   * @brief Fires app deregistered callbacks for testing.
   */
  void fireAppDeregistered(const Accessibility::Address& addr)
  {
    for(auto& cb : mDeregisteredCallbacks)
    {
      cb(addr);
    }
  }

private:
  void buildDemoTree()
  {
    auto makeStates = [](bool focusable = false, bool active = false, bool highlightable = false)
    {
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

    mTree.window = std::make_shared<TestAccessible>("Main Window", Accessibility::Role::WINDOW);
    mTree.window->SetStates(makeStates(false, true));
    mTree.window->SetExtents({0.0f, 0.0f, 480.0f, 800.0f});

    mTree.header = std::make_shared<TestAccessible>("Header", Accessibility::Role::PANEL);
    mTree.header->SetStates(makeStates());
    mTree.header->SetExtents({0.0f, 0.0f, 480.0f, 60.0f});

    mTree.menuBtn = std::make_shared<TestAccessible>("Menu", Accessibility::Role::PUSH_BUTTON);
    mTree.menuBtn->SetStates(makeStates(true));
    mTree.menuBtn->SetExtents({10.0f, 10.0f, 40.0f, 40.0f});

    mTree.titleLabel = std::make_shared<TestAccessible>("My Tizen App", Accessibility::Role::LABEL);
    mTree.titleLabel->SetStates(makeStates(false, false, true));
    mTree.titleLabel->SetExtents({60.0f, 10.0f, 360.0f, 40.0f});

    mTree.content = std::make_shared<TestAccessible>("Content", Accessibility::Role::PANEL);
    mTree.content->SetStates(makeStates());
    mTree.content->SetExtents({0.0f, 60.0f, 480.0f, 680.0f});

    mTree.playBtn = std::make_shared<TestAccessible>("Play", Accessibility::Role::PUSH_BUTTON);
    mTree.playBtn->SetStates(makeStates(true));
    mTree.playBtn->SetExtents({200.0f, 300.0f, 80.0f, 80.0f});

    mTree.volumeSlider = std::make_shared<TestAccessible>("Volume", Accessibility::Role::SLIDER);
    mTree.volumeSlider->SetStates(makeStates(true));
    mTree.volumeSlider->SetExtents({40.0f, 420.0f, 400.0f, 40.0f});

    mTree.nowPlayingLabel = std::make_shared<TestAccessible>("Now Playing: Bohemian Rhapsody", Accessibility::Role::LABEL);
    mTree.nowPlayingLabel->SetStates(makeStates(false, false, true));
    mTree.nowPlayingLabel->SetExtents({40.0f, 480.0f, 400.0f, 30.0f});

    mTree.footer = std::make_shared<TestAccessible>("Footer", Accessibility::Role::PANEL);
    mTree.footer->SetStates(makeStates());
    mTree.footer->SetExtents({0.0f, 740.0f, 480.0f, 60.0f});

    mTree.prevBtn = std::make_shared<TestAccessible>("Previous", Accessibility::Role::PUSH_BUTTON);
    mTree.prevBtn->SetStates(makeStates(true));
    mTree.prevBtn->SetExtents({100.0f, 750.0f, 80.0f, 40.0f});

    mTree.nextBtn = std::make_shared<TestAccessible>("Next", Accessibility::Role::PUSH_BUTTON);
    mTree.nextBtn->SetStates(makeStates(true));
    mTree.nextBtn->SetExtents({300.0f, 750.0f, 80.0f, 40.0f});

    mTree.header->AddChild(mTree.menuBtn);
    mTree.header->AddChild(mTree.titleLabel);
    mTree.content->AddChild(mTree.playBtn);
    mTree.content->AddChild(mTree.volumeSlider);
    mTree.content->AddChild(mTree.nowPlayingLabel);
    mTree.footer->AddChild(mTree.prevBtn);
    mTree.footer->AddChild(mTree.nextBtn);
    mTree.window->AddChild(mTree.header);
    mTree.window->AddChild(mTree.content);
    mTree.window->AddChild(mTree.footer);

    mTree.all = {mTree.window, mTree.header, mTree.menuBtn, mTree.titleLabel,
                 mTree.content, mTree.playBtn, mTree.volumeSlider, mTree.nowPlayingLabel,
                 mTree.footer, mTree.prevBtn, mTree.nextBtn};
  }

  DemoTree mTree;
  std::vector<Accessibility::AppCallback> mRegisteredCallbacks;
  std::vector<Accessibility::AppCallback> mDeregisteredCallbacks;
};

#endif // ACCESSIBILITY_TEST_MOCK_APP_REGISTRY_H
