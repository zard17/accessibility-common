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

// INTERNAL INCLUDES
#include <tools/inspector/query-engine.h>

// EXTERNAL INCLUDES
#include <cstdio>
#include <memory>

// INTERNAL INCLUDES
#include <accessibility/api/accessibility.h>
#include <accessibility/api/accessibility-bridge.h>
#include <accessibility/internal/bridge/accessibility-common.h>
#include <accessibility/internal/bridge/bridge-platform.h>
#include <test/mock/mock-dbus-wrapper.h>

namespace InspectorEngine
{
AccessibilityQueryEngine::AccessibilityQueryEngine() = default;
AccessibilityQueryEngine::~AccessibilityQueryEngine() = default;

std::string AccessibilityQueryEngine::MakeObjectPath(uint32_t id)
{
  return std::string{ATSPI_PREFIX_PATH} + std::to_string(id);
}

DBus::DBusClient AccessibilityQueryEngine::CreateClient(uint32_t id, const std::string& iface)
{
  return DBus::DBusClient{mBusName, MakeObjectPath(id), iface, mConnection};
}

std::string AccessibilityQueryEngine::RoleToString(Accessibility::Role role)
{
  static const char* names[] = {
    "INVALID", "ACCELERATOR_LABEL", "ALERT", "ANIMATION", "ARROW", "CALENDAR",
    "CANVAS", "CHECK_BOX", "CHECK_MENU_ITEM", "COLOR_CHOOSER", "COLUMN_HEADER",
    "COMBO_BOX", "DATE_EDITOR", "DESKTOP_ICON", "DESKTOP_FRAME", "DIAL", "DIALOG",
    "DIRECTORY_PANE", "DRAWING_AREA", "FILE_CHOOSER", "FILLER", "FOCUS_TRAVERSABLE",
    "FONT_CHOOSER", "FRAME", "GLASS_PANE", "HTML_CONTAINER", "ICON", "IMAGE",
    "INTERNAL_FRAME", "LABEL", "LAYERED_PANE", "LIST", "LIST_ITEM", "MENU",
    "MENU_BAR", "MENU_ITEM", "OPTION_PANE", "PAGE_TAB", "PAGE_TAB_LIST", "PANEL",
    "PASSWORD_TEXT", "POPUP_MENU", "PROGRESS_BAR", "PUSH_BUTTON", "RADIO_BUTTON",
    "RADIO_MENU_ITEM", "ROOT_PANE", "ROW_HEADER", "SCROLL_BAR", "SCROLL_PANE",
    "SEPARATOR", "SLIDER", "SPIN_BUTTON", "SPLIT_PANE", "STATUS_BAR", "TABLE",
    "TABLE_CELL", "TABLE_COLUMN_HEADER", "TABLE_ROW_HEADER", "TEAROFF_MENU_ITEM",
    "TERMINAL", "TEXT", "TOGGLE_BUTTON", "TOOL_BAR", "TOOL_TIP", "TREE", "TREE_TABLE",
    "UNKNOWN", "VIEWPORT", "WINDOW", "EXTENDED", "HEADER", "FOOTER", "PARAGRAPH",
    "RULER", "APPLICATION", "AUTOCOMPLETE", "EDITBAR", "EMBEDDED", "ENTRY", "CHART",
    "CAPTION", "DOCUMENT_FRAME", "HEADING", "PAGE", "SECTION", "REDUNDANT_OBJECT",
    "FORM", "LINK", "INPUT_METHOD_WINDOW", "TABLE_ROW", "TREE_ITEM", "DOCUMENT_SPREADSHEET",
    "DOCUMENT_PRESENTATION", "DOCUMENT_TEXT", "DOCUMENT_WEB", "DOCUMENT_EMAIL",
    "COMMENT", "LIST_BOX", "GROUPING", "IMAGE_MAP", "NOTIFICATION", "INFO_BAR",
    "LEVEL_BAR", "TITLE_BAR", "BLOCK_QUOTE", "AUDIO", "VIDEO", "DEFINITION",
    "ARTICLE", "LANDMARK", "LOG", "MARQUEE", "MATH", "RATING", "TIMER", "STATIC",
    "MATH_FRACTION", "MATH_ROOT", "SUBSCRIPT", "SUPERSCRIPT"
  };
  auto idx = static_cast<size_t>(role);
  if(idx < sizeof(names) / sizeof(names[0]))
  {
    return names[idx];
  }
  return "ROLE_" + std::to_string(idx);
}

AccessibilityQueryEngine::DemoTree AccessibilityQueryEngine::BuildDemoTree()
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

bool AccessibilityQueryEngine::Initialize()
{
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
  callbacks.getToolkitVersion    = []() -> std::string { return "inspector-1.0.0"; };
  callbacks.getAppName           = []() -> std::string { return "MyDaliApp"; };
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
  mDemo = BuildDemoTree();

  // Step 4: Get bridge
  mBridge = Accessibility::Bridge::GetCurrentBridge();
  if(!mBridge)
  {
    fprintf(stderr, "FATAL: Bridge is null.\n");
    return false;
  }

  mBridge->SetApplicationName("MyDaliApp");
  mBridge->SetToolkitName("dali");

  for(auto& acc : mDemo.all)
  {
    mBridge->AddAccessible(acc->GetId(), acc);
  }

  mBridge->AddTopLevelWindow(mDemo.window.get());

  // Step 5: Initialize and force up
  mBridge->Initialize();
  mBridge->ApplicationResumed();

  if(!mBridge->IsUp())
  {
    fprintf(stderr, "FATAL: Bridge failed to start.\n");
    return false;
  }

  mBusName    = mBridge->GetBusName();
  mConnection = DBusWrapper::Installed()->eldbus_address_connection_get_impl("unix:path=/tmp/mock-atspi");
  mRootId     = mDemo.window->GetId();
  mFocusedId  = mDemo.menuBtn->GetId();

  return true;
}

void AccessibilityQueryEngine::Shutdown()
{
  if(mBridge)
  {
    mBridge->Terminate();
    mBridge = nullptr;
  }
}

uint32_t AccessibilityQueryEngine::GetRootId() const
{
  return mRootId;
}

uint32_t AccessibilityQueryEngine::GetFocusedId() const
{
  return mFocusedId;
}

void AccessibilityQueryEngine::SetFocusedId(uint32_t id)
{
  mFocusedId = id;
}

ElementInfo AccessibilityQueryEngine::GetElementInfo(uint32_t id)
{
  ElementInfo info{};
  info.id = id;

  auto accIface  = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
  auto compIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::COMPONENT);
  auto client    = CreateClient(id, accIface);

  // Name
  auto nameResult = client.property<std::string>("Name").get();
  if(nameResult) info.name = std::get<0>(nameResult.getValues());
  else info.name = "(unknown)";

  // Description
  auto descResult = client.property<std::string>("Description").get();
  if(descResult) info.description = std::get<0>(descResult.getValues());

  // Role
  auto roleResult = client.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
  if(roleResult) info.role = RoleToString(static_cast<Accessibility::Role>(std::get<0>(roleResult.getValues())));
  else info.role = "UNKNOWN";

  // States
  auto stateResult = client.method<DBus::ValueOrError<std::array<uint32_t, 2>>()>("GetState").call();
  if(stateResult)
  {
    auto stateData = std::get<0>(stateResult.getValues());
    Accessibility::States states{stateData};
    static const std::pair<Accessibility::State, const char*> stateNames[] = {
      {Accessibility::State::ENABLED, "ENABLED"},
      {Accessibility::State::VISIBLE, "VISIBLE"},
      {Accessibility::State::SHOWING, "SHOWING"},
      {Accessibility::State::SENSITIVE, "SENSITIVE"},
      {Accessibility::State::FOCUSABLE, "FOCUSABLE"},
      {Accessibility::State::FOCUSED, "FOCUSED"},
      {Accessibility::State::ACTIVE, "ACTIVE"},
      {Accessibility::State::CHECKED, "CHECKED"},
      {Accessibility::State::SELECTED, "SELECTED"},
      {Accessibility::State::EXPANDED, "EXPANDED"},
      {Accessibility::State::PRESSED, "PRESSED"},
      {Accessibility::State::HIGHLIGHTABLE, "HIGHLIGHTABLE"},
      {Accessibility::State::HIGHLIGHTED, "HIGHLIGHTED"},
      {Accessibility::State::EDITABLE, "EDITABLE"},
      {Accessibility::State::READ_ONLY, "READ_ONLY"},
    };
    for(auto& [state, name] : stateNames)
    {
      if(states[state])
      {
        if(!info.states.empty()) info.states += ", ";
        info.states += name;
      }
    }
  }
  if(info.states.empty()) info.states = "(none)";

  // Extents
  auto compClient = CreateClient(id, compIface);
  auto extResult  = compClient.method<DBus::ValueOrError<std::tuple<int32_t, int32_t, int32_t, int32_t>>(uint32_t)>("GetExtents")
    .call(static_cast<uint32_t>(Accessibility::CoordinateType::SCREEN));
  if(extResult)
  {
    auto extents    = std::get<0>(extResult.getValues());
    info.boundsX      = static_cast<float>(std::get<0>(extents));
    info.boundsY      = static_cast<float>(std::get<1>(extents));
    info.boundsWidth  = static_cast<float>(std::get<2>(extents));
    info.boundsHeight = static_cast<float>(std::get<3>(extents));
  }

  // Child count and child IDs
  auto ccResult = client.property<int>("ChildCount").get();
  if(ccResult) info.childCount = std::get<0>(ccResult.getValues());

  for(int i = 0; i < info.childCount; ++i)
  {
    auto childResult = client.method<DBus::ValueOrError<Accessibility::Address>(int)>("GetChildAtIndex").call(i);
    if(childResult)
    {
      auto childAddr = std::get<0>(childResult.getValues());
      try
      {
        info.childIds.push_back(static_cast<uint32_t>(std::stoul(childAddr.GetPath())));
      }
      catch(...)
      {
      }
    }
  }

  // Parent
  auto parentResult = client.property<Accessibility::Address>("Parent").get();
  if(parentResult)
  {
    auto addr = std::get<0>(parentResult.getValues());
    try
    {
      info.parentId = static_cast<uint32_t>(std::stoul(addr.GetPath()));
    }
    catch(...)
    {
      info.parentId = 0;
    }
  }

  return info;
}

TreeNode AccessibilityQueryEngine::BuildTree(uint32_t rootId)
{
  TreeNode node;
  node.id = rootId;

  auto accIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
  auto client   = CreateClient(rootId, accIface);

  // Name
  auto nameResult = client.property<std::string>("Name").get();
  if(nameResult) node.name = std::get<0>(nameResult.getValues());
  else node.name = "(unknown)";

  // Role
  auto roleResult = client.method<DBus::ValueOrError<uint32_t>()>("GetRole").call();
  if(roleResult) node.role = RoleToString(static_cast<Accessibility::Role>(std::get<0>(roleResult.getValues())));
  else node.role = "UNKNOWN";

  // Children
  auto ccResult = client.property<int>("ChildCount").get();
  if(ccResult) node.childCount = std::get<0>(ccResult.getValues());

  for(int i = 0; i < node.childCount; ++i)
  {
    auto childResult = client.method<DBus::ValueOrError<Accessibility::Address>(int)>("GetChildAtIndex").call(i);
    if(childResult)
    {
      auto childAddr = std::get<0>(childResult.getValues());
      try
      {
        uint32_t childId = static_cast<uint32_t>(std::stoul(childAddr.GetPath()));
        node.children.push_back(BuildTree(childId));
      }
      catch(...)
      {
      }
    }
  }

  return node;
}

uint32_t AccessibilityQueryEngine::Navigate(uint32_t currentId, bool forward)
{
  auto accIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
  auto client   = CreateClient(currentId, accIface);

  std::string rootPath = MakeObjectPath(mRootId);
  auto result = client.method<DBus::ValueOrError<Accessibility::Address, uint8_t>(std::string, int32_t, int32_t)>("GetNeighbor")
    .call(rootPath, forward ? 1 : 0, 1);

  if(result)
  {
    auto addr = std::get<0>(result.getValues());
    try
    {
      return static_cast<uint32_t>(std::stoul(addr.GetPath()));
    }
    catch(...)
    {
    }
  }
  return currentId;
}

uint32_t AccessibilityQueryEngine::NavigateChild(uint32_t currentId)
{
  auto accIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
  auto client   = CreateClient(currentId, accIface);

  auto ccResult = client.property<int>("ChildCount").get();
  if(ccResult && std::get<0>(ccResult.getValues()) > 0)
  {
    auto childResult = client.method<DBus::ValueOrError<Accessibility::Address>(int)>("GetChildAtIndex").call(0);
    if(childResult)
    {
      auto addr = std::get<0>(childResult.getValues());
      try
      {
        return static_cast<uint32_t>(std::stoul(addr.GetPath()));
      }
      catch(...)
      {
      }
    }
  }
  return currentId;
}

uint32_t AccessibilityQueryEngine::NavigateParent(uint32_t currentId)
{
  auto accIface = Accessibility::Accessible::GetInterfaceName(Accessibility::AtspiInterface::ACCESSIBLE);
  auto client   = CreateClient(currentId, accIface);

  auto parentResult = client.property<Accessibility::Address>("Parent").get();
  if(parentResult)
  {
    auto addr = std::get<0>(parentResult.getValues());
    try
    {
      return static_cast<uint32_t>(std::stoul(addr.GetPath()));
    }
    catch(...)
    {
    }
  }
  return currentId;
}

} // namespace InspectorEngine
