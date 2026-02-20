#ifndef ACCESSIBILITY_TOOLS_SCREEN_READER_DIRECT_NODE_PROXY_H
#define ACCESSIBILITY_TOOLS_SCREEN_READER_DIRECT_NODE_PROXY_H

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
#include <functional>
#include <memory>
#include <string>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/accessible.h>
#include <accessibility/api/action.h>
#include <accessibility/api/node-proxy.h>
#include <accessibility/api/text.h>
#include <accessibility/api/value.h>

/**
 * @brief NodeProxy backed by a real Accessible* from a DALi application.
 *
 * Calls the C++ Accessible interface directly (no IPC).
 * Used for in-process screen reader demos on macOS where the bridge
 * runs in local-only mode.
 */
class DirectNodeProxy : public Accessibility::NodeProxy
{
public:
  using ProxyFactory = std::function<std::shared_ptr<DirectNodeProxy>(Accessibility::Accessible*)>;

  /**
   * @brief Constructs a DirectNodeProxy wrapping the given accessible.
   *
   * @param[in] accessible The accessible to wrap
   * @param[in] factory Factory for creating child/parent/neighbor proxies
   */
  DirectNodeProxy(Accessibility::Accessible* accessible, ProxyFactory factory)
  : mAccessible(accessible),
    mFactory(std::move(factory))
  {
  }

  Accessibility::Accessible* getAccessible() const { return mAccessible; }

  // --- Accessible interface ---

  std::string getName() override
  {
    return mAccessible ? mAccessible->GetName() : "";
  }

  std::string getDescription() override
  {
    return mAccessible ? mAccessible->GetDescription() : "";
  }

  Accessibility::Role getRole() override
  {
    return mAccessible ? mAccessible->GetRole() : Accessibility::Role::UNKNOWN;
  }

  std::string getRoleName() override
  {
    return mAccessible ? mAccessible->GetRoleName() : "";
  }

  std::string getLocalizedRoleName() override
  {
    return mAccessible ? mAccessible->GetLocalizedRoleName() : "";
  }

  Accessibility::States getStates() override
  {
    return mAccessible ? mAccessible->GetStates() : Accessibility::States{};
  }

  Accessibility::Attributes getAttributes() override
  {
    return mAccessible ? mAccessible->GetAttributes() : Accessibility::Attributes{};
  }

  std::vector<std::string> getInterfaces() override
  {
    return mAccessible ? mAccessible->GetInterfacesAsStrings() : std::vector<std::string>{};
  }

  std::shared_ptr<Accessibility::NodeProxy> getParent() override
  {
    if(!mAccessible) return nullptr;
    auto* parent = mAccessible->GetParent();
    return parent ? mFactory(parent) : nullptr;
  }

  int32_t getChildCount() override
  {
    return mAccessible ? static_cast<int32_t>(mAccessible->GetChildCount()) : 0;
  }

  std::shared_ptr<Accessibility::NodeProxy> getChildAtIndex(int32_t index) override
  {
    if(!mAccessible) return nullptr;
    auto* child = mAccessible->GetChildAtIndex(static_cast<std::size_t>(index));
    return child ? mFactory(child) : nullptr;
  }

  std::vector<std::shared_ptr<Accessibility::NodeProxy>> getChildren() override
  {
    std::vector<std::shared_ptr<Accessibility::NodeProxy>> result;
    if(!mAccessible) return result;
    auto children = mAccessible->GetChildren();
    result.reserve(children.size());
    for(auto* child : children)
    {
      result.push_back(mFactory(child));
    }
    return result;
  }

  int32_t getIndexInParent() override
  {
    return mAccessible ? static_cast<int32_t>(mAccessible->GetIndexInParent()) : 0;
  }

  std::vector<Accessibility::RemoteRelation> getRelationSet() override
  {
    return {};
  }

  std::shared_ptr<Accessibility::NodeProxy> getNeighbor(std::shared_ptr<Accessibility::NodeProxy> root, bool forward, Accessibility::NeighborSearchMode searchMode) override
  {
    if(!mAccessible) return nullptr;

    auto* rootAcc = root ? static_cast<DirectNodeProxy*>(root.get())->getAccessible() : nullptr;
    if(!rootAcc) return nullptr;

    // Build a flat list of highlightable nodes in DFS order
    std::vector<Accessibility::Accessible*> highlightable;
    std::function<void(Accessibility::Accessible*)> collect = [&](Accessibility::Accessible* node)
    {
      if(!node) return;
      auto states = node->GetStates();
      if(states[Accessibility::State::HIGHLIGHTABLE])
      {
        highlightable.push_back(node);
      }
      for(auto* child : node->GetChildren())
      {
        collect(child);
      }
    };
    collect(rootAcc);

    if(highlightable.empty()) return nullptr;

    // Find current position
    int currentIndex = -1;
    for(size_t i = 0; i < highlightable.size(); ++i)
    {
      if(highlightable[i] == mAccessible)
      {
        currentIndex = static_cast<int>(i);
        break;
      }
    }

    int nextIndex;
    if(currentIndex < 0)
    {
      nextIndex = forward ? 0 : static_cast<int>(highlightable.size()) - 1;
    }
    else if(forward)
    {
      nextIndex = (currentIndex + 1) % static_cast<int>(highlightable.size());
    }
    else
    {
      nextIndex = (currentIndex - 1 + static_cast<int>(highlightable.size())) % static_cast<int>(highlightable.size());
    }

    return mFactory(highlightable[nextIndex]);
  }

  std::shared_ptr<Accessibility::NodeProxy> getNavigableAtPoint(int32_t x, int32_t y, Accessibility::CoordinateType type) override
  {
    return nullptr;
  }

  Accessibility::ReadingMaterial getReadingMaterial() override
  {
    Accessibility::ReadingMaterial rm{};
    if(mAccessible)
    {
      rm.name          = mAccessible->GetName();
      rm.description   = mAccessible->GetDescription();
      rm.role          = mAccessible->GetRole();
      rm.states        = mAccessible->GetStates();
      rm.attributes    = mAccessible->GetAttributes();
      rm.childCount    = static_cast<int32_t>(mAccessible->GetChildCount());
      rm.indexInParent = static_cast<int32_t>(mAccessible->GetIndexInParent());

      // DALi sets CHECKED but not CHECKABLE; infer from role
      if(rm.role == Accessibility::Role::CHECK_BOX ||
         rm.role == Accessibility::Role::RADIO_BUTTON ||
         rm.role == Accessibility::Role::TOGGLE_BUTTON)
      {
        rm.states[Accessibility::State::CHECKABLE] = true;
      }
    }
    return rm;
  }

  Accessibility::NodeInfo getNodeInfo() override
  {
    Accessibility::NodeInfo info{};
    if(mAccessible)
    {
      info.name     = mAccessible->GetName();
      info.roleName = mAccessible->GetRoleName();
      info.states   = mAccessible->GetStates();
      auto ext      = mAccessible->GetExtents(Accessibility::CoordinateType::SCREEN);
      info.screenExtents = Accessibility::Rect<int>{
        static_cast<int>(ext.x), static_cast<int>(ext.y),
        static_cast<int>(ext.width), static_cast<int>(ext.height)};
    }
    return info;
  }

  Accessibility::DefaultLabelInfo getDefaultLabelInfo() override
  {
    return {};
  }

  // --- Component interface ---

  Accessibility::Rect<int> getExtents(Accessibility::CoordinateType type) override
  {
    if(!mAccessible) return {};
    auto ext = mAccessible->GetExtents(type);
    return {static_cast<int>(ext.x), static_cast<int>(ext.y),
            static_cast<int>(ext.width), static_cast<int>(ext.height)};
  }

  Accessibility::ComponentLayer getLayer() override
  {
    return mAccessible ? mAccessible->GetLayer() : Accessibility::ComponentLayer::INVALID;
  }

  double getAlpha() override
  {
    return mAccessible ? mAccessible->GetAlpha() : 1.0;
  }

  bool grabFocus() override
  {
    return mAccessible ? mAccessible->GrabFocus() : false;
  }

  bool grabHighlight() override
  {
    return mAccessible ? mAccessible->GrabHighlight() : false;
  }

  bool clearHighlight() override
  {
    return mAccessible ? mAccessible->ClearHighlight() : false;
  }

  bool doGesture(const Accessibility::GestureInfo& gesture) override
  {
    return mAccessible ? mAccessible->DoGesture(gesture) : false;
  }

  // --- Action interface ---

  int32_t getActionCount() override
  {
    if(!mAccessible) return 0;
    auto action = mAccessible->GetFeature<Accessibility::Action>();
    return action ? static_cast<int32_t>(action->GetActionCount()) : 0;
  }

  std::string getActionName(int32_t index) override
  {
    if(!mAccessible) return "";
    auto action = mAccessible->GetFeature<Accessibility::Action>();
    return action ? action->GetActionName(index) : "";
  }

  bool doActionByName(const std::string& name) override
  {
    if(!mAccessible) return false;
    auto action = mAccessible->GetFeature<Accessibility::Action>();
    return action ? action->DoAction(name) : false;
  }

  // --- Value interface ---

  double getCurrentValue() override
  {
    if(!mAccessible) return 0.0;
    auto value = mAccessible->GetFeature<Accessibility::Value>();
    return value ? value->GetCurrent() : 0.0;
  }

  double getMaximumValue() override
  {
    if(!mAccessible) return 0.0;
    auto value = mAccessible->GetFeature<Accessibility::Value>();
    return value ? value->GetMaximum() : 0.0;
  }

  double getMinimumValue() override
  {
    if(!mAccessible) return 0.0;
    auto value = mAccessible->GetFeature<Accessibility::Value>();
    return value ? value->GetMinimum() : 0.0;
  }

  double getMinimumIncrement() override
  {
    if(!mAccessible) return 0.0;
    auto value = mAccessible->GetFeature<Accessibility::Value>();
    return value ? value->GetMinimumIncrement() : 0.0;
  }

  bool setCurrentValue(double val) override
  {
    if(!mAccessible) return false;
    auto value = mAccessible->GetFeature<Accessibility::Value>();
    return value ? value->SetCurrent(val) : false;
  }

  // --- Text interface ---

  std::string getText(int32_t startOffset, int32_t endOffset) override
  {
    if(!mAccessible) return "";
    auto text = mAccessible->GetFeature<Accessibility::Text>();
    return text ? text->GetText(startOffset, endOffset) : "";
  }

  int32_t getCharacterCount() override
  {
    if(!mAccessible) return 0;
    auto text = mAccessible->GetFeature<Accessibility::Text>();
    return text ? static_cast<int32_t>(text->GetCharacterCount()) : 0;
  }

  int32_t getCursorOffset() override
  {
    if(!mAccessible) return 0;
    auto text = mAccessible->GetFeature<Accessibility::Text>();
    return text ? static_cast<int32_t>(text->GetCursorOffset()) : 0;
  }

  Accessibility::Range getTextAtOffset(int32_t offset, Accessibility::TextBoundary boundary) override
  {
    if(!mAccessible) return {};
    auto text = mAccessible->GetFeature<Accessibility::Text>();
    return text ? text->GetTextAtOffset(offset, boundary) : Accessibility::Range{};
  }

  Accessibility::Range getRangeOfSelection(int32_t selectionIndex) override
  {
    if(!mAccessible) return {};
    auto text = mAccessible->GetFeature<Accessibility::Text>();
    return text ? text->GetRangeOfSelection(selectionIndex) : Accessibility::Range{};
  }

  // --- Utility ---

  Accessibility::Address getAddress() override
  {
    return mAccessible ? mAccessible->GetAddress() : Accessibility::Address{};
  }

  std::string getStringProperty(const std::string& propertyName) override
  {
    return mAccessible ? mAccessible->GetStringProperty(propertyName) : "";
  }

  std::string dumpTree(int32_t detailLevel) override
  {
    return "";
  }

private:
  Accessibility::Accessible* mAccessible;
  ProxyFactory               mFactory;
};

#endif // ACCESSIBILITY_TOOLS_SCREEN_READER_DIRECT_NODE_PROXY_H
