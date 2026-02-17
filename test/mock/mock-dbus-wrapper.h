#ifndef ACCESSIBILITY_TEST_MOCK_DBUS_WRAPPER_H
#define ACCESSIBILITY_TEST_MOCK_DBUS_WRAPPER_H

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
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

// INTERNAL INCLUDES
#include <accessibility/internal/bridge/dbus/dbus.h>

// Minimal _Eina_Value stub for property-changed listener (not used in-process)
struct _Eina_Value
{
  int   type;
  void* value;
};

/**
 * @brief In-memory value type for mock D-Bus serialization.
 */
using StoredValue = std::variant<
  uint8_t, uint16_t, uint32_t, uint64_t,
  int16_t, int32_t, int64_t,
  double, bool, std::string, ObjectPath>;

/**
 * @brief Mock MessageIter that stores typed values in-memory.
 */
struct MockMessageIter : DBusWrapper::MessageIter
{
  std::vector<StoredValue>                      values;
  size_t                                        readCursor{0};
  std::vector<std::shared_ptr<MockMessageIter>> children;
  size_t                                        childReadCursor{0};
  int                                           containerType{0}; // 'r', 'a', 'v', 'e', or 0
  std::string                                   containerSig;
  std::string                                   signature; // built incrementally
};

/**
 * @brief Mock Message with in-memory iter and routing metadata.
 */
struct MockMessage : DBusWrapper::Message
{
  std::shared_ptr<MockMessageIter> iter;
  std::string                      errorName;
  std::string                      errorText;
  bool                             isError{false};
  std::string                      path;
  std::string                      interface;
  std::string                      member;
  std::string                      sender;
  // Reference to original request (for method_return)
  std::shared_ptr<MockMessage>     request;
};

struct MockConnection : DBusWrapper::Connection
{
  std::string uniqueName;
};

struct MockObject : DBusWrapper::Object
{
  std::string busName;
  std::string path;
};

struct MockProxy : DBusWrapper::Proxy
{
  std::string busName;
  std::string path;
  std::string interface;
};

struct MockPending : DBusWrapper::Pending
{
};

struct MockEventPropertyChanged : DBusWrapper::EventPropertyChanged
{
};

/**
 * @brief Key for looking up registered interface methods.
 */
struct InterfaceMethodKey
{
  std::string path;
  std::string interface;
  std::string member;

  bool operator==(const InterfaceMethodKey& o) const
  {
    return path == o.path && interface == o.interface && member == o.member;
  }
};

struct InterfaceMethodKeyHash
{
  size_t operator()(const InterfaceMethodKey& k) const
  {
    auto h1 = std::hash<std::string>{}(k.path);
    auto h2 = std::hash<std::string>{}(k.interface);
    auto h3 = std::hash<std::string>{}(k.member);
    size_t v = h1;
    v = (v * 11400714819323198485llu) + h2;
    v = (v * 11400714819323198485llu) + h3;
    return v;
  }
};

/**
 * @brief Key for property get/set callbacks.
 */
struct InterfacePropertyKey
{
  std::string path;
  std::string interface;
  std::string member;

  bool operator==(const InterfacePropertyKey& o) const
  {
    return path == o.path && interface == o.interface && member == o.member;
  }
};

struct InterfacePropertyKeyHash
{
  size_t operator()(const InterfacePropertyKey& k) const
  {
    auto h1 = std::hash<std::string>{}(k.path);
    auto h2 = std::hash<std::string>{}(k.interface);
    auto h3 = std::hash<std::string>{}(k.member);
    size_t v = h1;
    v = (v * 11400714819323198485llu) + h2;
    v = (v * 11400714819323198485llu) + h3;
    return v;
  }
};

/**
 * @brief Canned response entry for external service calls during bridge init.
 */
struct CannedResponse
{
  std::string path;
  std::string member;
  // Callback that produces a reply message given the request
  std::function<DBusWrapper::MessagePtr(const DBusWrapper::MessagePtr&)> handler;
};

/**
 * @brief Mock D-Bus wrapper that exercises the full bridge pipeline in-process.
 *
 * Instead of real D-Bus IPC, this mock stores typed values in std::variant vectors
 * and routes method calls to registered interface callbacks.
 */
class MockDBusWrapper : public DBusWrapper
{
public:
  MockDBusWrapper();
  ~MockDBusWrapper() override = default;

  // --- Connection ---
  ConnectionPtr eldbus_address_connection_get_impl(const std::string& addr) override;
  ConnectionPtr eldbus_connection_get_impl(ConnectionType type) override;
  std::string   eldbus_connection_unique_name_get_impl(const ConnectionPtr& conn) override;

  // --- Object / Proxy ---
  ObjectPtr eldbus_object_get_impl(const ConnectionPtr& conn, const std::string& bus, const std::string& path) override;
  ProxyPtr  eldbus_proxy_get_impl(const ObjectPtr& obj, const std::string& interface) override;
  ProxyPtr  eldbus_proxy_copy_impl(const ProxyPtr& ptr) override;

  // --- Message creation ---
  MessagePtr eldbus_proxy_method_call_new_impl(const ProxyPtr& proxy, const std::string& funcName) override;
  MessagePtr eldbus_message_method_return_new_impl(const MessagePtr& msg) override;
  MessagePtr eldbus_message_error_new_impl(const MessagePtr& msg, const std::string& err, const std::string& txt) override;
  MessagePtr eldbus_message_signal_new_impl(const std::string& path, const std::string& iface, const std::string& name) override;
  MessagePtr eldbus_message_ref_impl(const MessagePtr& msg) override;

  // --- Message inspection ---
  bool        eldbus_message_error_get_impl(const MessagePtr& msg, std::string& name, std::string& text) override;
  std::string eldbus_message_signature_get_impl(const MessagePtr& msg) override;

  // --- Message iter ---
  MessageIterPtr eldbus_message_iter_get_impl(const MessagePtr& it, bool write) override;
  MessageIterPtr eldbus_message_iter_container_new_impl(const MessageIterPtr& it, int type, const std::string& sig) override;
  MessageIterPtr eldbus_message_iter_get_and_next_by_type_impl(const MessageIterPtr& it, int type) override;
  std::string    eldbus_message_iter_signature_get_impl(const MessageIterPtr& iter) override;

  // --- Basic type append/get ---
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, uint8_t src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, uint8_t& dst) override;
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, uint16_t src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, uint16_t& dst) override;
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, uint32_t src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, uint32_t& dst) override;
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, uint64_t src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, uint64_t& dst) override;
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, int16_t src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, int16_t& dst) override;
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, int32_t src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, int32_t& dst) override;
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, int64_t src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, int64_t& dst) override;
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, double src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, double& dst) override;
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, bool src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, bool& dst) override;
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, const std::string& src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, std::string& dst) override;
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, const ObjectPath& src) override;
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, ObjectPath& dst) override;

  // --- Send ---
  MessagePtr eldbus_proxy_send_and_block_impl(const ProxyPtr& proxy, const MessagePtr& msg) override;
  PendingPtr eldbus_proxy_send_impl(const ProxyPtr& proxy, const MessagePtr& msg, const SendCallback& callback) override;
  PendingPtr eldbus_connection_send_impl(const ConnectionPtr& conn, const MessagePtr& msg) override;

  // --- Proxy info ---
  std::string eldbus_proxy_interface_get_impl(const ProxyPtr&) override;
  void        eldbus_proxy_signal_handler_add_impl(const ProxyPtr& proxy, const std::string& member, const std::function<void(const MessagePtr&)>& cb) override;

  // --- Interface registration ---
  void add_interface_impl(bool fallback, const std::string& pathName, const ConnectionPtr& connection, std::vector<std::function<void()>>& destructors, const std::string& interfaceName, std::vector<MethodInfo>& dscrMethods, std::vector<PropertyInfo>& dscrProperties, std::vector<SignalInfo>& dscrSignals) override;
  void add_property_changed_event_listener_impl(const ProxyPtr& proxy, const std::string& interface, const std::string& name, std::function<void(const _Eina_Value*)> cb) override;

  // --- Bus name ---
  void eldbus_name_request_impl(const ConnectionPtr& conn, const std::string& bus) override;
  void eldbus_name_release_impl(const ConnectionPtr& conn, const std::string& bus) override;

private:
  /**
   * @brief Routes a method call to registered interface callbacks or canned responses.
   */
  MessagePtr RouteMethodCall(const std::string& path, const std::string& interface, const std::string& member, const MessagePtr& msg);

  /**
   * @brief Initializes canned responses for bridge init calls.
   */
  void SetupCannedResponses();

  // Interface registry: method callbacks keyed by (path, interface, member)
  std::unordered_map<InterfaceMethodKey, DBusWrapper::MethodInfo, InterfaceMethodKeyHash> mMethodRegistry;

  // Property registry: keyed by (path, interface, propertyName)
  std::unordered_map<InterfacePropertyKey, DBusWrapper::PropertyInfo, InterfacePropertyKeyHash> mPropertyRegistry;

  // Fallback interfaces: (interface, member) -> callback, matches any path
  using FallbackKey = std::pair<std::string, std::string>;
  struct FallbackKeyHash
  {
    size_t operator()(const FallbackKey& k) const
    {
      return std::hash<std::string>{}(k.first) ^ (std::hash<std::string>{}(k.second) << 32);
    }
  };
  std::unordered_map<FallbackKey, DBusWrapper::MethodInfo, FallbackKeyHash> mFallbackMethodRegistry;

  // Fallback property registry: (interface, propertyName) -> PropertyInfo
  std::unordered_map<FallbackKey, DBusWrapper::PropertyInfo, FallbackKeyHash> mFallbackPropertyRegistry;

  // Canned responses for external services
  std::vector<CannedResponse> mCannedResponses;

  // The shared connection used by the bridge
  ConnectionPtr mConnection;

  // Signal handlers (stored but not fired in mock)
  std::vector<std::tuple<std::string, std::string, std::function<void(const MessagePtr&)>>> mSignalHandlers;
};

#endif // ACCESSIBILITY_TEST_MOCK_DBUS_WRAPPER_H
