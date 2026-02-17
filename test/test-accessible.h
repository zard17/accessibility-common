#ifndef ACCESSIBILITY_TEST_TEST_ACCESSIBLE_H
#define ACCESSIBILITY_TEST_TEST_ACCESSIBLE_H

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
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/api/accessible.h>

/**
 * @brief Concrete Accessible + Component for building test trees.
 *
 * Allows configuring name, role, states, extents, and parent/child relationships.
 * Uses auto-incrementing IDs for GetAddress().
 */
class TestAccessible : public Accessibility::Accessible,
                       public std::enable_shared_from_this<TestAccessible>
{
public:
  /**
   * @brief Constructs a test accessible with the given name and role.
   */
  TestAccessible(std::string name, Accessibility::Role role);
  ~TestAccessible() override = default;

  // --- Tree building ---

  /**
   * @brief Adds a child to this accessible, setting its parent pointer.
   */
  void AddChild(std::shared_ptr<TestAccessible> child);

  // --- Configuration ---

  void SetStates(Accessibility::States states) { mStates = states; }
  void SetExtents(Accessibility::Rect<float> extents) { mExtents = extents; }

  // --- Accessible interface ---
  std::string                          GetName() const override;
  std::string                          GetDescription() const override;
  std::string                          GetValue() const override;
  Accessibility::Accessible*           GetParent() override;
  std::size_t                          GetChildCount() const override;
  std::vector<Accessibility::Accessible*> GetChildren() override;
  Accessibility::Accessible*           GetChildAtIndex(std::size_t index) override;
  std::size_t                          GetIndexInParent() override;
  Accessibility::Role                  GetRole() const override;
  Accessibility::States                GetStates() override;
  Accessibility::Attributes            GetAttributes() const override;
  bool                                 DoGesture(const Accessibility::GestureInfo& gestureInfo) override;
  std::vector<Accessibility::Relation> GetRelationSet() override;
  Accessibility::Address               GetAddress() const override;
  std::string                          GetStringProperty(std::string propertyName) const override;
  void                                 InitDefaultFeatures() override;

  // --- Component interface ---
  Accessibility::Rect<float>     GetExtents(Accessibility::CoordinateType type) const override;
  Accessibility::ComponentLayer  GetLayer() const override;
  std::int16_t                   GetMdiZOrder() const override;
  bool                           GrabFocus() override;
  double                         GetAlpha() const override;
  bool                           GrabHighlight() override;
  bool                           ClearHighlight() override;
  bool                           IsScrollable() const override;

  /**
   * @brief Returns the numeric ID used in GetAddress().
   */
  uint32_t GetId() const { return mId; }

private:
  static std::atomic<uint32_t>                sNextId;
  uint32_t                                    mId;
  std::string                                 mName;
  Accessibility::Role                         mRole;
  Accessibility::States                       mStates;
  Accessibility::Rect<float>                  mExtents{0.0f, 0.0f, 100.0f, 50.0f};
  Accessibility::Accessible*                  mParent{nullptr};
  std::vector<std::shared_ptr<TestAccessible>> mChildren;
};

#endif // ACCESSIBILITY_TEST_TEST_ACCESSIBLE_H
