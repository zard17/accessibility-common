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
#include <accessibility/api/reading-composer.h>

namespace Accessibility
{
ReadingComposer::ReadingComposer(ReadingComposerConfig config)
: mConfig(config)
{
}

std::string ReadingComposer::composeRoleTrait(const ReadingMaterial& rm) const
{
  switch(rm.role)
  {
    case Role::PUSH_BUTTON:      return "Button";
    case Role::CHECK_BOX:        return "Check box";
    case Role::RADIO_BUTTON:     return "Radio button";
    case Role::TOGGLE_BUTTON:    return "Toggle button";
    case Role::SLIDER:           return "Slider";
    case Role::PROGRESS_BAR:     return "Progress bar";
    case Role::SPIN_BUTTON:      return "Spin button";
    case Role::ENTRY:            return "Edit field";
    case Role::PASSWORD_TEXT:    return "Password field";
    case Role::LABEL:            return "Label";
    case Role::LIST:             return "List";
    case Role::LIST_ITEM:        return "List item";
    case Role::MENU:             return "Menu";
    case Role::MENU_ITEM:        return "Menu item";
    case Role::MENU_BAR:         return "Menu bar";
    case Role::PAGE_TAB:         return "Tab";
    case Role::PAGE_TAB_LIST:    return "Tab bar";
    case Role::COMBO_BOX:        return "Combo box";
    case Role::DIALOG:           return "Dialog";
    case Role::ALERT:            return "Alert";
    case Role::POPUP_MENU:       return "Popup menu";
    case Role::TOOL_TIP:         return "Tooltip";
    case Role::TOOL_BAR:         return "Toolbar";
    case Role::STATUS_BAR:       return "Status bar";
    case Role::TABLE:            return "Table";
    case Role::TABLE_CELL:       return "Table cell";
    case Role::TREE:             return "Tree";
    case Role::TREE_ITEM:        return "Tree item";
    case Role::SCROLL_BAR:       return "Scroll bar";
    case Role::SEPARATOR:        return "Separator";
    case Role::HEADING:          return "Heading";
    case Role::LINK:             return "Link";
    case Role::IMAGE:            return "Image";
    case Role::ICON:             return "Icon";
    case Role::NOTIFICATION:     return "Notification";
    case Role::WINDOW:           return "Window";
    case Role::PANEL:            return "Panel";
    default:                     return "";
  }
}

std::string ReadingComposer::composeStateTrait(const ReadingMaterial& rm) const
{
  std::string result;

  // Checked/unchecked for checkable items
  if(rm.states[State::CHECKABLE])
  {
    if(rm.states[State::CHECKED])
    {
      result += "Checked";
    }
    else
    {
      result += "Not checked";
    }
  }

  // Selected
  if(rm.states[State::SELECTED])
  {
    if(!result.empty()) result += ", ";
    result += "Selected";
  }

  // Expanded/collapsed
  if(rm.states[State::EXPANDABLE])
  {
    if(!result.empty()) result += ", ";
    if(rm.states[State::EXPANDED])
    {
      result += "Expanded";
    }
    else
    {
      result += "Collapsed";
    }
  }

  // Disabled
  if(!rm.states[State::ENABLED])
  {
    if(!result.empty()) result += ", ";
    result += "Disabled";
  }

  // Read-only
  if(rm.states[State::READ_ONLY] && rm.states[State::EDITABLE])
  {
    if(!result.empty()) result += ", ";
    result += "Read only";
  }

  // Required
  if(rm.states[State::REQUIRED])
  {
    if(!result.empty()) result += ", ";
    result += "Required";
  }

  return result;
}

std::string ReadingComposer::composeDescriptionTrait(const ReadingMaterial& rm) const
{
  std::string result;

  // Item count for lists/popup menus (TV trait)
  if(mConfig.includeTvTraits)
  {
    if(rm.role == Role::POPUP_MENU && rm.childCount > 0)
    {
      result += std::to_string(rm.childCount) + " items";
    }

    if(rm.role == Role::PROGRESS_BAR)
    {
      result += std::to_string(static_cast<int>(rm.currentValue)) + "%";
    }
  }

  // Slider value
  if(rm.role == Role::SLIDER)
  {
    if(!result.empty()) result += ", ";
    result += rm.formattedValue.empty()
              ? std::to_string(static_cast<int>(rm.currentValue))
              : rm.formattedValue;
  }

  // Description
  if(!rm.description.empty())
  {
    if(!result.empty()) result += ", ";
    result += rm.description;
  }

  // Touch hint (suppressed on TV)
  if(!mConfig.suppressTouchHints)
  {
    if(rm.role == Role::PUSH_BUTTON || rm.role == Role::CHECK_BOX ||
       rm.role == Role::RADIO_BUTTON || rm.role == Role::TOGGLE_BUTTON ||
       rm.role == Role::LINK)
    {
      if(!result.empty()) result += ". ";
      result += "Double tap to activate";
    }
    else if(rm.role == Role::SLIDER)
    {
      if(!result.empty()) result += ". ";
      result += "Swipe up or down to adjust";
    }
  }

  return result;
}

std::string ReadingComposer::compose(const ReadingMaterial& rm) const
{
  std::string result;

  // 1. Name (priority: labeledByName > name > textIfceName)
  std::string name;
  if(!rm.labeledByName.empty())
  {
    name = rm.labeledByName;
  }
  else if(!rm.name.empty())
  {
    name = rm.name;
  }
  else if(!rm.textIfceName.empty())
  {
    name = rm.textIfceName;
  }

  if(!name.empty())
  {
    result = name;
  }

  // 2. Role trait
  auto roleTrait = composeRoleTrait(rm);
  if(!roleTrait.empty())
  {
    if(!result.empty()) result += ", ";
    result += roleTrait;
  }

  // 3. State trait
  auto stateTrait = composeStateTrait(rm);
  if(!stateTrait.empty())
  {
    if(!result.empty()) result += ", ";
    result += stateTrait;
  }

  // 4. Description trait
  auto descTrait = composeDescriptionTrait(rm);
  if(!descTrait.empty())
  {
    if(!result.empty()) result += ", ";
    result += descTrait;
  }

  return result;
}

} // namespace Accessibility
