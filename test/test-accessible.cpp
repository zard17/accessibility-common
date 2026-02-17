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

// CLASS HEADER
#include <test/test-accessible.h>

// EXTERNAL INCLUDES
#include <stdexcept>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility-bridge.h>

std::atomic<uint32_t> TestAccessible::sNextId{1000};

TestAccessible::TestAccessible(std::string name, Accessibility::Role role)
: mId(sNextId++),
  mName(std::move(name)),
  mRole(role)
{
}

void TestAccessible::AddChild(std::shared_ptr<TestAccessible> child)
{
  child->mParent = this;
  mChildren.push_back(std::move(child));
}

std::string TestAccessible::GetName() const
{
  return mName;
}

std::string TestAccessible::GetDescription() const
{
  return {};
}

std::string TestAccessible::GetValue() const
{
  return {};
}

Accessibility::Accessible* TestAccessible::GetParent()
{
  if(mParent)
  {
    return mParent;
  }
  // If no parent is set, return the application root from the bridge
  auto bridge = Accessibility::Bridge::GetCurrentBridge();
  if(bridge)
  {
    return bridge->GetApplication();
  }
  return nullptr;
}

std::size_t TestAccessible::GetChildCount() const
{
  return mChildren.size();
}

std::vector<Accessibility::Accessible*> TestAccessible::GetChildren()
{
  std::vector<Accessibility::Accessible*> result;
  result.reserve(mChildren.size());
  for(auto& child : mChildren)
  {
    result.push_back(child.get());
  }
  return result;
}

Accessibility::Accessible* TestAccessible::GetChildAtIndex(std::size_t index)
{
  if(index >= mChildren.size())
  {
    throw std::domain_error{"invalid index " + std::to_string(index) + " for object with " + std::to_string(mChildren.size()) + " children"};
  }
  return mChildren[index].get();
}

std::size_t TestAccessible::GetIndexInParent()
{
  if(!mParent)
  {
    return 0;
  }

  auto siblings = mParent->GetChildren();
  for(std::size_t i = 0; i < siblings.size(); ++i)
  {
    if(siblings[i] == this)
    {
      return i;
    }
  }

  throw std::domain_error{"object not found in parent's children"};
}

Accessibility::Role TestAccessible::GetRole() const
{
  return mRole;
}

Accessibility::States TestAccessible::GetStates()
{
  return mStates;
}

Accessibility::Attributes TestAccessible::GetAttributes() const
{
  return {};
}

bool TestAccessible::DoGesture(const Accessibility::GestureInfo& gestureInfo)
{
  return false;
}

std::vector<Accessibility::Relation> TestAccessible::GetRelationSet()
{
  return {};
}

Accessibility::Address TestAccessible::GetAddress() const
{
  auto bridge = Accessibility::Bridge::GetCurrentBridge();
  if(bridge)
  {
    return {bridge->GetBusName(), std::to_string(mId)};
  }
  return {"", std::to_string(mId)};
}

std::string TestAccessible::GetStringProperty(std::string propertyName) const
{
  return {};
}

void TestAccessible::InitDefaultFeatures()
{
  // No extra features (Action, Value, etc.) for basic test nodes
}

// --- Component interface ---

Accessibility::Rect<float> TestAccessible::GetExtents(Accessibility::CoordinateType type) const
{
  return mExtents;
}

Accessibility::ComponentLayer TestAccessible::GetLayer() const
{
  return Accessibility::ComponentLayer::WIDGET;
}

std::int16_t TestAccessible::GetMdiZOrder() const
{
  return 0;
}

bool TestAccessible::GrabFocus()
{
  return false;
}

double TestAccessible::GetAlpha() const
{
  return 1.0;
}

bool TestAccessible::GrabHighlight()
{
  return false;
}

bool TestAccessible::ClearHighlight()
{
  return false;
}

bool TestAccessible::IsScrollable() const
{
  return false;
}
