#ifndef ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_BASE_H
#define ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_BASE_H

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
#include <list>
#include <memory>
#include <tuple>

// INTERNAL INCLUDES
#include <accessibility/api/proxy-accessible.h>
#include <accessibility/api/accessible.h>
#include <accessibility/api/application.h>
#include <accessibility/api/collection.h>
#include <accessibility/api/socket.h>
#include <accessibility/internal/bridge/accessibility-common.h>
#include <accessibility/internal/bridge/collection-impl.h>
#include <accessibility/internal/bridge/ipc/ipc-registry-client.h>
#include <accessibility/internal/bridge/ipc/ipc-server.h>
#include <accessibility/internal/bridge/ipc/ipc-transport-factory.h>
#ifdef ENABLE_TIDL_BACKEND
#include <accessibility/internal/bridge/tidl/tidl-interface-description.h>
#endif

namespace Accessibility
{
/**
 * @brief The ApplicationAccessible class is to define Accessibility Application.
 */
class ApplicationAccessible : public Accessibility::Accessible,
                              public Accessibility::Application,
                              public Accessibility::Collection,
                              public Accessibility::Socket,
                              public std::enable_shared_from_this<ApplicationAccessible>
{
public:
  Accessibility::ProxyAccessible          mParent;
  std::vector<Accessibility::Accessible*> mChildren;
  std::string                                   mName;
  std::string                                   mToolkitName{"dali"};
  bool                                          mIsEmbedded{false};
  bool                                          mShouldIncludeHidden{false};

  std::string                                   GetName() const override;
  std::string                                   GetDescription() const override;
  std::string                                   GetValue() const override;
  Accessibility::Accessible*              GetParent() override;
  size_t                                        GetChildCount() const override;
  std::vector<Accessibility::Accessible*> GetChildren() override;
  Accessibility::Accessible*              GetChildAtIndex(size_t index) override;
  size_t                                        GetIndexInParent() override;
  Accessibility::Role                     GetRole() const override;
  Accessibility::States                   GetStates() override;
  Accessibility::Attributes               GetAttributes() const override;
  void InitDefaultFeatures() override;

  bool                                       DoGesture(const Accessibility::GestureInfo& gestureInfo) override;
  std::vector<Accessibility::Relation> GetRelationSet() override;
  Accessibility::Address               GetAddress() const override;
  std::string                                GetStringProperty(std::string propertyName) const override;

  // Application
  std::string GetToolkitName() const override;
  std::string GetVersion() const override;
  bool        GetIncludeHidden() const override;
  bool        SetIncludeHidden(bool includeHidden) override;

  // Socket
  Accessibility::Address Embed(Accessibility::Address plug) override;
  void                         Unembed(Accessibility::Address plug) override;
  void                         SetOffset(std::int32_t x, std::int32_t y) override;

  // Component
  Accessibility::Rect<float>          GetExtents(Accessibility::CoordinateType type) const override;
  Accessibility::ComponentLayer GetLayer() const override;
  std::int16_t                        GetMdiZOrder() const override;
  bool                                GrabFocus() override;
  double                              GetAlpha() const override;
  bool                                GrabHighlight() override;
  bool                                ClearHighlight() override;
  bool                                IsScrollable() const override;

  // Collection
  std::vector<Accessible*> GetMatches(MatchRule rule, uint32_t sortBy, size_t maxCount) override;
  std::vector<Accessible*> GetMatchesInMatches(MatchRule firstRule, MatchRule secondRule, uint32_t sortBy, int32_t firstCount, int32_t secondCount) override;

private:
  std::shared_ptr<Collection>   mCollection{nullptr};
};
} //namespace Accessibility

/**
 * @brief Enumeration for CoalescableMessages.
 */
enum class CoalescableMessages
{
  BOUNDS_CHANGED,                                     ///< Bounds changed
  SET_OFFSET,                                         ///< Set offset
  POST_RENDER,                                        ///< Post render
  STATE_CHANGED_BEGIN = 500,                          ///< State changed (begin of reserved range)
  STATE_CHANGED_END   = STATE_CHANGED_BEGIN + 99,     ///< State changed (end of reserved range)
  PROPERTY_CHANGED_BEGIN,                             ///< Property changed (begin of reserved range)
  PROPERTY_CHANGED_END = PROPERTY_CHANGED_BEGIN + 99, ///< Property changed (end of reserved range)
};

// Custom specialization of std::hash
namespace std
{
template<>
struct hash<std::pair<CoalescableMessages, Accessibility::Accessible*>>
{
  size_t operator()(std::pair<CoalescableMessages, Accessibility::Accessible*> value) const
  {
    return (static_cast<size_t>(value.first) * 131) ^ reinterpret_cast<size_t>(value.second);
  }
};
} // namespace std

/**
 * @brief The BridgeBase class is basic class for Bridge functions.
 */
class BridgeBase : public Accessibility::Bridge
{
  std::unordered_map<std::pair<CoalescableMessages, Accessibility::Accessible*>, std::tuple<unsigned int, unsigned int, std::function<void()>>> mCoalescableMessages;

  /**
   * @brief Removes all CoalescableMessages using Tick signal.
   *
   * @return False if mCoalescableMessages is empty, otherwise true.
   */
  bool TickCoalescableMessages();

public:
  /**
   * @brief Adds CoalescableMessages, Accessible, and delay time to mCoalescableMessages.
   *
   * @param[in] kind CoalescableMessages enum value
   * @param[in] obj Accessible object
   * @param[in] delay The delay time
   * @param[in] functor The function to be called // NEED TO UPDATE!
   */
  void AddCoalescableMessage(CoalescableMessages kind, Accessibility::Accessible* obj, float delay, std::function<void()> functor);

  /**
   * @copydoc Accessibility::Bridge::GetBusName()
   */
  const std::string& GetBusName() const override;

  /**
   * @copydoc Accessibility::Bridge::AddTopLevelWindow()
   */
  void AddTopLevelWindow(Accessibility::Accessible* windowAccessible) override;

  /**
   * @copydoc Accessibility::Bridge::RemoveTopLevelWindow()
   */
  void RemoveTopLevelWindow(Accessibility::Accessible* windowAccessible) override;

  /**
   * @copydoc Accessibility::Bridge::RegisterDefaultLabel()
   */
  void RegisterDefaultLabel(Accessibility::Accessible* accessible) override;

  /**
   * @copydoc Accessibility::Bridge::UnregisterDefaultLabel()
   */
  void UnregisterDefaultLabel(Accessibility::Accessible* accessible) override;

  /**
   * @copydoc Accessibility::Bridge::GetDefaultLabel()
   */
  Accessibility::Accessible* GetDefaultLabel(Accessibility::Accessible* root) override;

  /**
   * @copydoc Accessibility::Bridge::GetApplication()
   */
  Accessibility::Accessible* GetApplication() const override
  {
    return mApplication.get();
  }

  /**
   * @brief Adds function to interface description.
   *
   * Casts the abstract InterfaceDescription to the concrete backend type
   * (DBusInterfaceDescription or TidlInterfaceDescription) to register
   * the method handler.
   */
  template<typename SELF, typename... RET, typename... ARGS>
  void AddFunctionToInterface(
    Ipc::InterfaceDescription& desc, const std::string& funcName, DBus::ValueOrError<RET...> (SELF::*funcPtr)(ARGS...))
  {
    if(auto self = dynamic_cast<SELF*>(this))
    {
      auto callback = [=](ARGS... args) -> DBus::ValueOrError<RET...>
      {
        try
        {
          return (self->*funcPtr)(std::move(args)...);
        }
        catch(std::domain_error& e)
        {
          return DBus::Error{e.what()};
        }
      };
#ifdef ENABLE_TIDL_BACKEND
      auto& tidlDesc = static_cast<Ipc::Tidl::TidlInterfaceDescription&>(desc);
      tidlDesc.addMethod<DBus::ValueOrError<RET...>(ARGS...)>(funcName, callback);
#else
      auto& dbusDesc = static_cast<DBus::DBusInterfaceDescription&>(desc);
      dbusDesc.addMethod<DBus::ValueOrError<RET...>(ARGS...)>(funcName, callback);
#endif
    }
  }

  /**
   * @brief Adds 'Get' property to interface description.
   */
  template<typename T, typename SELF>
  void AddGetPropertyToInterface(Ipc::InterfaceDescription& desc,
                                 const std::string&         funcName,
                                 T (SELF::*funcPtr)())
  {
    if(auto self = dynamic_cast<SELF*>(this))
    {
      auto getter = [=]() -> DBus::ValueOrError<T>
      {
        try
        {
          return (self->*funcPtr)();
        }
        catch(std::domain_error& e)
        {
          return DBus::Error{e.what()};
        }
      };
#ifdef ENABLE_TIDL_BACKEND
      auto& tidlDesc = static_cast<Ipc::Tidl::TidlInterfaceDescription&>(desc);
      tidlDesc.addProperty<T>(funcName, getter, {});
#else
      auto& dbusDesc = static_cast<DBus::DBusInterfaceDescription&>(desc);
      dbusDesc.addProperty<T>(funcName, getter, {});
#endif
    }
  }

  /**
   * @brief Adds 'Set' property to interface description.
   */
  template<typename T, typename SELF>
  void AddSetPropertyToInterface(Ipc::InterfaceDescription& desc,
                                 const std::string&         funcName,
                                 void (SELF::*funcPtr)(T))
  {
    if(auto self = dynamic_cast<SELF*>(this))
    {
      auto setter = [=](T t) -> DBus::ValueOrError<void>
      {
        try
        {
          (self->*funcPtr)(std::move(t));
          return {};
        }
        catch(std::domain_error& e)
        {
          return DBus::Error{e.what()};
        }
      };
#ifdef ENABLE_TIDL_BACKEND
      auto& tidlDesc = static_cast<Ipc::Tidl::TidlInterfaceDescription&>(desc);
      tidlDesc.addProperty<T>(funcName, {}, setter);
#else
      auto& dbusDesc = static_cast<DBus::DBusInterfaceDescription&>(desc);
      dbusDesc.addProperty<T>(funcName, {}, setter);
#endif
    }
  }

  /**
   * @brief Adds 'Set' and 'Get' properties to interface description.
   */
  template<typename T, typename T1, typename SELF>
  void AddGetSetPropertyToInterface(Ipc::InterfaceDescription& desc,
                                    const std::string&         funcName,
                                    T1 (SELF::*funcPtrGet)(),
                                    DBus::ValueOrError<void> (SELF::*funcPtrSet)(T))
  {
    if(auto self = dynamic_cast<SELF*>(this))
    {
      auto getter = [=]() -> DBus::ValueOrError<T>
      {
        try
        {
          return (self->*funcPtrGet)();
        }
        catch(std::domain_error& e)
        {
          return DBus::Error{e.what()};
        }
      };
      auto setter = [=](T t) -> DBus::ValueOrError<void>
      {
        try
        {
          (self->*funcPtrSet)(std::move(t));
          return {};
        }
        catch(std::domain_error& e)
        {
          return DBus::Error{e.what()};
        }
      };
#ifdef ENABLE_TIDL_BACKEND
      auto& tidlDesc = static_cast<Ipc::Tidl::TidlInterfaceDescription&>(desc);
      tidlDesc.addProperty<T>(funcName, getter, setter);
#else
      auto& dbusDesc = static_cast<DBus::DBusInterfaceDescription&>(desc);
      dbusDesc.addProperty<T>(funcName, getter, setter);
#endif
    }
  }

  /**
   * @brief Adds 'Get' and 'Set' properties to interface description.
   */
  template<typename T, typename T1, typename SELF>
  void AddGetSetPropertyToInterface(Ipc::InterfaceDescription& desc,
                                    const std::string&         funcName,
                                    T1 (SELF::*funcPtrGet)(),
                                    void (SELF::*funcPtrSet)(T))
  {
    if(auto self = dynamic_cast<SELF*>(this))
    {
      auto getter = [=]() -> DBus::ValueOrError<T>
      {
        try
        {
          return (self->*funcPtrGet)();
        }
        catch(std::domain_error& e)
        {
          return DBus::Error{e.what()};
        }
      };
      auto setter = [=](T t) -> DBus::ValueOrError<void>
      {
        try
        {
          (self->*funcPtrSet)(std::move(t));
          return {};
        }
        catch(std::domain_error& e)
        {
          return DBus::Error{e.what()};
        }
      };
#ifdef ENABLE_TIDL_BACKEND
      auto& tidlDesc = static_cast<Ipc::Tidl::TidlInterfaceDescription&>(desc);
      tidlDesc.addProperty<T>(funcName, getter, setter);
#else
      auto& dbusDesc = static_cast<DBus::DBusInterfaceDescription&>(desc);
      dbusDesc.addProperty<T>(funcName, getter, setter);
#endif
    }
  }

  /**
   * @brief Gets the string of the path excluding the specified prefix.
   *
   * @param path The path to get
   * @return The string stripped of the specific prefix
   */
  static std::string StripPrefix(const std::string& path);

  /**
   * @brief Finds the Accessible object according to the path.
   *
   * @param[in] path The path for Accessible object
   * @return The Accessible object corresponding to the path
   */
  Accessibility::Accessible* Find(const std::string& path) const;

  /**
   * @brief Finds the Accessible object with the given address.
   *
   * @param[in] ptr The unique Address of the object
   * @return The Accessible object corresponding to the path
   */
  Accessibility::Accessible* Find(const Accessibility::Address& ptr) const;

  /**
   * @brief Returns the target object of the currently executed DBus method call.
   *
   * @return The Accessible object
   * @note When a DBus method is called on some object, this target object (`currentObject`) is temporarily saved by the bridge,
   * because DBus handles the invocation target separately from the method arguments.
   * We then use the saved object inside the 'glue' method (e.g. BridgeValue::GetMinimum)
   * to call the equivalent method on the respective C++ object (this could be ScrollBar::AccessibleImpl::GetMinimum in the example given).
   */
  Accessibility::Accessible* FindCurrentObject() const;

  /**
   * @brief Returns the target object of the currently executed DBus method call.
   *
   * This method tries to downcast the return value of FindCurrentObject() to the requested type,
   * issuing an error reply to the DBus caller if the requested type is not implemented. Whether
   * a given type is implemented is decided based on the return value of Accessible::GetInterfaces()
   * for the current object.
   *
   * @tparam I The requested AT-SPI interface
   * @return The Accessible object (cast to a more derived type)
   *
   * @see FindCurrentObject()
   * @see Accessibility::AtspiInterface
   * @see Accessibility::AtspiInterfaceType
   * @see Accessibility::Accessible::GetInterfaces()
   */
  template<Accessibility::AtspiInterface I>
  auto FindCurrentObjectWithInterface() const
  {
    using Type = Accessibility::AtspiInterfaceType<I>;

    std::shared_ptr<Type> result;
    auto* currentObject = FindCurrentObject();
    assert(currentObject && "Current BridgeBase's Accessible should not be nullptr"); // FindCurrentObject() throws domain_error

    if(!(result = currentObject->GetFeature<Type>()))
    {
      std::stringstream s;

      s << "Object " << currentObject->GetAddress().ToString();
      s << " does not implement ";
      s << Accessibility::Accessible::GetInterfaceName(I);

      throw std::domain_error{s.str()};
    }

    return result;
  }

  /**
   * @copydoc Accessibility::Bridge::FindByPath()
   */
  Accessibility::Accessible* FindByPath(const std::string& name) const override;

  /**
   * @copydoc Accessibility::Bridge::SetApplicationName()
   */
  void SetApplicationName(std::string name) override
  {
    mApplication->mName = std::move(name);
  }

  /**
   * @copydoc Accessibility::Bridge::SetToolkitName()
   */
  void SetToolkitName(std::string_view toolkitName) override
  {
    mApplication->mToolkitName = std::string{toolkitName};
  }

protected:
  // Pair of (window root accessible, label accessible)
  using DefaultLabelType  = std::pair<Accessibility::Accessible*, Accessibility::Accessible*>;
  using DefaultLabelsType = std::list<DefaultLabelType>;

  std::shared_ptr<Accessibility::ApplicationAccessible> mApplication;

  DefaultLabelsType mDefaultLabels;
  bool              mIsScreenReaderSuppressed = false;

private:
  /**
   * @brief Sets an ID.
   * @param[in] id An ID (integer value)
   */
  void SetId(int id);

  /**
   * @brief Gets the ID.
   * @return The ID to be set
   */
  int GetId();

  /**
   * @brief Update registered events.
   */
  void UpdateRegisteredEvents();

  using CacheElementType = std::tuple<
    Accessibility::Address,
    Accessibility::Address,
    Accessibility::Address,
    std::vector<Accessibility::Address>,
    std::vector<std::string>,
    std::string,
    Accessibility::Role,
    std::string,
    std::array<uint32_t, 2>>;

  /**
   * @brief Gets Items  // NEED TO UPDATE!
   *
   * @return
   */
  DBus::ValueOrError<std::vector<CacheElementType>> GetItems();

  /**
   * @brief Removes expired elements from the default label collection.
   */
  void CompressDefaultLabels();

protected:
  BridgeBase();
  virtual ~BridgeBase();

  /**
   * @copydoc Accessibility::Bridge::ForceUp()
   */
  ForceUpResult ForceUp() override;

  /**
   * @copydoc Accessibility::Bridge::ForceDown()
   */
  void ForceDown() override;

  std::unique_ptr<Ipc::TransportFactory> mTransportFactory;
  std::unique_ptr<Ipc::Server>           mIpcServer;
  std::unique_ptr<Ipc::RegistryClient>   mRegistryClient;
  int                                    mId = 0;
  bool                                   IsBoundsChangedEventAllowed{false};
};

#endif // ACCESSIBILITY_INTERNAL_ACCESSIBILITY_BRIDGE_BASE_H
