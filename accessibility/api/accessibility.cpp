/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// EXTERNAL INCLUDES
#include <cassert>
#include <string_view>
#include <unordered_map>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility-bridge.h>
#include <accessibility/api/proxy-accessible.h>
#include <accessibility/api/accessible.h>
#include <accessibility/api/action.h>
#include <accessibility/api/application.h>
#include <accessibility/api/collection.h>
#include <accessibility/api/editable-text.h>
#include <accessibility/api/hyperlink.h>
#include <accessibility/api/hypertext.h>
#include <accessibility/api/selection.h>
#include <accessibility/api/socket.h>
#include <accessibility/api/text.h>
#include <accessibility/api/value.h>
#include <accessibility/public-api/accessibility-common.h>

namespace Accessibility
{

const std::string& Accessibility::Address::GetBus() const
{
  return mBus.empty() && Bridge::GetCurrentBridge() ? Bridge::GetCurrentBridge()->GetBusName() : mBus;
}

std::string Accessible::GetLocalizedRoleName() const
{
  return GetRoleName();
}

std::string Accessible::GetRoleName() const
{
  static const std::unordered_map<Role, std::string_view> roleMap{
    {Role::INVALID, "invalid"},
    {Role::ACCELERATOR_LABEL, "accelerator label"},
    {Role::ALERT, "alert"},
    {Role::ANIMATION, "animation"},
    {Role::ARROW, "arrow"},
    {Role::CALENDAR, "calendar"},
    {Role::CANVAS, "canvas"},
    {Role::CHECK_BOX, "check box"},
    {Role::CHECK_MENU_ITEM, "check menu item"},
    {Role::COLOR_CHOOSER, "color chooser"},
    {Role::COLUMN_HEADER, "column header"},
    {Role::COMBO_BOX, "combo box"},
    {Role::DATE_EDITOR, "date editor"},
    {Role::DESKTOP_ICON, "desktop icon"},
    {Role::DESKTOP_FRAME, "desktop frame"},
    {Role::DIAL, "dial"},
    {Role::DIALOG, "dialog"},
    {Role::DIRECTORY_PANE, "directory pane"},
    {Role::DRAWING_AREA, "drawing area"},
    {Role::FILE_CHOOSER, "file chooser"},
    {Role::FILLER, "filler"},
    {Role::FOCUS_TRAVERSABLE, "focus traversable"},
    {Role::FONT_CHOOSER, "font chooser"},
    {Role::FRAME, "frame"},
    {Role::GLASS_PANE, "glass pane"},
    {Role::HTML_CONTAINER, "html container"},
    {Role::ICON, "icon"},
    {Role::IMAGE, "image"},
    {Role::INTERNAL_FRAME, "internal frame"},
    {Role::LABEL, "label"},
    {Role::LAYERED_PANE, "layered pane"},
    {Role::LIST, "list"},
    {Role::LIST_ITEM, "list item"},
    {Role::MENU, "menu"},
    {Role::MENU_BAR, "menu bar"},
    {Role::MENU_ITEM, "menu item"},
    {Role::OPTION_PANE, "option pane"},
    {Role::PAGE_TAB, "page tab"},
    {Role::PAGE_TAB_LIST, "page tab list"},
    {Role::PANEL, "panel"},
    {Role::PASSWORD_TEXT, "password text"},
    {Role::POPUP_MENU, "popup menu"},
    {Role::PROGRESS_BAR, "progress bar"},
    {Role::PUSH_BUTTON, "push button"},
    {Role::RADIO_BUTTON, "radio button"},
    {Role::RADIO_MENU_ITEM, "radio menu item"},
    {Role::ROOT_PANE, "root pane"},
    {Role::ROW_HEADER, "row header"},
    {Role::SCROLL_BAR, "scroll bar"},
    {Role::SCROLL_PANE, "scroll pane"},
    {Role::SEPARATOR, "separator"},
    {Role::SLIDER, "slider"},
    {Role::SPIN_BUTTON, "spin button"},
    {Role::SPLIT_PANE, "split pane"},
    {Role::STATUS_BAR, "status bar"},
    {Role::TABLE, "table"},
    {Role::TABLE_CELL, "table cell"},
    {Role::TABLE_COLUMN_HEADER, "table column header"},
    {Role::TABLE_ROW_HEADER, "table row header"},
    {Role::TEAROFF_MENU_ITEM, "tearoff menu item"},
    {Role::TERMINAL, "terminal"},
    {Role::TEXT, "text"},
    {Role::TOGGLE_BUTTON, "toggle button"},
    {Role::TOOL_BAR, "tool bar"},
    {Role::TOOL_TIP, "tool tip"},
    {Role::TREE, "tree"},
    {Role::TREE_TABLE, "tree table"},
    {Role::UNKNOWN, "unknown"},
    {Role::VIEWPORT, "viewport"},
    {Role::WINDOW, "window"},
    {Role::EXTENDED, "extended"},
    {Role::HEADER, "header"},
    {Role::FOOTER, "footer"},
    {Role::PARAGRAPH, "paragraph"},
    {Role::RULER, "ruler"},
    {Role::APPLICATION, "application"},
    {Role::AUTOCOMPLETE, "autocomplete"},
    {Role::EDITBAR, "edit bar"},
    {Role::EMBEDDED, "embedded"},
    {Role::ENTRY, "entry"},
    {Role::CHART, "chart"},
    {Role::CAPTION, "caution"},
    {Role::DOCUMENT_FRAME, "document frame"},
    {Role::HEADING, "heading"},
    {Role::PAGE, "page"},
    {Role::SECTION, "section"},
    {Role::REDUNDANT_OBJECT, "redundant object"},
    {Role::FORM, "form"},
    {Role::LINK, "link"},
    {Role::INPUT_METHOD_WINDOW, "input method window"},
    {Role::TABLE_ROW, "table row"},
    {Role::TREE_ITEM, "tree item"},
    {Role::DOCUMENT_SPREADSHEET, "document spreadsheet"},
    {Role::DOCUMENT_PRESENTATION, "document presentation"},
    {Role::DOCUMENT_TEXT, "document text"},
    {Role::DOCUMENT_WEB, "document web"},
    {Role::DOCUMENT_EMAIL, "document email"},
    {Role::COMMENT, "comment"},
    {Role::LIST_BOX, "list box"},
    {Role::GROUPING, "grouping"},
    {Role::IMAGE_MAP, "image map"},
    {Role::NOTIFICATION, "notification"},
    {Role::INFO_BAR, "info bar"},
    {Role::LEVEL_BAR, "level bar"},
    {Role::TITLE_BAR, "title bar"},
    {Role::BLOCK_QUOTE, "block quote"},
    {Role::AUDIO, "audio"},
    {Role::VIDEO, "video"},
    {Role::DEFINITION, "definition"},
    {Role::ARTICLE, "article"},
    {Role::LANDMARK, "landmark"},
    {Role::LOG, "log"},
    {Role::MARQUEE, "marquee"},
    {Role::MATH, "math"},
    {Role::RATING, "rating"},
    {Role::TIMER, "timer"},
    {Role::STATIC, "static"},
    {Role::MATH_FRACTION, "math fraction"},
    {Role::MATH_ROOT, "math root"},
    {Role::SUBSCRIPT, "subscript"},
    {Role::SUPERSCRIPT, "superscript"},
  };

  auto it = roleMap.find(GetRole());

  if(it == roleMap.end())
  {
    return {};
  }

  return std::string{it->second};
}

AtspiInterfaces Accessible::GetInterfaces() const
{
  if(!mInterfaces)
  {
    mInterfaces = DoGetInterfaces();
    assert(mInterfaces); // There has to be at least AtspiInterface::ACCESSIBLE
  }

  return mInterfaces;
}

std::vector<std::string> Accessible::GetInterfacesAsStrings() const
{
  std::vector<std::string> ret;
  AtspiInterfaces          interfaces = GetInterfaces();

  for(std::size_t i = 0u; i < static_cast<std::size_t>(AtspiInterface::MAX_COUNT); ++i)
  {
    auto interface = static_cast<AtspiInterface>(i);

    if(interfaces[interface])
    {
      auto name = GetInterfaceName(interface);

      assert(!name.empty());
      ret.emplace_back(std::move(name));
    }
  }

  return ret;
}

AtspiInterfaces Accessible::DoGetInterfaces() const
{
  AtspiInterfaces interfaces;

  interfaces[AtspiInterface::ACCESSIBLE]    = true; // always true
  interfaces[AtspiInterface::ACTION]        = GetFeature<Action>() != nullptr;
  interfaces[AtspiInterface::APPLICATION]   = GetFeature<Application>() != nullptr;
  interfaces[AtspiInterface::COLLECTION]    = GetFeature<Collection>() != nullptr;
  interfaces[AtspiInterface::COMPONENT]     = true; // always true
  interfaces[AtspiInterface::EDITABLE_TEXT] = GetFeature<EditableText>() != nullptr;
  interfaces[AtspiInterface::HYPERLINK]     = GetFeature<Hyperlink>() != nullptr;
  interfaces[AtspiInterface::HYPERTEXT]     = GetFeature<Hypertext>() != nullptr;
  interfaces[AtspiInterface::SELECTION]     = GetFeature<Selection>() != nullptr;
  interfaces[AtspiInterface::SOCKET]        = GetFeature<Socket>() != nullptr;
  interfaces[AtspiInterface::TABLE]         = false;
  interfaces[AtspiInterface::TABLE_CELL]    = false;
  interfaces[AtspiInterface::TEXT]          = GetFeature<Text>() != nullptr;
  interfaces[AtspiInterface::VALUE]         = GetFeature<Value>() != nullptr;

  return interfaces;
}

std::string Accessible::GetInterfaceName(AtspiInterface interface)
{
  static const std::unordered_map<AtspiInterface, std::string_view> interfaceMap{
    {AtspiInterface::ACCESSIBLE, "org.a11y.atspi.Accessible"},
    {AtspiInterface::ACTION, "org.a11y.atspi.Action"},
    {AtspiInterface::APPLICATION, "org.a11y.atspi.Application"},
    {AtspiInterface::CACHE, "org.a11y.atspi.Cache"},
    {AtspiInterface::COLLECTION, "org.a11y.atspi.Collection"},
    {AtspiInterface::COMPONENT, "org.a11y.atspi.Component"},
    {AtspiInterface::DEVICE_EVENT_CONTROLLER, "org.a11y.atspi.DeviceEventController"},
    {AtspiInterface::DEVICE_EVENT_LISTENER, "org.a11y.atspi.DeviceEventListener"},
    {AtspiInterface::DOCUMENT, "org.a11y.atspi.Document"},
    {AtspiInterface::EDITABLE_TEXT, "org.a11y.atspi.EditableText"},
    {AtspiInterface::EVENT_DOCUMENT, "org.a11y.atspi.Event.Document"},
    {AtspiInterface::EVENT_FOCUS, "org.a11y.atspi.Event.Focus"},
    {AtspiInterface::EVENT_KEYBOARD, "org.a11y.atspi.Event.Keyboard"},
    {AtspiInterface::EVENT_MOUSE, "org.a11y.atspi.Event.Mouse"},
    {AtspiInterface::EVENT_OBJECT, "org.a11y.atspi.Event.Object"},
    {AtspiInterface::EVENT_TERMINAL, "org.a11y.atspi.Event.Terminal"},
    {AtspiInterface::EVENT_WINDOW, "org.a11y.atspi.Event.Window"},
    {AtspiInterface::HYPERLINK, "org.a11y.atspi.Hyperlink"},
    {AtspiInterface::HYPERTEXT, "org.a11y.atspi.Hypertext"},
    {AtspiInterface::IMAGE, "org.a11y.atspi.Image"},
    {AtspiInterface::REGISTRY, "org.a11y.atspi.Registry"},
    {AtspiInterface::SELECTION, "org.a11y.atspi.Selection"},
    {AtspiInterface::SOCKET, "org.a11y.atspi.Socket"},
    {AtspiInterface::TABLE, "org.a11y.atspi.Table"},
    {AtspiInterface::TABLE_CELL, "org.a11y.atspi.TableCell"},
    {AtspiInterface::TEXT, "org.a11y.atspi.Text"},
    {AtspiInterface::VALUE, "org.a11y.atspi.Value"},
  };

  auto it = interfaceMap.find(interface);

  if(it == interfaceMap.end())
  {
    return {};
  }

  return std::string{it->second};
}

Accessible* Accessible::GetCurrentlyHighlightedAccessible()
{
  return IsUp() ? Bridge::GetCurrentBridge()->mData->mCurrentlyHighlightedAccessible : nullptr;
}

void Accessible::SetCurrentlyHighlightedAccessible(Accessible* accessible)
{
  if(IsUp())
  {
    Bridge::GetCurrentBridge()->mData->mCurrentlyHighlightedAccessible = accessible;
  }
}

bool Accessible::IsHighlighted() const
{
  return this == GetCurrentlyHighlightedAccessible();
}

void Bridge::ForceDown()
{
  if(mData && mData->mCurrentlyHighlightedAccessible)
  {
    mData->mCurrentlyHighlightedAccessible->ClearHighlight();
  }
  mData = {};
}

void Bridge::SetIsOnRootLevel(Accessible* owner)
{
  owner->mIsOnRootLevel = true;
}

} //namespace Accessibility
