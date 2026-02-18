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

/**
 * @brief GDBus (GLib/GIO) D-Bus backend for standard Linux desktops.
 *
 * Implements the DBusWrapper virtual interface using GLib's GDBus API,
 * enabling D-Bus accessibility on Linux without EFL/Eldbus dependency.
 * This backend is selected when gio-2.0 is available but eldbus is not.
 */

// CLASS HEADER
#include <accessibility/internal/bridge/accessibility-common.h>
#include <accessibility/internal/bridge/dbus/dbus.h>

// EXTERNAL INCLUDES
#include <gio/gio.h>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stack>

// INTERNAL INCLUDES
#include <accessibility/public-api/accessibility-common.h>

#define DBUS_INTERFACE_PROPERTIES "org.freedesktop.DBus.Properties"

std::atomic<unsigned int>                       DBus::detail::CallId::LastId{0};
static std::function<void(const char*, size_t)> debugPrinter;
static std::mutex                               debugLock;

thread_local std::string                DBus::DBusServer::currentObjectPath;
thread_local DBusWrapper::ConnectionPtr DBus::DBusServer::currentConnection;

void DBus::setDebugPrinter(std::function<void(const char*, size_t)> printer)
{
  std::lock_guard<std::mutex> lock(debugLock);
  debugPrinter = std::move(printer);
}

void DBus::debugPrint(const char* file, size_t line, const char* format, ...)
{
  std::function<void(const char*, size_t)> debugPrintFunc;
  {
    std::lock_guard<std::mutex> lock(debugLock);
    if(!debugPrinter)
      return;
    debugPrintFunc = debugPrinter;
  }
  std::vector<char> buf(4096);
  int               offset;
  while(true)
  {
    offset = snprintf(buf.data(), buf.size(), "%s:%u: ", file, static_cast<unsigned int>(line));
    if(offset < 0)
      return;
    if(static_cast<size_t>(offset) < buf.size())
      break;
    buf.resize(offset + 1);
  }

  while(true)
  {
    va_list args;
    va_start(args, format);
    int z = vsnprintf(buf.data() + offset, buf.size() - offset, format, args);
    va_end(args);
    if(z < 0)
      return;
    bool done = static_cast<size_t>(z) + static_cast<size_t>(offset) < buf.size();
    buf.resize(static_cast<size_t>(z) + static_cast<size_t>(offset));
    if(done)
      break;
  }
  debugPrintFunc(buf.data(), buf.size());
}

DBusWrapper::ConnectionPtr DBus::getDBusConnectionByName(const std::string& name)
{
  return DBUS_W->eldbus_address_connection_get_impl(name);
}

DBusWrapper::ConnectionPtr DBus::getDBusConnectionByType(ConnectionType connectionType)
{
  return DBUS_W->eldbus_connection_get_impl(connectionType);
}

DBus::DBusClient::DBusClient(std::string busName, std::string pathName, std::string interfaceName, ConnectionType tp)
: DBusClient(std::move(busName), std::move(pathName), std::move(interfaceName), getDBusConnectionByType(tp))
{
}

DBus::DBusClient::DBusClient(std::string busName, std::string pathName, std::string interfaceName, const DBusWrapper::ConnectionPtr& conn)
{
  if(!conn)
    connectionState->connection = getDBusConnectionByType(ConnectionType::SESSION);
  else
    connectionState->connection = conn;

  if(!connectionState->connection)
  {
    ACCESSIBILITY_LOG_ERROR("DBusClient connection is not ready\n");
    return;
  }

  connectionState->object = DBUS_W->eldbus_object_get_impl(connectionState->connection, busName.c_str(), pathName.c_str());
  if(connectionState->object)
  {
    connectionState->proxy = DBUS_W->eldbus_proxy_get_impl(connectionState->object, interfaceName);
    if(interfaceName != DBUS_INTERFACE_PROPERTIES)
    {
      connectionState->propertiesProxy = DBUS_W->eldbus_proxy_get_impl(connectionState->object, DBUS_INTERFACE_PROPERTIES);
    }
    else
    {
      connectionState->propertiesProxy = DBUS_W->eldbus_proxy_copy_impl(connectionState->proxy);
    }
  }
  connectionInfo                = std::make_shared<ConnectionInfo>();
  connectionInfo->busName       = std::move(busName);
  connectionInfo->pathName      = std::move(pathName);
  connectionInfo->interfaceName = std::move(interfaceName);
}

DBus::DBusServer::DBusServer(ConnectionType tp)
: DBus::DBusServer(DBus::getDBusConnectionByType(tp))
{
}

DBus::DBusServer::DBusServer(const DBusWrapper::ConnectionPtr& conn)
{
  if(!conn)
    connection = getDBusConnectionByType(ConnectionType::SESSION);
  else
    connection = conn;
}

DBus::DBusInterfaceDescription::DBusInterfaceDescription(std::string interfaceName)
: Ipc::InterfaceDescription(std::move(interfaceName))
{
}

void DBus::DBusServer::addInterface(const std::string& pathName, DBusInterfaceDescription& dscr, bool fallback)
{
  DBUS_W->add_interface_impl(fallback, pathName, connection, destructorObject->destructors, dscr.mInterfaceName, dscr.methods, dscr.properties, dscr.signals);
}

std::string DBus::DBusServer::getBusName() const
{
  return getConnectionName(connection);
}

DBusWrapper::ConnectionPtr DBus::DBusServer::getConnection()
{
  return connection;
}

std::string DBus::getConnectionName(const DBusWrapper::ConnectionPtr& c)
{
  return DBUS_W->eldbus_connection_unique_name_get_impl(c);
}

void DBus::requestBusName(const DBusWrapper::ConnectionPtr& conn, const std::string& bus)
{
  DBUS_W->eldbus_name_request_impl(conn, bus);
}

void DBus::releaseBusName(const DBusWrapper::ConnectionPtr& conn, const std::string& bus)
{
  DBUS_W->eldbus_name_release_impl(conn, bus);
}

struct _Eina_Value
{
  int   type;
  void* value;
};

bool DBus::DBusClient::getFromEinaValue(const _Eina_Value* v, void* dst)
{
  if(!v || !v->value || !dst)
    return false;
  // Only bool type (type==1) is used by addPropertyChangedEvent<bool>
  if(v->type == 1)
  {
    *static_cast<bool*>(dst) = *static_cast<bool*>(v->value);
    return true;
  }
  return false;
}

static std::unique_ptr<DBusWrapper> InstalledWrapper;

// =============================================================================
// GDBusWrapper — GLib/GIO D-Bus backend
// =============================================================================

struct GDBusWrapper : public DBusWrapper
{
  static constexpr int GDBUS_CALL_TIMEOUT_MS = 1000;

  GDBusWrapper()
  {
  }

  ~GDBusWrapper()
  {
  }

  // ---------------------------------------------------------------------------
  // Wrapper types
  // ---------------------------------------------------------------------------

  struct ConnectionImpl : public Connection
  {
    GDBusConnection* conn       = nullptr;
    bool             eraseOnExit = false;

    ConnectionImpl(GDBusConnection* c, bool erase = false)
    : conn(c),
      eraseOnExit(erase)
    {
    }

    ~ConnectionImpl()
    {
      if(eraseOnExit && conn)
      {
        g_object_unref(conn);
      }
    }
  };

  struct ObjectImpl : public Object
  {
    std::string busName;
    std::string path;
    std::weak_ptr<ConnectionImpl> connection;

    ObjectImpl(const std::string& bus, const std::string& p, std::shared_ptr<ConnectionImpl> c)
    : busName(bus),
      path(p),
      connection(c)
    {
    }
  };

  struct ProxyImpl : public Proxy
  {
    GDBusProxy*  proxy = nullptr;
    std::string  busName;
    std::string  path;
    std::string  interface;
    std::weak_ptr<ConnectionImpl> connection;
    bool         eraseOnExit = false;

    ProxyImpl(GDBusProxy* p, const std::string& bus, const std::string& pth, const std::string& iface, std::shared_ptr<ConnectionImpl> c, bool erase = false)
    : proxy(p),
      busName(bus),
      path(pth),
      interface(iface),
      connection(c),
      eraseOnExit(erase)
    {
    }

    ~ProxyImpl()
    {
      if(eraseOnExit && proxy)
      {
        g_object_unref(proxy);
      }
    }
  };

  struct MessageIterImpl : public MessageIter
  {
    // Write mode: stack of builders for constructing GVariant
    GVariantBuilder* builder = nullptr;
    std::vector<GVariantBuilder*> builderStack; // sub-containers

    // Read mode: GVariant for traversal
    GVariant* variant    = nullptr;
    gsize     readCursor = 0;
    gsize     numChildren = 0;
    bool      ownsVariant = false;
    bool      ownsBuilder = false;

    // Signature tracking for write mode
    std::string writtenSignature;

    MessageIterImpl() = default;

    ~MessageIterImpl()
    {
      if(ownsVariant && variant)
      {
        g_variant_unref(variant);
      }
      if(ownsBuilder && builder)
      {
        g_variant_builder_unref(builder);
      }
      for(auto* b : builderStack)
      {
        g_variant_builder_unref(b);
      }
    }
  };

  struct MessageImpl : public Message
  {
    GVariant*   body       = nullptr;
    bool        ownsBody   = false;
    GError*     error      = nullptr;
    std::string signature;
    std::string path;
    std::string interface;
    std::string member;
    std::string destination;

    // For write mode (building a method call message)
    GVariantBuilder* bodyBuilder = nullptr;
    std::string      bodyTypeString;

    // For reply construction
    std::shared_ptr<MessageImpl> requestMsg;

    MessageImpl() = default;

    ~MessageImpl()
    {
      if(ownsBody && body)
      {
        g_variant_unref(body);
      }
      if(error)
      {
        g_error_free(error);
      }
      if(bodyBuilder)
      {
        g_variant_builder_unref(bodyBuilder);
      }
    }
  };

  struct PendingImpl : public Pending
  {
  };

  struct EventPropertyChangedImpl : public EventPropertyChanged
  {
  };

  // ---------------------------------------------------------------------------
  // Helper casts
  // ---------------------------------------------------------------------------

  static ConnectionImpl* getConn(const ConnectionPtr& c)
  {
    return static_cast<ConnectionImpl*>(c.get());
  }

  static ObjectImpl* getObj(const ObjectPtr& o)
  {
    return static_cast<ObjectImpl*>(o.get());
  }

  static ProxyImpl* getProxy(const ProxyPtr& p)
  {
    return static_cast<ProxyImpl*>(p.get());
  }

  static MessageImpl* getMsg(const MessagePtr& m)
  {
    return static_cast<MessageImpl*>(m.get());
  }

  static MessageIterImpl* getIter(const MessageIterPtr& it)
  {
    return static_cast<MessageIterImpl*>(it.get());
  }

  // ---------------------------------------------------------------------------
  // D-Bus type character mapping
  // ---------------------------------------------------------------------------

  static const char* gvariantTypeForDBusSig(char sig)
  {
    switch(sig)
    {
      case 'y': return "y";
      case 'q': return "q";
      case 'u': return "u";
      case 't': return "t";
      case 'n': return "n";
      case 'i': return "i";
      case 'x': return "x";
      case 'd': return "d";
      case 'b': return "b";
      case 's': return "s";
      case 'o': return "o";
      case 'v': return "v";
      default: return nullptr;
    }
  }

  // ---------------------------------------------------------------------------
  // Connection management
  // ---------------------------------------------------------------------------

  ConnectionPtr eldbus_address_connection_get_impl(const std::string& addr) override
  {
    GError* err = nullptr;
    auto*   conn = g_dbus_connection_new_for_address_sync(
      addr.c_str(),
      static_cast<GDBusConnectionFlags>(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
      nullptr,
      nullptr,
      &err);

    if(err)
    {
      ACCESSIBILITY_LOG_ERROR("g_dbus_connection_new_for_address_sync failed: %s\n", err->message);
      g_error_free(err);
      return {};
    }

    return std::make_shared<ConnectionImpl>(conn, true);
  }

  ConnectionPtr eldbus_connection_get_impl(ConnectionType type) override
  {
    GBusType busType = (type == ConnectionType::SYSTEM) ? G_BUS_TYPE_SYSTEM : G_BUS_TYPE_SESSION;
    GError*  err     = nullptr;
    auto*    conn    = g_bus_get_sync(busType, nullptr, &err);

    if(err)
    {
      ACCESSIBILITY_LOG_ERROR("g_bus_get_sync failed: %s\n", err->message);
      g_error_free(err);
      return {};
    }

    return std::make_shared<ConnectionImpl>(conn, true);
  }

  std::string eldbus_connection_unique_name_get_impl(const ConnectionPtr& conn) override
  {
    auto* c = getConn(conn);
    if(!c || !c->conn)
      return {};
    const char* name = g_dbus_connection_get_unique_name(c->conn);
    return name ? name : "";
  }

  // ---------------------------------------------------------------------------
  // Object / Proxy
  // ---------------------------------------------------------------------------

  ObjectPtr eldbus_object_get_impl(const ConnectionPtr& conn, const std::string& bus, const std::string& path) override
  {
    auto connImpl = std::static_pointer_cast<ConnectionImpl>(conn);
    return std::make_shared<ObjectImpl>(bus, path, connImpl);
  }

  ProxyPtr eldbus_proxy_get_impl(const ObjectPtr& obj, const std::string& interface) override
  {
    auto* o = getObj(obj);
    auto  connImpl = o->connection.lock();
    if(!connImpl || !connImpl->conn)
      return {};

    GError*    err   = nullptr;
    GDBusProxy* proxy = g_dbus_proxy_new_sync(
      connImpl->conn,
      G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
      nullptr,
      o->busName.c_str(),
      o->path.c_str(),
      interface.c_str(),
      nullptr,
      &err);

    if(err)
    {
      ACCESSIBILITY_LOG_ERROR("g_dbus_proxy_new_sync failed: %s\n", err->message);
      g_error_free(err);
      return {};
    }

    return std::make_shared<ProxyImpl>(proxy, o->busName, o->path, interface, connImpl, true);
  }

  ProxyPtr eldbus_proxy_copy_impl(const ProxyPtr& ptr) override
  {
    auto* p = getProxy(ptr);
    if(!p)
      return {};

    auto connImpl = p->connection.lock();
    if(p->proxy)
    {
      g_object_ref(p->proxy);
    }
    return std::make_shared<ProxyImpl>(p->proxy, p->busName, p->path, p->interface, connImpl, true);
  }

  std::string eldbus_proxy_interface_get_impl(const ProxyPtr& proxy) override
  {
    auto* p = getProxy(proxy);
    if(!p)
      return {};
    return p->interface;
  }

  // ---------------------------------------------------------------------------
  // Bus name management
  // ---------------------------------------------------------------------------

  void eldbus_name_request_impl(const ConnectionPtr& conn, const std::string& bus) override
  {
    auto* c = getConn(conn);
    if(!c || !c->conn)
      return;

    GError*  err    = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
      c->conn,
      "org.freedesktop.DBus",
      "/org/freedesktop/DBus",
      "org.freedesktop.DBus",
      "RequestName",
      g_variant_new("(su)", bus.c_str(), 0x4u /* DBUS_NAME_FLAG_DO_NOT_QUEUE */),
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      GDBUS_CALL_TIMEOUT_MS,
      nullptr,
      &err);

    if(err)
    {
      ACCESSIBILITY_LOG_ERROR("RequestName failed: %s\n", err->message);
      g_error_free(err);
      return;
    }
    if(result)
    {
      g_variant_unref(result);
    }
  }

  void eldbus_name_release_impl(const ConnectionPtr& conn, const std::string& bus) override
  {
    auto* c = getConn(conn);
    if(!c || !c->conn)
      return;

    GError*  err    = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
      c->conn,
      "org.freedesktop.DBus",
      "/org/freedesktop/DBus",
      "org.freedesktop.DBus",
      "ReleaseName",
      g_variant_new("(s)", bus.c_str()),
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      GDBUS_CALL_TIMEOUT_MS,
      nullptr,
      &err);

    if(err)
    {
      g_error_free(err);
    }
    if(result)
    {
      g_variant_unref(result);
    }
  }

  // ---------------------------------------------------------------------------
  // Basic type serialization (append / get)
  // ---------------------------------------------------------------------------

#define GDBUS_IMPL_BASIC_APPEND_GET(TYPE, FMT, GTYPE)                                                 \
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, TYPE src) override          \
  {                                                                                                    \
    auto* iter = getIter(it);                                                                          \
    if(iter->builder)                                                                                  \
    {                                                                                                  \
      g_variant_builder_add(iter->builder, FMT, static_cast<GTYPE>(src));                              \
    }                                                                                                  \
  }                                                                                                    \
  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, TYPE& dst) override             \
  {                                                                                                    \
    auto* iter = getIter(it);                                                                          \
    if(!iter->variant)                                                                                 \
      return false;                                                                                    \
    if(iter->readCursor >= iter->numChildren)                                                          \
      return false;                                                                                    \
    GVariant* child = g_variant_get_child_value(iter->variant, iter->readCursor);                      \
    if(!child)                                                                                         \
      return false;                                                                                    \
    GTYPE val;                                                                                         \
    g_variant_get(child, FMT, &val);                                                                   \
    dst = static_cast<TYPE>(val);                                                                      \
    g_variant_unref(child);                                                                            \
    iter->readCursor++;                                                                                \
    return true;                                                                                       \
  }

  // clang-format off
  GDBUS_IMPL_BASIC_APPEND_GET(uint8_t,  "y", guchar)
  GDBUS_IMPL_BASIC_APPEND_GET(uint16_t, "q", guint16)
  GDBUS_IMPL_BASIC_APPEND_GET(uint32_t, "u", guint32)
  GDBUS_IMPL_BASIC_APPEND_GET(uint64_t, "t", guint64)
  GDBUS_IMPL_BASIC_APPEND_GET(int16_t,  "n", gint16)
  GDBUS_IMPL_BASIC_APPEND_GET(int32_t,  "i", gint32)
  GDBUS_IMPL_BASIC_APPEND_GET(int64_t,  "x", gint64)
  GDBUS_IMPL_BASIC_APPEND_GET(double,   "d", gdouble)
  // clang-format on

#undef GDBUS_IMPL_BASIC_APPEND_GET

  // bool needs special handling (GVariant uses gboolean = gint)
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, bool src) override
  {
    auto* iter = getIter(it);
    if(iter->builder)
    {
      g_variant_builder_add(iter->builder, "b", static_cast<gboolean>(src ? TRUE : FALSE));
    }
  }

  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, bool& dst) override
  {
    auto* iter = getIter(it);
    if(!iter->variant)
      return false;
    if(iter->readCursor >= iter->numChildren)
      return false;
    GVariant* child = g_variant_get_child_value(iter->variant, iter->readCursor);
    if(!child)
      return false;
    gboolean val;
    g_variant_get(child, "b", &val);
    dst = val != FALSE;
    g_variant_unref(child);
    iter->readCursor++;
    return true;
  }

  // string
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, const std::string& src) override
  {
    auto* iter = getIter(it);
    if(iter->builder)
    {
      g_variant_builder_add(iter->builder, "s", src.c_str());
    }
  }

  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, std::string& dst) override
  {
    auto* iter = getIter(it);
    if(!iter->variant)
      return false;
    if(iter->readCursor >= iter->numChildren)
      return false;
    GVariant* child = g_variant_get_child_value(iter->variant, iter->readCursor);
    if(!child)
      return false;

    const gchar* str = nullptr;
    if(g_variant_is_of_type(child, G_VARIANT_TYPE_STRING))
    {
      str = g_variant_get_string(child, nullptr);
    }
    else if(g_variant_is_of_type(child, G_VARIANT_TYPE_OBJECT_PATH))
    {
      str = g_variant_get_string(child, nullptr);
    }
    else if(g_variant_is_of_type(child, G_VARIANT_TYPE_SIGNATURE))
    {
      str = g_variant_get_string(child, nullptr);
    }

    if(str)
    {
      dst = str;
    }
    else
    {
      g_variant_unref(child);
      return false;
    }

    g_variant_unref(child);
    iter->readCursor++;
    return true;
  }

  // ObjectPath
  void eldbus_message_iter_arguments_append_impl(const MessageIterPtr& it, const ObjectPath& src) override
  {
    auto* iter = getIter(it);
    if(iter->builder)
    {
      g_variant_builder_add(iter->builder, "o", src.value.c_str());
    }
  }

  bool eldbus_message_iter_get_and_next_impl(const MessageIterPtr& it, ObjectPath& dst) override
  {
    auto* iter = getIter(it);
    if(!iter->variant)
      return false;
    if(iter->readCursor >= iter->numChildren)
      return false;
    GVariant* child = g_variant_get_child_value(iter->variant, iter->readCursor);
    if(!child)
      return false;

    if(!g_variant_is_of_type(child, G_VARIANT_TYPE_OBJECT_PATH))
    {
      g_variant_unref(child);
      return false;
    }

    const gchar* str = g_variant_get_string(child, nullptr);
    dst.value = str ? str : "";
    g_variant_unref(child);
    iter->readCursor++;
    return true;
  }

  // ---------------------------------------------------------------------------
  // Container operations
  // ---------------------------------------------------------------------------

  MessageIterPtr eldbus_message_iter_container_new_impl(const MessageIterPtr& it, int type, const std::string& sig) override
  {
    auto* parentIter = getIter(it);
    if(!parentIter->builder)
      return {};

    auto child = std::make_shared<MessageIterImpl>();
    child->ownsBuilder = true;

    // Map D-Bus container type characters to GVariantBuilder type strings
    switch(type)
    {
      case 'r': // struct
      case '(':
      {
        child->builder = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
        break;
      }
      case 'a': // array
      {
        std::string fullSig = "a" + sig;
        GVariantType* vtype = g_variant_type_new(fullSig.c_str());
        child->builder = g_variant_builder_new(vtype);
        g_variant_type_free(vtype);
        break;
      }
      case 'v': // variant
      {
        child->builder = g_variant_builder_new(G_VARIANT_TYPE_VARIANT);
        break;
      }
      case 'e': // dict entry
      case '{':
      {
        std::string fullSig = "{" + sig + "}";
        GVariantType* vtype = g_variant_type_new(fullSig.c_str());
        child->builder = g_variant_builder_new(vtype);
        g_variant_type_free(vtype);
        break;
      }
      default:
      {
        if(!sig.empty())
        {
          GVariantType* vtype = g_variant_type_new(sig.c_str());
          child->builder = g_variant_builder_new(vtype);
          g_variant_type_free(vtype);
        }
        else
        {
          child->builder = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
        }
        break;
      }
    }

    // Store reference to parent so we can close the container in destructor
    // We use a custom shared_ptr destructor to close the container
    auto parentBuilder = parentIter->builder;
    auto childBuilder  = child->builder;

    // The child's builder will be ended and merged into parent when child is destroyed
    struct ContainerCloser
    {
      GVariantBuilder* parent;
      GVariantBuilder* child;
    };
    auto closer = std::make_shared<ContainerCloser>(ContainerCloser{parentBuilder, childBuilder});

    // Wrap child in a shared_ptr with custom destructor
    auto result = std::shared_ptr<MessageIterImpl>(child.get(), [child, closer](MessageIterImpl*)
    {
      // When this shared_ptr is destroyed, end the child builder and add to parent
      GVariant* built = g_variant_builder_end(closer->child);
      if(built)
      {
        g_variant_builder_add_value(closer->parent, built);
      }
      // Prevent double-free: child's ownsBuilder should not call unref since we ended it
      child->builder     = nullptr;
      child->ownsBuilder = false;
    });

    return result;
  }

  MessageIterPtr eldbus_message_iter_get_and_next_by_type_impl(const MessageIterPtr& it, int type) override
  {
    auto* iter = getIter(it);
    if(!iter->variant)
      return {};
    if(iter->readCursor >= iter->numChildren)
      return {};

    GVariant* child = g_variant_get_child_value(iter->variant, iter->readCursor);
    if(!child)
      return {};

    auto result = std::make_shared<MessageIterImpl>();
    result->variant     = child;
    result->ownsVariant = true;

    if(g_variant_is_container(child))
    {
      result->numChildren = g_variant_n_children(child);
    }
    else
    {
      result->numChildren = 0;
    }

    iter->readCursor++;
    return result;
  }

  MessageIterPtr eldbus_message_iter_get_impl(const MessagePtr& msg, bool write) override
  {
    auto* m = getMsg(msg);
    auto  iter = std::make_shared<MessageIterImpl>();

    if(write)
    {
      // Write mode: create a builder for the message body
      iter->builder     = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
      iter->ownsBuilder = true;

      // Store builder reference in message for later retrieval
      if(m->bodyBuilder)
      {
        g_variant_builder_unref(m->bodyBuilder);
      }
      m->bodyBuilder = iter->builder;
      g_variant_builder_ref(m->bodyBuilder);
    }
    else
    {
      // Read mode: wrap the message body
      if(m->body)
      {
        iter->variant     = m->body;
        iter->ownsVariant = false; // message owns the body
        iter->numChildren = g_variant_n_children(m->body);
      }
    }

    return iter;
  }

  // ---------------------------------------------------------------------------
  // Message iter signature
  // ---------------------------------------------------------------------------

  std::string eldbus_message_iter_signature_get_impl(const MessageIterPtr& iter) override
  {
    auto* it = getIter(iter);
    if(it->variant)
    {
      const gchar* sig = g_variant_get_type_string(it->variant);
      if(sig)
      {
        std::string s(sig);
        // Strip outer tuple parens if present
        if(s.size() >= 2 && s.front() == '(' && s.back() == ')')
        {
          return s.substr(1, s.size() - 2);
        }
        return s;
      }
    }
    return {};
  }

  // ---------------------------------------------------------------------------
  // Method call creation and sending
  // ---------------------------------------------------------------------------

  MessagePtr eldbus_proxy_method_call_new_impl(const ProxyPtr& proxy, const std::string& funcName) override
  {
    auto* p = getProxy(proxy);
    if(!p)
      return {};

    auto msg       = std::make_shared<MessageImpl>();
    msg->path      = p->path;
    msg->interface = p->interface;
    msg->member    = funcName;
    msg->destination = p->busName;
    return msg;
  }

  MessagePtr eldbus_proxy_send_and_block_impl(const ProxyPtr& proxy, const MessagePtr& msg) override
  {
    auto* p = getProxy(proxy);
    auto* m = getMsg(msg);
    if(!p || !m)
      return {};

    auto connImpl = p->connection.lock();
    if(!connImpl || !connImpl->conn)
      return {};

    // Build the argument tuple from the message's builder
    GVariant* args = nullptr;
    if(m->bodyBuilder)
    {
      args = g_variant_builder_end(m->bodyBuilder);
      g_variant_builder_unref(m->bodyBuilder);
      m->bodyBuilder = nullptr;
    }

    GError*  err    = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
      connImpl->conn,
      m->destination.c_str(),
      m->path.c_str(),
      m->interface.c_str(),
      m->member.c_str(),
      args,
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      GDBUS_CALL_TIMEOUT_MS,
      nullptr,
      &err);

    auto reply = std::make_shared<MessageImpl>();
    if(err)
    {
      reply->error = err;
    }
    if(result)
    {
      reply->body     = result;
      reply->ownsBody = true;
    }
    return reply;
  }

  PendingPtr eldbus_proxy_send_impl(const ProxyPtr& proxy, const MessagePtr& msg, const SendCallback& callback) override
  {
    auto* p = getProxy(proxy);
    auto* m = getMsg(msg);
    if(!p || !m)
      return {};

    auto connImpl = p->connection.lock();
    if(!connImpl || !connImpl->conn)
      return {};

    // Build the argument tuple
    GVariant* args = nullptr;
    if(m->bodyBuilder)
    {
      args = g_variant_builder_end(m->bodyBuilder);
      g_variant_builder_unref(m->bodyBuilder);
      m->bodyBuilder = nullptr;
    }

    struct AsyncData
    {
      SendCallback callback;
    };
    auto* data = new AsyncData{callback};

    g_dbus_connection_call(
      connImpl->conn,
      m->destination.c_str(),
      m->path.c_str(),
      m->interface.c_str(),
      m->member.c_str(),
      args,
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      GDBUS_CALL_TIMEOUT_MS,
      nullptr,
      [](GObject* source, GAsyncResult* res, gpointer userData)
      {
        auto* ad = static_cast<AsyncData*>(userData);
        GError*  err    = nullptr;
        GVariant* result = g_dbus_connection_call_finish(
          G_DBUS_CONNECTION(source), res, &err);

        auto reply = std::make_shared<MessageImpl>();
        if(err)
        {
          reply->error = err;
        }
        if(result)
        {
          reply->body     = result;
          reply->ownsBody = true;
        }

        if(ad->callback)
        {
          ad->callback(reply);
        }
        delete ad;
      },
      data);

    return std::make_shared<PendingImpl>();
  }

  // ---------------------------------------------------------------------------
  // Message operations
  // ---------------------------------------------------------------------------

  bool eldbus_message_error_get_impl(const MessagePtr& msg, std::string& name, std::string& text) override
  {
    auto* m = getMsg(msg);
    if(!m || !m->error)
      return false;

    // GError doesn't have a structured error name, synthesize from domain+code
    if(m->error->domain == g_dbus_error_quark())
    {
      gchar* remoteName = g_dbus_error_get_remote_error(m->error);
      if(remoteName)
      {
        name = remoteName;
        g_free(remoteName);
      }
      else
      {
        name = "org.freedesktop.DBus.Error.Failed";
      }
    }
    else
    {
      name = "org.freedesktop.DBus.Error.Failed";
    }
    text = m->error->message ? m->error->message : "";
    return true;
  }

  std::string eldbus_message_signature_get_impl(const MessagePtr& msg) override
  {
    auto* m = getMsg(msg);
    if(!m || !m->body)
      return {};

    const gchar* sig = g_variant_get_type_string(m->body);
    if(!sig)
      return {};

    std::string s(sig);
    // Strip outer tuple parens
    if(s.size() >= 2 && s.front() == '(' && s.back() == ')')
    {
      return s.substr(1, s.size() - 2);
    }
    return s;
  }

  MessagePtr eldbus_message_method_return_new_impl(const MessagePtr& msg) override
  {
    auto reply       = std::make_shared<MessageImpl>();
    reply->requestMsg = std::static_pointer_cast<MessageImpl>(msg);
    return reply;
  }

  MessagePtr eldbus_message_error_new_impl(const MessagePtr& msg, const std::string& err, const std::string& txt) override
  {
    auto reply  = std::make_shared<MessageImpl>();
    reply->error = g_error_new_literal(g_dbus_error_quark(), G_DBUS_ERROR_FAILED, txt.c_str());
    reply->requestMsg = std::static_pointer_cast<MessageImpl>(msg);
    return reply;
  }

  MessagePtr eldbus_message_signal_new_impl(const std::string& path, const std::string& iface, const std::string& name) override
  {
    auto msg       = std::make_shared<MessageImpl>();
    msg->path      = path;
    msg->interface = iface;
    msg->member    = name;
    return msg;
  }

  MessagePtr eldbus_message_ref_impl(const MessagePtr& msg) override
  {
    auto* m = getMsg(msg);
    if(m && m->body)
    {
      g_variant_ref(m->body);
    }
    return msg;
  }

  // ---------------------------------------------------------------------------
  // Signal sending
  // ---------------------------------------------------------------------------

  PendingPtr eldbus_connection_send_impl(const ConnectionPtr& conn, const MessagePtr& msg) override
  {
    auto* c = getConn(conn);
    auto* m = getMsg(msg);
    if(!c || !c->conn || !m)
      return {};

    // Build the signal body from the message's builder
    GVariant* body = nullptr;
    if(m->bodyBuilder)
    {
      body = g_variant_builder_end(m->bodyBuilder);
      g_variant_builder_unref(m->bodyBuilder);
      m->bodyBuilder = nullptr;
    }
    else if(m->body)
    {
      body = m->body;
      g_variant_ref(body);
    }

    GError* err = nullptr;
    g_dbus_connection_emit_signal(
      c->conn,
      nullptr,
      m->path.c_str(),
      m->interface.c_str(),
      m->member.c_str(),
      body,
      &err);

    if(err)
    {
      g_error_free(err);
    }

    return std::make_shared<PendingImpl>();
  }

  // ---------------------------------------------------------------------------
  // Signal handling
  // ---------------------------------------------------------------------------

  void eldbus_proxy_signal_handler_add_impl(const ProxyPtr& proxy, const std::string& member, const std::function<void(const MessagePtr&)>& cb) override
  {
    auto* p = getProxy(proxy);
    if(!p)
      return;

    auto connImpl = p->connection.lock();
    if(!connImpl || !connImpl->conn)
      return;

    struct SignalData
    {
      std::function<void(const MessagePtr&)> callback;
    };
    auto* data = new SignalData{cb};

    g_dbus_connection_signal_subscribe(
      connImpl->conn,
      p->busName.empty() ? nullptr : p->busName.c_str(),
      p->interface.c_str(),
      member.c_str(),
      p->path.c_str(),
      nullptr,
      G_DBUS_SIGNAL_FLAGS_NONE,
      [](GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant* parameters, gpointer userData)
      {
        auto* sd = static_cast<SignalData*>(userData);
        auto  msg = std::make_shared<MessageImpl>();
        if(parameters)
        {
          msg->body     = parameters;
          msg->ownsBody = false; // GDBus owns signal parameters
        }
        sd->callback(msg);
      },
      data,
      [](gpointer userData)
      {
        delete static_cast<SignalData*>(userData);
      });
  }

  // ---------------------------------------------------------------------------
  // Interface registration (most complex)
  // ---------------------------------------------------------------------------

  struct InterfaceRegistration
  {
    std::unordered_map<std::string, DBusWrapper::MethodInfo>   methodsMap;
    std::unordered_map<std::string, DBusWrapper::PropertyInfo> propertiesMap;
    std::unordered_map<unsigned int, DBusWrapper::SignalInfo>   signalsMap;
    DBusWrapper::ConnectionWeakPtr                             connection;
    GDBusNodeInfo*                                             introspectionData = nullptr;
  };

  static void handleMethodCall(GDBusConnection*       connection,
                               const gchar*           sender,
                               const gchar*           objectPath,
                               const gchar*           interfaceName,
                               const gchar*           methodName,
                               GVariant*              parameters,
                               GDBusMethodInvocation* invocation,
                               gpointer               userData)
  {
    auto* reg = static_cast<InterfaceRegistration*>(userData);

    auto it = reg->methodsMap.find(methodName);
    if(it == reg->methodsMap.end())
    {
      g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.UnknownMethod", "Method not found");
      return;
    }

    auto conn = reg->connection.lock();
    if(!conn)
    {
      g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.Failed", "Connection lost");
      return;
    }

    // Build request message from parameters
    auto reqMsg       = std::make_shared<MessageImpl>();
    reqMsg->path      = objectPath ? objectPath : "";
    reqMsg->interface = interfaceName ? interfaceName : "";
    reqMsg->member    = methodName ? methodName : "";
    if(parameters)
    {
      reqMsg->body     = parameters;
      reqMsg->ownsBody = false;
    }

    DBus::DBusServer::CurrentObjectSetter currentObjectSetter(conn, reqMsg->path);
    auto replyMsg = it->second.callback(reqMsg);

    auto* reply = static_cast<MessageImpl*>(replyMsg.get());
    if(reply && reply->error)
    {
      gchar* remoteName = nullptr;
      if(reply->error->domain == g_dbus_error_quark())
      {
        remoteName = g_dbus_error_get_remote_error(reply->error);
      }
      g_dbus_method_invocation_return_dbus_error(
        invocation,
        remoteName ? remoteName : "org.freedesktop.DBus.Error.Failed",
        reply->error->message ? reply->error->message : "");
      if(remoteName)
        g_free(remoteName);
      return;
    }

    // Build reply body
    GVariant* replyBody = nullptr;
    if(reply && reply->bodyBuilder)
    {
      replyBody = g_variant_builder_end(reply->bodyBuilder);
      g_variant_builder_unref(reply->bodyBuilder);
      reply->bodyBuilder = nullptr;
    }
    else if(reply && reply->body)
    {
      replyBody = reply->body;
      g_variant_ref(replyBody);
    }

    g_dbus_method_invocation_return_value(invocation, replyBody);
  }

  static GVariant* handleGetProperty(GDBusConnection* connection,
                                     const gchar*     sender,
                                     const gchar*     objectPath,
                                     const gchar*     interfaceName,
                                     const gchar*     propertyName,
                                     GError**         error,
                                     gpointer         userData)
  {
    auto* reg = static_cast<InterfaceRegistration*>(userData);

    auto it = reg->propertiesMap.find(propertyName);
    if(it == reg->propertiesMap.end() || !it->second.getCallback)
    {
      g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Unknown property: %s", propertyName);
      return nullptr;
    }

    auto conn = reg->connection.lock();
    if(!conn)
    {
      g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Connection lost");
      return nullptr;
    }

    // Create a request message and iter for the property callback
    auto reqMsg       = std::make_shared<MessageImpl>();
    reqMsg->path      = objectPath ? objectPath : "";
    reqMsg->interface = interfaceName ? interfaceName : "";

    auto dstIter = std::make_shared<MessageIterImpl>();
    dstIter->builder     = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
    dstIter->ownsBuilder = true;

    DBus::DBusServer::CurrentObjectSetter currentObjectSetter(conn, reqMsg->path);
    auto errStr = it->second.getCallback(reqMsg, dstIter);

    if(!errStr.empty())
    {
      g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", errStr.c_str());
      return nullptr;
    }

    // Extract the value from the builder
    GVariant* tuple = g_variant_builder_end(dstIter->builder);
    dstIter->builder     = nullptr;
    dstIter->ownsBuilder = false;

    if(tuple && g_variant_n_children(tuple) > 0)
    {
      GVariant* val = g_variant_get_child_value(tuple, 0);
      g_variant_unref(tuple);
      return val;
    }
    if(tuple)
    {
      g_variant_unref(tuple);
    }

    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Property get returned no value");
    return nullptr;
  }

  static gboolean handleSetProperty(GDBusConnection* connection,
                                    const gchar*     sender,
                                    const gchar*     objectPath,
                                    const gchar*     interfaceName,
                                    const gchar*     propertyName,
                                    GVariant*        value,
                                    GError**         error,
                                    gpointer         userData)
  {
    auto* reg = static_cast<InterfaceRegistration*>(userData);

    auto it = reg->propertiesMap.find(propertyName);
    if(it == reg->propertiesMap.end() || !it->second.setCallback)
    {
      g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Unknown property: %s", propertyName);
      return FALSE;
    }

    auto conn = reg->connection.lock();
    if(!conn)
    {
      g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Connection lost");
      return FALSE;
    }

    auto reqMsg       = std::make_shared<MessageImpl>();
    reqMsg->path      = objectPath ? objectPath : "";
    reqMsg->interface = interfaceName ? interfaceName : "";

    auto srcIter = std::make_shared<MessageIterImpl>();
    if(value)
    {
      srcIter->variant     = value;
      srcIter->ownsVariant = false;
      srcIter->numChildren = g_variant_is_container(value) ? g_variant_n_children(value) : 0;
    }

    DBus::DBusServer::CurrentObjectSetter currentObjectSetter(conn, reqMsg->path);
    auto errStr = it->second.setCallback(reqMsg, srcIter);

    if(!errStr.empty())
    {
      g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", errStr.c_str());
      return FALSE;
    }
    return TRUE;
  }

  void add_interface_impl(bool                                fallback,
                          const std::string&                  pathName,
                          const ConnectionPtr&                connection,
                          std::vector<std::function<void()>>& destructors,
                          const std::string&                  interfaceName,
                          std::vector<MethodInfo>&            dscrMethods,
                          std::vector<PropertyInfo>&          dscrProperties,
                          std::vector<SignalInfo>&            dscrSignals) override
  {
    auto* c = getConn(connection);
    if(!c || !c->conn)
      return;

    auto reg = new InterfaceRegistration();
    reg->connection = std::static_pointer_cast<ConnectionImpl>(connection);

    // Build GDBus introspection XML
    std::ostringstream xml;
    xml << "<node>"
        << "<interface name='" << interfaceName << "'>";

    for(auto& method : dscrMethods)
    {
      auto key = method.memberName;
      xml << "<method name='" << method.memberName << "'>";
      for(auto& arg : method.in)
      {
        xml << "<arg name='" << arg.first << "' type='" << arg.second << "' direction='in'/>";
      }
      for(auto& arg : method.out)
      {
        xml << "<arg name='" << arg.first << "' type='" << arg.second << "' direction='out'/>";
      }
      xml << "</method>";
      reg->methodsMap[key] = std::move(method);
    }

    for(auto& prop : dscrProperties)
    {
      auto key = prop.memberName;
      std::string access;
      if(prop.getCallback && prop.setCallback)
        access = "readwrite";
      else if(prop.getCallback)
        access = "read";
      else
        access = "write";

      xml << "<property name='" << prop.memberName << "' type='" << prop.typeSignature << "' access='" << access << "'/>";
      reg->propertiesMap[key] = std::move(prop);
    }

    for(auto& sig : dscrSignals)
    {
      xml << "<signal name='" << sig.memberName << "'>";
      for(auto& arg : sig.args)
      {
        xml << "<arg name='" << arg.first << "' type='" << arg.second << "'/>";
      }
      xml << "</signal>";
    }

    xml << "</interface></node>";

    dscrMethods.clear();
    dscrProperties.clear();
    dscrSignals.clear();

    std::string xmlStr = xml.str();
    GError* err = nullptr;
    reg->introspectionData = g_dbus_node_info_new_for_xml(xmlStr.c_str(), &err);
    if(err)
    {
      ACCESSIBILITY_LOG_ERROR("g_dbus_node_info_new_for_xml failed: %s\n", err->message);
      g_error_free(err);
      delete reg;
      return;
    }

    if(!reg->introspectionData || !reg->introspectionData->interfaces || !reg->introspectionData->interfaces[0])
    {
      ACCESSIBILITY_LOG_ERROR("No interface found in introspection XML\n");
      if(reg->introspectionData)
        g_dbus_node_info_unref(reg->introspectionData);
      delete reg;
      return;
    }

    static const GDBusInterfaceVTable vtable = {
      handleMethodCall,
      handleGetProperty,
      handleSetProperty,
      {0}
    };

    if(fallback)
    {
      // Use subtree for fallback registration
      static const GDBusSubtreeVTable subtreeVtable = {
        // enumerate
        [](GDBusConnection*, const gchar*, const gchar*, gpointer) -> gchar** {
          return nullptr;
        },
        // introspect
        [](GDBusConnection*, const gchar*, const gchar*, const gchar*, gpointer userData) -> GDBusInterfaceInfo** {
          auto* r = static_cast<InterfaceRegistration*>(userData);
          if(!r->introspectionData || !r->introspectionData->interfaces || !r->introspectionData->interfaces[0])
            return nullptr;
          auto** result = g_new0(GDBusInterfaceInfo*, 2);
          result[0] = g_dbus_interface_info_ref(r->introspectionData->interfaces[0]);
          return result;
        },
        // dispatch
        [](GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, gpointer* outUserData, gpointer userData) -> const GDBusInterfaceVTable* {
          // Pass through the registration user data to the vtable callbacks
          *outUserData = userData;
          return &vtable;
        },
        {0}
      };

      guint regId = g_dbus_connection_register_subtree(
        c->conn,
        pathName.c_str(),
        &subtreeVtable,
        G_DBUS_SUBTREE_FLAGS_DISPATCH_TO_UNENUMERATED_NODES,
        reg,
        nullptr,
        &err);

      if(err || regId == 0)
      {
        ACCESSIBILITY_LOG_ERROR("g_dbus_connection_register_subtree failed: %s\n", err ? err->message : "unknown");
        if(err)
          g_error_free(err);
        g_dbus_node_info_unref(reg->introspectionData);
        delete reg;
        return;
      }

      auto* conn = c->conn;
      destructors.push_back([conn, regId, reg]()
      {
        g_dbus_connection_unregister_subtree(conn, regId);
        g_dbus_node_info_unref(reg->introspectionData);
        delete reg;
      });
    }
    else
    {
      guint regId = g_dbus_connection_register_object(
        c->conn,
        pathName.c_str(),
        reg->introspectionData->interfaces[0],
        &vtable,
        reg,
        nullptr,
        &err);

      if(err || regId == 0)
      {
        ACCESSIBILITY_LOG_ERROR("g_dbus_connection_register_object failed: %s\n", err ? err->message : "unknown");
        if(err)
          g_error_free(err);
        g_dbus_node_info_unref(reg->introspectionData);
        delete reg;
        return;
      }

      auto* conn = c->conn;
      destructors.push_back([conn, regId, reg]()
      {
        g_dbus_connection_unregister_object(conn, regId);
        g_dbus_node_info_unref(reg->introspectionData);
        delete reg;
      });
    }
  }

  // ---------------------------------------------------------------------------
  // Property change listener
  // ---------------------------------------------------------------------------

  void add_property_changed_event_listener_impl(const ProxyPtr& proxy, const std::string& interface, const std::string& name, std::function<void(const _Eina_Value*)> cb) override
  {
    auto* p = getProxy(proxy);
    if(!p || !p->proxy)
      return;

    struct PropertyChangeData
    {
      std::string                          propertyName;
      std::string                          interfaceName;
      std::function<void(const _Eina_Value*)> callback;
    };
    auto* data = new PropertyChangeData{name, interface, cb};

    g_signal_connect_data(
      p->proxy,
      "g-properties-changed",
      G_CALLBACK(+[](GDBusProxy*  proxy,
                      GVariant*    changedProperties,
                      GStrv        invalidatedProperties,
                      gpointer     userData)
      {
        auto* pd = static_cast<PropertyChangeData*>(userData);

        // Check interface matches
        const char* ifc = g_dbus_proxy_get_interface_name(proxy);
        if(!ifc || pd->interfaceName != ifc)
          return;

        // Look for our property in changedProperties dict
        GVariantIter iter;
        const gchar* key;
        GVariant*    value;
        g_variant_iter_init(&iter, changedProperties);
        while(g_variant_iter_next(&iter, "{&sv}", &key, &value))
        {
          if(pd->propertyName == key)
          {
            // Convert to _Eina_Value
            if(g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN))
            {
              bool boolVal = g_variant_get_boolean(value) != FALSE;
              _Eina_Value ev;
              ev.type  = 1; // bool type
              ev.value = &boolVal;
              pd->callback(&ev);
            }
            g_variant_unref(value);
            return;
          }
          g_variant_unref(value);
        }
      }),
      data,
      [](gpointer userData, GClosure*)
      {
        delete static_cast<PropertyChangeData*>(userData);
      },
      static_cast<GConnectFlags>(0));
  }
};

// =============================================================================
// DBusWrapper Install/Installed
// =============================================================================

DBusWrapper* DBusWrapper::Installed()
{
  if(!InstalledWrapper)
  {
    InstalledWrapper.reset(new GDBusWrapper);
  }
  return InstalledWrapper.get();
}

void DBusWrapper::Install(std::unique_ptr<DBusWrapper> w)
{
  InstalledWrapper = std::move(w);
}
