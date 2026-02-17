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
#include <test/mock/mock-dbus-wrapper.h>

// EXTERNAL INCLUDES
#include <cassert>
#include <iostream>

// --- Helper to cast iter ---
static std::shared_ptr<MockMessageIter> ToMock(const DBusWrapper::MessageIterPtr& it)
{
  return std::static_pointer_cast<MockMessageIter>(it);
}

static std::shared_ptr<MockMessage> ToMock(const DBusWrapper::MessagePtr& msg)
{
  return std::static_pointer_cast<MockMessage>(msg);
}

static std::shared_ptr<MockProxy> ToMock(const DBusWrapper::ProxyPtr& p)
{
  return std::static_pointer_cast<MockProxy>(p);
}

static std::shared_ptr<MockObject> ToMock(const DBusWrapper::ObjectPtr& o)
{
  return std::static_pointer_cast<MockObject>(o);
}

static std::shared_ptr<MockConnection> ToMock(const DBusWrapper::ConnectionPtr& c)
{
  return std::static_pointer_cast<MockConnection>(c);
}

// --- Helper to build signature from StoredValue ---
static char SignatureChar(const StoredValue& v)
{
  return std::visit([](auto&& arg) -> char {
    using T = std::decay_t<decltype(arg)>;
    if constexpr(std::is_same_v<T, uint8_t>)      return 'y';
    else if constexpr(std::is_same_v<T, uint16_t>) return 'q';
    else if constexpr(std::is_same_v<T, uint32_t>) return 'u';
    else if constexpr(std::is_same_v<T, uint64_t>) return 't';
    else if constexpr(std::is_same_v<T, int16_t>)  return 'n';
    else if constexpr(std::is_same_v<T, int32_t>)  return 'i';
    else if constexpr(std::is_same_v<T, int64_t>)  return 'x';
    else if constexpr(std::is_same_v<T, double>)   return 'd';
    else if constexpr(std::is_same_v<T, bool>)     return 'b';
    else if constexpr(std::is_same_v<T, std::string>) return 's';
    else if constexpr(std::is_same_v<T, ObjectPath>)  return 'o';
    else return '?';
  }, v);
}

// --- Build recursive signature for an iter ---
static std::string BuildSignature(const std::shared_ptr<MockMessageIter>& iter)
{
  std::string sig;
  for(auto& v : iter->values)
  {
    sig += SignatureChar(v);
  }
  for(auto& child : iter->children)
  {
    if(child->containerType == 'r')
    {
      sig += '(' + BuildSignature(child) + ')';
    }
    else if(child->containerType == 'a')
    {
      sig += 'a' + child->containerSig;
    }
    else if(child->containerType == 'v')
    {
      sig += 'v';
    }
    else if(child->containerType == 'e' || child->containerType == '{')
    {
      sig += '{' + BuildSignature(child) + '}';
    }
  }
  if(!iter->signature.empty())
  {
    return iter->signature;
  }
  return sig;
}

// --- MockDBusWrapper ---

MockDBusWrapper::MockDBusWrapper()
{
  auto conn = std::make_shared<MockConnection>();
  conn->uniqueName = ":mock.1";
  mConnection = conn;
  SetupCannedResponses();
}

void MockDBusWrapper::SetupCannedResponses()
{
  // org.a11y.Bus / GetAddress -> return mock address
  mCannedResponses.push_back({"/org/a11y/bus", "GetAddress",
    [this](const DBusWrapper::MessagePtr& req) -> DBusWrapper::MessagePtr {
      auto reply = std::make_shared<MockMessage>();
      reply->iter = std::make_shared<MockMessageIter>();
      reply->iter->values.push_back(std::string{"unix:path=/tmp/mock-atspi"});
      reply->iter->signature = "s";
      reply->request = ToMock(req);
      return reply;
    }});

  // org.a11y.atspi.Registry / GetRegisteredEvents -> return empty vector
  mCannedResponses.push_back({"/org/a11y/atspi/registry", "GetRegisteredEvents",
    [this](const DBusWrapper::MessagePtr& req) -> DBusWrapper::MessagePtr {
      auto reply = std::make_shared<MockMessage>();
      reply->iter = std::make_shared<MockMessageIter>();
      // Empty array - the bridge expects vector<tuple<string,string>>
      auto arrayIter = std::make_shared<MockMessageIter>();
      arrayIter->containerType = 'a';
      arrayIter->containerSig = "(ss)";
      reply->iter->children.push_back(arrayIter);
      reply->iter->signature = "a(ss)";
      reply->request = ToMock(req);
      return reply;
    }});

  // org.a11y.atspi.Socket / Embed -> return dummy parent Address
  mCannedResponses.push_back({"", "Embed",
    [this](const DBusWrapper::MessagePtr& req) -> DBusWrapper::MessagePtr {
      auto reply = std::make_shared<MockMessage>();
      reply->iter = std::make_shared<MockMessageIter>();
      // Return Address = (busName, objectPath) as a struct
      auto structIter = std::make_shared<MockMessageIter>();
      structIter->containerType = 'r';
      structIter->values.push_back(std::string{":mock.parent"});
      structIter->values.push_back(ObjectPath{"/org/a11y/atspi/accessible/mock_parent"});
      reply->iter->children.push_back(structIter);
      reply->iter->signature = "(so)";
      reply->request = ToMock(req);
      return reply;
    }});

  // org.a11y.atspi.Socket / Unembed -> no-op success
  mCannedResponses.push_back({"", "Unembed",
    [this](const DBusWrapper::MessagePtr& req) -> DBusWrapper::MessagePtr {
      auto reply = std::make_shared<MockMessage>();
      reply->iter = std::make_shared<MockMessageIter>();
      reply->request = ToMock(req);
      return reply;
    }});

  // org.freedesktop.DBus.Properties / Get -> handle property get
  mCannedResponses.push_back({"", "Get",
    [this](const DBusWrapper::MessagePtr& req) -> DBusWrapper::MessagePtr {
      // Properties.Get takes (interface_name, property_name)
      auto mockReq = ToMock(req);
      auto mockProxy = std::static_pointer_cast<MockProxy>(std::make_shared<MockProxy>());

      // Try to read interface and property name from the request
      if(mockReq->iter && mockReq->iter->values.size() >= 2)
      {
        auto* ifaceName = std::get_if<std::string>(&mockReq->iter->values[0]);
        auto* propName  = std::get_if<std::string>(&mockReq->iter->values[1]);

        if(ifaceName && propName)
        {
          // Look up property in fallback registry
          FallbackKey key{*ifaceName, *propName};
          auto it = mFallbackPropertyRegistry.find(key);
          if(it != mFallbackPropertyRegistry.end() && it->second.getCallback)
          {
            auto reply = std::make_shared<MockMessage>();
            reply->iter = std::make_shared<MockMessageIter>();
            reply->request = mockReq;

            // Create a variant container for the property value
            auto variantIter = std::make_shared<MockMessageIter>();
            variantIter->containerType = 'v';

            // Set current object path so bridge can find the right object
            std::string currentPath = mockReq->path;
            DBus::DBusServer::CurrentObjectSetter setter(mConnection, currentPath);

            auto error = it->second.getCallback(req, variantIter);
            if(error.empty())
            {
              reply->iter->children.push_back(variantIter);
              reply->iter->signature = "v";
              return reply;
            }
          }
        }
      }

      // Return error
      auto errReply = std::make_shared<MockMessage>();
      errReply->isError = true;
      errReply->errorName = "org.freedesktop.DBus.Error.UnknownProperty";
      errReply->errorText = "Property not found";
      errReply->request = mockReq;
      return errReply;
    }});

  // org.freedesktop.DBus.Properties / Set -> handle property set
  mCannedResponses.push_back({"", "Set",
    [this](const DBusWrapper::MessagePtr& req) -> DBusWrapper::MessagePtr {
      auto mockReq = ToMock(req);

      if(mockReq->iter && mockReq->iter->values.size() >= 2)
      {
        auto* ifaceName = std::get_if<std::string>(&mockReq->iter->values[0]);
        auto* propName  = std::get_if<std::string>(&mockReq->iter->values[1]);

        if(ifaceName && propName)
        {
          FallbackKey key{*ifaceName, *propName};
          auto it = mFallbackPropertyRegistry.find(key);
          if(it != mFallbackPropertyRegistry.end() && it->second.setCallback)
          {
            std::string currentPath = mockReq->path;
            DBus::DBusServer::CurrentObjectSetter setter(mConnection, currentPath);

            // Get the variant iter (third child)
            DBusWrapper::MessageIterPtr valueIter;
            if(!mockReq->iter->children.empty())
            {
              valueIter = mockReq->iter->children[0];
            }
            else
            {
              valueIter = std::make_shared<MockMessageIter>();
            }

            auto error = it->second.setCallback(req, valueIter);
            auto reply = std::make_shared<MockMessage>();
            reply->iter = std::make_shared<MockMessageIter>();
            reply->request = mockReq;
            if(!error.empty())
            {
              reply->isError = true;
              reply->errorName = "org.freedesktop.DBus.Error.Failed";
              reply->errorText = error;
            }
            return reply;
          }
        }
      }

      auto errReply = std::make_shared<MockMessage>();
      errReply->isError = true;
      errReply->errorName = "org.freedesktop.DBus.Error.UnknownProperty";
      errReply->errorText = "Property not found";
      errReply->request = ToMock(req);
      return errReply;
    }});

  // NotifyListenersSync -> key events not consumed
  mCannedResponses.push_back({"", "NotifyListenersSync",
    [this](const DBusWrapper::MessagePtr& req) -> DBusWrapper::MessagePtr {
      auto reply = std::make_shared<MockMessage>();
      reply->iter = std::make_shared<MockMessageIter>();
      reply->iter->values.push_back(false);
      reply->iter->signature = "b";
      reply->request = ToMock(req);
      return reply;
    }});

  // org.a11y.Status / IsEnabled -> false
  mCannedResponses.push_back({"", "IsEnabled",
    [this](const DBusWrapper::MessagePtr& req) -> DBusWrapper::MessagePtr {
      auto reply = std::make_shared<MockMessage>();
      reply->iter = std::make_shared<MockMessageIter>();
      // Wrap in variant for Properties.Get
      auto variantIter = std::make_shared<MockMessageIter>();
      variantIter->containerType = 'v';
      variantIter->values.push_back(false);
      reply->iter->children.push_back(variantIter);
      reply->iter->signature = "v";
      reply->request = ToMock(req);
      return reply;
    }});

  // ScreenReaderEnabled -> false
  mCannedResponses.push_back({"", "ScreenReaderEnabled",
    [this](const DBusWrapper::MessagePtr& req) -> DBusWrapper::MessagePtr {
      auto reply = std::make_shared<MockMessage>();
      reply->iter = std::make_shared<MockMessageIter>();
      auto variantIter = std::make_shared<MockMessageIter>();
      variantIter->containerType = 'v';
      variantIter->values.push_back(false);
      reply->iter->children.push_back(variantIter);
      reply->iter->signature = "v";
      reply->request = ToMock(req);
      return reply;
    }});
}

// --- Connection ---

DBusWrapper::ConnectionPtr MockDBusWrapper::eldbus_address_connection_get_impl(const std::string& addr)
{
  auto conn = std::make_shared<MockConnection>();
  conn->uniqueName = ":mock.addr." + addr;
  return conn;
}

DBusWrapper::ConnectionPtr MockDBusWrapper::eldbus_connection_get_impl(ConnectionType type)
{
  return mConnection;
}

std::string MockDBusWrapper::eldbus_connection_unique_name_get_impl(const ConnectionPtr& conn)
{
  if(auto mc = ToMock(conn))
  {
    return mc->uniqueName;
  }
  return ":mock.unknown";
}

// --- Object / Proxy ---

DBusWrapper::ObjectPtr MockDBusWrapper::eldbus_object_get_impl(const ConnectionPtr& conn, const std::string& bus, const std::string& path)
{
  auto obj = std::make_shared<MockObject>();
  obj->busName = bus;
  obj->path = path;
  return obj;
}

DBusWrapper::ProxyPtr MockDBusWrapper::eldbus_proxy_get_impl(const ObjectPtr& obj, const std::string& interface)
{
  auto mockObj = ToMock(obj);
  auto proxy = std::make_shared<MockProxy>();
  proxy->busName = mockObj->busName;
  proxy->path = mockObj->path;
  proxy->interface = interface;
  return proxy;
}

DBusWrapper::ProxyPtr MockDBusWrapper::eldbus_proxy_copy_impl(const ProxyPtr& ptr)
{
  auto orig = ToMock(ptr);
  auto copy = std::make_shared<MockProxy>();
  copy->busName = orig->busName;
  copy->path = orig->path;
  copy->interface = orig->interface;
  return copy;
}

// --- Message creation ---

DBusWrapper::MessagePtr MockDBusWrapper::eldbus_proxy_method_call_new_impl(const ProxyPtr& proxy, const std::string& funcName)
{
  auto mockProxy = ToMock(proxy);
  auto msg = std::make_shared<MockMessage>();
  msg->iter = std::make_shared<MockMessageIter>();
  msg->path = mockProxy->path;
  msg->interface = mockProxy->interface;
  msg->member = funcName;
  return msg;
}

DBusWrapper::MessagePtr MockDBusWrapper::eldbus_message_method_return_new_impl(const MessagePtr& msg)
{
  auto reply = std::make_shared<MockMessage>();
  reply->iter = std::make_shared<MockMessageIter>();
  reply->request = ToMock(msg);
  return reply;
}

DBusWrapper::MessagePtr MockDBusWrapper::eldbus_message_error_new_impl(const MessagePtr& msg, const std::string& err, const std::string& txt)
{
  auto reply = std::make_shared<MockMessage>();
  reply->iter = std::make_shared<MockMessageIter>();
  reply->isError = true;
  reply->errorName = err;
  reply->errorText = txt;
  reply->request = ToMock(msg);
  return reply;
}

DBusWrapper::MessagePtr MockDBusWrapper::eldbus_message_signal_new_impl(const std::string& path, const std::string& iface, const std::string& name)
{
  auto msg = std::make_shared<MockMessage>();
  msg->iter = std::make_shared<MockMessageIter>();
  msg->path = path;
  msg->interface = iface;
  msg->member = name;
  return msg;
}

DBusWrapper::MessagePtr MockDBusWrapper::eldbus_message_ref_impl(const MessagePtr& msg)
{
  return msg;
}

// --- Message inspection ---

bool MockDBusWrapper::eldbus_message_error_get_impl(const MessagePtr& msg, std::string& name, std::string& text)
{
  auto mockMsg = ToMock(msg);
  if(mockMsg->isError)
  {
    name = mockMsg->errorName;
    text = mockMsg->errorText;
    return true;
  }
  return false;
}

std::string MockDBusWrapper::eldbus_message_signature_get_impl(const MessagePtr& msg)
{
  auto mockMsg = ToMock(msg);
  if(mockMsg->iter)
  {
    if(!mockMsg->iter->signature.empty())
    {
      return mockMsg->iter->signature;
    }
    return BuildSignature(mockMsg->iter);
  }
  return {};
}

// --- Message iter ---

DBusWrapper::MessageIterPtr MockDBusWrapper::eldbus_message_iter_get_impl(const MessagePtr& msg, bool write)
{
  auto mockMsg = ToMock(msg);
  if(!mockMsg->iter)
  {
    mockMsg->iter = std::make_shared<MockMessageIter>();
  }
  return mockMsg->iter;
}

DBusWrapper::MessageIterPtr MockDBusWrapper::eldbus_message_iter_container_new_impl(const MessageIterPtr& it, int type, const std::string& sig)
{
  auto mockIter = ToMock(it);
  auto child = std::make_shared<MockMessageIter>();
  child->containerType = type;
  child->containerSig = sig;
  mockIter->children.push_back(child);
  return child;
}

DBusWrapper::MessageIterPtr MockDBusWrapper::eldbus_message_iter_get_and_next_by_type_impl(const MessageIterPtr& it, int type)
{
  auto mockIter = ToMock(it);
  if(mockIter->childReadCursor < mockIter->children.size())
  {
    auto& child = mockIter->children[mockIter->childReadCursor];
    if(child->containerType == type || child->containerType == 0)
    {
      mockIter->childReadCursor++;
      return child;
    }
    // Also match '{' for 'e' and vice versa (dict entry)
    if((type == '{' && child->containerType == 'e') || (type == 'e' && child->containerType == '{'))
    {
      mockIter->childReadCursor++;
      return child;
    }
  }
  return {};
}

std::string MockDBusWrapper::eldbus_message_iter_signature_get_impl(const MessageIterPtr& iter)
{
  auto mockIter = ToMock(iter);
  if(!mockIter->signature.empty())
  {
    return mockIter->signature;
  }
  return BuildSignature(mockIter);
}

// --- Basic type append/get (macro-generated) ---

#define IMPL_BASIC_TYPE(TYPE, SIG_CHAR)                                                           \
  void MockDBusWrapper::eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, TYPE src) \
  {                                                                                                \
    auto mockIter = ToMock(it);                                                                    \
    mockIter->values.push_back(src);                                                               \
    mockIter->signature += SIG_CHAR;                                                               \
  }                                                                                                \
  bool MockDBusWrapper::eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, TYPE& dst) \
  {                                                                                                \
    auto mockIter = ToMock(it);                                                                    \
    if(mockIter->readCursor >= mockIter->values.size()) return false;                              \
    auto* val = std::get_if<TYPE>(&mockIter->values[mockIter->readCursor]);                        \
    if(!val) return false;                                                                         \
    dst = *val;                                                                                    \
    mockIter->readCursor++;                                                                        \
    return true;                                                                                   \
  }

IMPL_BASIC_TYPE(uint8_t, 'y')
IMPL_BASIC_TYPE(uint16_t, 'q')
IMPL_BASIC_TYPE(uint32_t, 'u')
IMPL_BASIC_TYPE(uint64_t, 't')
IMPL_BASIC_TYPE(int16_t, 'n')
IMPL_BASIC_TYPE(int32_t, 'i')
IMPL_BASIC_TYPE(int64_t, 'x')
IMPL_BASIC_TYPE(double, 'd')
IMPL_BASIC_TYPE(bool, 'b')

#undef IMPL_BASIC_TYPE

void MockDBusWrapper::eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, const std::string& src)
{
  auto mockIter = ToMock(it);
  mockIter->values.push_back(src);
  mockIter->signature += 's';
}

bool MockDBusWrapper::eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, std::string& dst)
{
  auto mockIter = ToMock(it);
  if(mockIter->readCursor >= mockIter->values.size()) return false;
  auto* val = std::get_if<std::string>(&mockIter->values[mockIter->readCursor]);
  if(!val) return false;
  dst = *val;
  mockIter->readCursor++;
  return true;
}

void MockDBusWrapper::eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, const ObjectPath& src)
{
  auto mockIter = ToMock(it);
  mockIter->values.push_back(src);
  mockIter->signature += 'o';
}

bool MockDBusWrapper::eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, ObjectPath& dst)
{
  auto mockIter = ToMock(it);
  if(mockIter->readCursor >= mockIter->values.size()) return false;
  auto* val = std::get_if<ObjectPath>(&mockIter->values[mockIter->readCursor]);
  if(!val) return false;
  dst = *val;
  mockIter->readCursor++;
  return true;
}

// --- Send ---

DBusWrapper::MessagePtr MockDBusWrapper::RouteMethodCall(const std::string& path, const std::string& interface, const std::string& member, const MessagePtr& msg)
{
  // 1. Try exact match in method registry
  {
    InterfaceMethodKey key{path, interface, member};
    auto it = mMethodRegistry.find(key);
    if(it != mMethodRegistry.end() && it->second.callback)
    {
      DBus::DBusServer::CurrentObjectSetter setter(mConnection, path);
      return it->second.callback(msg);
    }
  }

  // 2. Try fallback match (interface, member) - matches any path
  {
    FallbackKey key{interface, member};
    auto it = mFallbackMethodRegistry.find(key);
    if(it != mFallbackMethodRegistry.end() && it->second.callback)
    {
      DBus::DBusServer::CurrentObjectSetter setter(mConnection, path);
      return it->second.callback(msg);
    }
  }

  // 3. Try canned responses
  for(auto& canned : mCannedResponses)
  {
    if(canned.member == member && (canned.path.empty() || canned.path == path))
    {
      return canned.handler(msg);
    }
  }

  // 4. Not found - return error
  auto errReply = std::make_shared<MockMessage>();
  errReply->isError = true;
  errReply->errorName = "org.freedesktop.DBus.Error.UnknownMethod";
  errReply->errorText = "Method '" + member + "' not found on path '" + path + "' interface '" + interface + "'";
  errReply->iter = std::make_shared<MockMessageIter>();
  errReply->request = ToMock(msg);
  return errReply;
}

DBusWrapper::MessagePtr MockDBusWrapper::eldbus_proxy_send_and_block_impl(const ProxyPtr& proxy, const MessagePtr& msg)
{
  auto mockProxy = ToMock(proxy);
  auto mockMsg = ToMock(msg);

  return RouteMethodCall(mockProxy->path, mockProxy->interface, mockMsg->member, msg);
}

DBusWrapper::PendingPtr MockDBusWrapper::eldbus_proxy_send_impl(const ProxyPtr& proxy, const MessagePtr& msg, const SendCallback& callback)
{
  // Async call: invoke synchronously and call callback immediately
  auto mockProxy = ToMock(proxy);
  auto mockMsg = ToMock(msg);

  auto reply = RouteMethodCall(mockProxy->path, mockProxy->interface, mockMsg->member, msg);
  if(callback)
  {
    callback(reply);
  }

  return std::make_shared<MockPending>();
}

DBusWrapper::PendingPtr MockDBusWrapper::eldbus_connection_send_impl(const ConnectionPtr& conn, const MessagePtr& msg)
{
  // Signal emission - no-op in mock (signals are not dispatched in-process)
  return std::make_shared<MockPending>();
}

// --- Proxy info ---

std::string MockDBusWrapper::eldbus_proxy_interface_get_impl(const ProxyPtr& proxy)
{
  auto mockProxy = ToMock(proxy);
  return mockProxy->interface;
}

void MockDBusWrapper::eldbus_proxy_signal_handler_add_impl(const ProxyPtr& proxy, const std::string& member, const std::function<void(const MessagePtr&)>& cb)
{
  auto mockProxy = ToMock(proxy);
  mSignalHandlers.emplace_back(mockProxy->interface, member, cb);
}

// --- Interface registration ---

void MockDBusWrapper::add_interface_impl(bool fallback, const std::string& pathName, const ConnectionPtr& connection, std::vector<std::function<void()>>& destructors, const std::string& interfaceName, std::vector<MethodInfo>& dscrMethods, std::vector<PropertyInfo>& dscrProperties, std::vector<SignalInfo>& dscrSignals)
{
  for(auto& method : dscrMethods)
  {
    if(fallback || pathName == "/")
    {
      FallbackKey key{interfaceName, method.memberName};
      mFallbackMethodRegistry[key] = method;
    }
    else
    {
      InterfaceMethodKey key{pathName, interfaceName, method.memberName};
      mMethodRegistry[key] = method;
    }
  }

  for(auto& prop : dscrProperties)
  {
    if(fallback || pathName == "/")
    {
      FallbackKey key{interfaceName, prop.memberName};
      mFallbackPropertyRegistry[key] = prop;
    }
    else
    {
      InterfacePropertyKey key{pathName, interfaceName, prop.memberName};
      mPropertyRegistry[key] = prop;
    }
  }

  // Signals: store info but don't do anything special in mock
}

void MockDBusWrapper::add_property_changed_event_listener_impl(const ProxyPtr& proxy, const std::string& interface, const std::string& name, std::function<void(const _Eina_Value*)> cb)
{
  // Store but don't fire - property change notifications from external sources don't apply in-process
}

// --- Bus name ---

void MockDBusWrapper::eldbus_name_request_impl(const ConnectionPtr& conn, const std::string& bus)
{
  // No-op in mock
}

void MockDBusWrapper::eldbus_name_release_impl(const ConnectionPtr& conn, const std::string& bus)
{
  // No-op in mock
}

