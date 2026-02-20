CMAKE_MINIMUM_REQUIRED( VERSION 3.8.2 )

CMAKE_POLICY( SET CMP0012 NEW ) # Avoids the if() bug

SET( CMAKE_CXX_STANDARD 17 )

PROJECT( accessibility-common )

SET( name "accessibility-common" )
SET( DALI_ACCESSIBILITY_VERSION 0.1.0 )

# Build options
OPTION( ENABLE_ACCESSIBILITY       "Enable accessibility (bridge, service, IPC)" ON )
OPTION( ENABLE_TIDL        "Use TIDL (rpc_port) instead of D-Bus" OFF )
OPTION( ENABLE_PKG_CONFIGURE "Use pkgconfig" ON )
OPTION( ENABLE_EXPORTALL   "Enable Export all symbols" OFF )
OPTION( ENABLE_DEBUG       "Enable Debug" OFF )
OPTION( ENABLE_COVERAGE    "Coverage" OFF )

SET( PREFIX ${CMAKE_INSTALL_PREFIX} )
SET( EXEC_PREFIX ${CMAKE_INSTALL_PREFIX} )

IF( NOT LIB_DIR )
  SET( LIB_DIR ${CMAKE_INSTALL_LIBDIR} )
ENDIF()
IF( NOT INCLUDE_DIR )
  SET( INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR} )
ENDIF()

IF( ENABLE_PKG_CONFIGURE )
  INCLUDE( FindPkgConfig )
ENDIF()

# Optional dependency: eldbus (for AT-SPI D-Bus on Tizen/Linux)
IF( ENABLE_PKG_CONFIGURE )
  PKG_CHECK_MODULES( ELDBUS eldbus )
  IF( ELDBUS_FOUND )
    SET( eldbus_available ON )
  ELSE()
    SET( eldbus_available OFF )
  ENDIF()

  PKG_CHECK_MODULES( GIO gio-2.0 )
  IF( GIO_FOUND )
    SET( gdbus_available ON )
  ELSE()
    SET( gdbus_available OFF )
  ENDIF()

  PKG_CHECK_MODULES( RPC_PORT rpc-port )
  IF( RPC_PORT_FOUND )
    SET( tidl_available ON )
  ELSE()
    SET( tidl_available OFF )
  ENDIF()
ELSE()
  SET( eldbus_available OFF )
  SET( gdbus_available OFF )
  SET( tidl_available OFF )
ENDIF()

# Source tree path variables
SET( accessibility_common_root ${CMAKE_CURRENT_SOURCE_DIR}/../../ )
SET( accessibility_common_api_dir ${accessibility_common_root}/accessibility/api )
SET( accessibility_common_public_api_dir ${accessibility_common_root}/accessibility/public-api )
SET( accessibility_common_internal_dir ${accessibility_common_root}/accessibility/internal )

# Include file lists
INCLUDE( ${accessibility_common_api_dir}/file.list )
INCLUDE( ${accessibility_common_internal_dir}/bridge/file.list )

# Collect sources
SET( SOURCES
  ${accessibility_common_api_src_files}
  ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
)

IF( ENABLE_ACCESSIBILITY )
  SET( SOURCES ${SOURCES} ${accessibility_common_atspi_bridge_src_files} )
  SET( SOURCES ${SOURCES} ${accessibility_common_service_src_files} )
  IF( ENABLE_TIDL )
    # TIDL backend: direct P2P via rpc_port (Tizen native)
    SET( SOURCES ${SOURCES} ${accessibility_common_tidl_src_files} )
    # TIDL still needs a D-Bus stub for types/serialization used in bridge modules
    SET( SOURCES ${SOURCES} ${accessibility_common_dbus_stub_src_files} )
    SET( DBUS_BACKEND "tidl (Tizen native P2P)" )
    ADD_DEFINITIONS( -DENABLE_TIDL_BACKEND )
    IF( tidl_available )
      INCLUDE_DIRECTORIES( ${RPC_PORT_INCLUDE_DIRS} )
    ENDIF()
  ELSEIF( eldbus_available )
    SET( SOURCES ${SOURCES} ${accessibility_common_dbus_tizen_src_files} )
    SET( DBUS_BACKEND "eldbus (EFL/Tizen)" )
  ELSEIF( gdbus_available )
    SET( SOURCES ${SOURCES} ${accessibility_common_dbus_gdbus_src_files} )
    SET( DBUS_BACKEND "gdbus (GLib/GIO)" )
  ELSE()
    SET( SOURCES ${SOURCES} ${accessibility_common_dbus_stub_src_files} )
    SET( DBUS_BACKEND "stub (no D-Bus)" )
  ENDIF()
ELSE()
  SET( SOURCES ${SOURCES} ${accessibility_common_atspi_dummy_src_files} )
  SET( DBUS_BACKEND "none (accessibility disabled)" )
ENDIF()

# Include directories -- no external toolkit dependencies
INCLUDE_DIRECTORIES(
  ${accessibility_common_root}
)

# CFLAGS
SET( DALI_ACCESSIBILITY_CFLAGS
  -Wall
)

IF( enable_debug )
  ADD_DEFINITIONS( -DDEBUG_ENABLED )
  SET( ENABLE_EXPORTALL ON )
ENDIF()

IF( NOT ${ENABLE_EXPORTALL} )
  ADD_DEFINITIONS( "-fvisibility=hidden -DHIDE_ACCESSIBILITY_INTERNALS" )
ENDIF()

IF( ENABLE_COVERAGE OR "$ENV{CXXFLAGS}" MATCHES --coverage )
  ADD_COMPILE_OPTIONS( --coverage )
  SET( ENABLE_COVERAGE ON )
  SET( COVERAGE --coverage )
ENDIF()

ADD_COMPILE_OPTIONS( ${DALI_ACCESSIBILITY_CFLAGS} )

# Build the shared library
SET( LIBTYPE SHARED )
ADD_LIBRARY( ${name} ${LIBTYPE} ${SOURCES} )

TARGET_COMPILE_DEFINITIONS( ${name} PRIVATE BUILDING_ACCESSIBILITY_COMMON )

# Link dependencies -- no external toolkit libraries required
SET( DALI_ACCESSIBILITY_LDFLAGS "" )

IF( ENABLE_ACCESSIBILITY AND ENABLE_TIDL AND tidl_available )
  SET( DALI_ACCESSIBILITY_LDFLAGS ${DALI_ACCESSIBILITY_LDFLAGS}
    ${RPC_PORT_LDFLAGS}
  )
ELSEIF( ENABLE_ACCESSIBILITY AND eldbus_available AND NOT ENABLE_TIDL )
  SET( DALI_ACCESSIBILITY_LDFLAGS ${DALI_ACCESSIBILITY_LDFLAGS}
    ${ELDBUS_LDFLAGS}
  )
  INCLUDE_DIRECTORIES( ${ELDBUS_INCLUDE_DIRS} )
ELSEIF( ENABLE_ACCESSIBILITY AND gdbus_available AND NOT ENABLE_TIDL )
  SET( DALI_ACCESSIBILITY_LDFLAGS ${DALI_ACCESSIBILITY_LDFLAGS}
    ${GIO_LDFLAGS}
  )
  INCLUDE_DIRECTORIES( ${GIO_INCLUDE_DIRS} )
ENDIF()

IF( UNIX )
  FIND_LIBRARY( PTHREAD pthread )
  IF( PTHREAD )
    SET( DALI_ACCESSIBILITY_LDFLAGS ${DALI_ACCESSIBILITY_LDFLAGS} -lpthread )
  ENDIF()
ENDIF()

TARGET_LINK_LIBRARIES( ${name}
  ${DALI_ACCESSIBILITY_LDFLAGS}
  ${COVERAGE}
)

# Set library version
SET_TARGET_PROPERTIES( ${name} PROPERTIES
  VERSION ${DALI_ACCESSIBILITY_VERSION}
  SOVERSION ${DALI_ACCESSIBILITY_VERSION}
  CLEAN_DIRECT_OUTPUT 1
)

# Install the library
INSTALL( TARGETS ${name} DESTINATION ${LIB_DIR} )

# Install headers
SET( DEV_INCLUDE_PATH ${INCLUDE_DIR} )
SET( accessibilityapidir ${DEV_INCLUDE_PATH}/accessibility/api )
SET( accessibilitypublicapidir ${DEV_INCLUDE_PATH}/accessibility/public-api )

INSTALL( FILES ${accessibility_common_api_header_files} DESTINATION ${accessibilityapidir} )
INSTALL( FILES ${accessibility_common_public_api_dir}/accessibility-common.h DESTINATION ${accessibilitypublicapidir} )

# Install internal headers needed by toolkit adaptors for platform callbacks registration
SET( accessibilityinternalbridgedir ${DEV_INCLUDE_PATH}/accessibility/internal/bridge )
INSTALL( FILES ${accessibility_common_internal_dir}/bridge/bridge-platform.h
               ${accessibility_common_internal_dir}/bridge/collection-impl.h
         DESTINATION ${accessibilityinternalbridgedir} )

# Configure and install pkg-config file
IF( ENABLE_PKG_CONFIGURE )
  SET( PKG_CFG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/accessibility-common.pc )
  CONFIGURE_FILE( ${PKG_CFG_FILE}.in ${PKG_CFG_FILE} @ONLY )
  INSTALL( FILES ${PKG_CFG_FILE} DESTINATION ${LIB_DIR}/pkgconfig )
ENDIF()

# Test application
# Tests compile bridge sources directly (not via shared lib) because the
# library hides internal symbols with -fvisibility=hidden.
OPTION( BUILD_TESTS "Build test applications" OFF )
IF( BUILD_TESTS )
  SET( TEST_BRIDGE_SOURCES
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_stub_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
  )
  IF( ENABLE_TIDL )
    SET( TEST_BRIDGE_SOURCES ${TEST_BRIDGE_SOURCES} ${accessibility_common_tidl_src_files} )
  ENDIF()
  ADD_EXECUTABLE( accessibility-test
    ${TEST_BRIDGE_SOURCES}
    ${accessibility_common_root}/test/mock/mock-dbus-wrapper.cpp
    ${accessibility_common_root}/test/test-accessible.cpp
    ${accessibility_common_root}/test/test-app.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-test PRIVATE ${accessibility_common_root} )
  IF( ENABLE_TIDL )
    TARGET_COMPILE_DEFINITIONS( accessibility-test PRIVATE ENABLE_TIDL_BACKEND )
  ENDIF()
ENDIF()

# AccessibilityService unit tests (mock-based, no D-Bus dependency)
OPTION( BUILD_SERVICE_TESTS "Build AccessibilityService unit tests" OFF )
IF( BUILD_SERVICE_TESTS )
  SET( SERVICE_TEST_BRIDGE_SOURCES
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_stub_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
    ${accessibility_common_service_src_files}
  )
  ADD_EXECUTABLE( accessibility-service-test
    ${SERVICE_TEST_BRIDGE_SOURCES}
    ${accessibility_common_root}/test/mock/mock-dbus-wrapper.cpp
    ${accessibility_common_root}/test/test-accessible.cpp
    ${accessibility_common_root}/test/test-service.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-service-test PRIVATE ${accessibility_common_root} )
ENDIF()

# Accessibility Inspector
OPTION( BUILD_INSPECTOR "Build accessibility inspector tool" OFF )
IF( BUILD_INSPECTOR )
  SET( INSPECTOR_BRIDGE_SOURCES
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_stub_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
  )
  ADD_EXECUTABLE( accessibility-inspector
    ${INSPECTOR_BRIDGE_SOURCES}
    ${accessibility_common_root}/test/mock/mock-dbus-wrapper.cpp
    ${accessibility_common_root}/test/test-accessible.cpp
    ${accessibility_common_root}/tools/inspector/query-engine.cpp
    ${accessibility_common_root}/tools/inspector/inspector.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-inspector PRIVATE ${accessibility_common_root} )
  IF( APPLE )
    TARGET_SOURCES( accessibility-inspector PRIVATE
      ${accessibility_common_root}/tools/inspector/tts-mac.mm
    )
    TARGET_LINK_LIBRARIES( accessibility-inspector "-framework AVFoundation" "-framework Foundation" objc )
  ELSE()
    TARGET_SOURCES( accessibility-inspector PRIVATE
      ${accessibility_common_root}/tools/inspector/tts-stub.cpp
    )
  ENDIF()
ENDIF()

# Web-based Accessibility Inspector
OPTION( BUILD_WEB_INSPECTOR "Build web-based accessibility inspector tool" OFF )
IF( BUILD_WEB_INSPECTOR )
  SET( WEB_INSPECTOR_BRIDGE_SOURCES
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_stub_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
  )
  ADD_EXECUTABLE( accessibility-web-inspector
    ${WEB_INSPECTOR_BRIDGE_SOURCES}
    ${accessibility_common_root}/test/mock/mock-dbus-wrapper.cpp
    ${accessibility_common_root}/test/test-accessible.cpp
    ${accessibility_common_root}/tools/inspector/query-engine.cpp
    ${accessibility_common_root}/tools/inspector/web-inspector.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-web-inspector PRIVATE
    ${accessibility_common_root}
    ${accessibility_common_root}/third-party
  )
  FIND_PACKAGE( Threads REQUIRED )
  TARGET_LINK_LIBRARIES( accessibility-web-inspector Threads::Threads )
ENDIF()

# Embeddable Inspector Library (static lib for use by external apps)
OPTION( BUILD_INSPECTOR_LIB "Build embeddable inspector library" OFF )
IF( BUILD_INSPECTOR_LIB )
  FIND_PACKAGE( Threads REQUIRED )
  ADD_LIBRARY( accessibility-inspector STATIC
    ${accessibility_common_root}/tools/inspector/direct-query-engine.cpp
    ${accessibility_common_root}/tools/inspector/web-inspector-server.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-inspector PUBLIC
    ${accessibility_common_root}
    ${accessibility_common_root}/third-party
  )
  TARGET_LINK_LIBRARIES( accessibility-inspector ${name} Threads::Threads )
  INSTALL( TARGETS accessibility-inspector DESTINATION ${LIB_DIR} )

  # Install inspector headers for consumers
  SET( inspectorincdir ${DEV_INCLUDE_PATH}/tools/inspector )
  INSTALL( FILES
    ${accessibility_common_root}/tools/inspector/inspector-types.h
    ${accessibility_common_root}/tools/inspector/direct-query-engine.h
    ${accessibility_common_root}/tools/inspector/web-inspector-server.h
    ${accessibility_common_root}/tools/inspector/web-inspector-resources.h
    DESTINATION ${inspectorincdir} )
  INSTALL( FILES ${accessibility_common_root}/third-party/cpp-httplib/httplib.h
    DESTINATION ${DEV_INCLUDE_PATH}/cpp-httplib )
ENDIF()

# InspectorService unit tests
OPTION( BUILD_INSPECTOR_SERVICE_TESTS "Build InspectorService unit tests" OFF )
IF( BUILD_INSPECTOR_SERVICE_TESTS )
  FIND_PACKAGE( Threads REQUIRED )
  SET( INSPECTOR_SERVICE_TEST_SOURCES
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_stub_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
    ${accessibility_common_service_src_files}
    ${accessibility_common_internal_dir}/service/inspector-service.cpp
    ${accessibility_common_root}/tools/inspector/node-proxy-query-engine.cpp
    ${accessibility_common_root}/tools/inspector/web-inspector-server.cpp
  )
  ADD_EXECUTABLE( accessibility-inspector-service-test
    ${INSPECTOR_SERVICE_TEST_SOURCES}
    ${accessibility_common_root}/test/mock/mock-dbus-wrapper.cpp
    ${accessibility_common_root}/test/test-accessible.cpp
    ${accessibility_common_root}/test/test-inspector-service.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-inspector-service-test PRIVATE
    ${accessibility_common_root}
    ${accessibility_common_root}/third-party
  )
  TARGET_LINK_LIBRARIES( accessibility-inspector-service-test Threads::Threads )
ENDIF()

# InspectorService standalone binary
OPTION( BUILD_INSPECTOR_SERVICE "Build InspectorService standalone binary" OFF )
IF( BUILD_INSPECTOR_SERVICE )
  FIND_PACKAGE( Threads REQUIRED )
  SET( INSPECTOR_SERVICE_SOURCES
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_stub_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
    ${accessibility_common_service_src_files}
    ${accessibility_common_internal_dir}/service/inspector-service.cpp
    ${accessibility_common_root}/tools/inspector/node-proxy-query-engine.cpp
    ${accessibility_common_root}/tools/inspector/web-inspector-server.cpp
  )
  ADD_EXECUTABLE( accessibility-inspector-service
    ${INSPECTOR_SERVICE_SOURCES}
    ${accessibility_common_root}/test/mock/mock-dbus-wrapper.cpp
    ${accessibility_common_root}/test/test-accessible.cpp
    ${accessibility_common_root}/tools/inspector/inspector-service-main.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-inspector-service PRIVATE
    ${accessibility_common_root}
    ${accessibility_common_root}/third-party
  )
  TARGET_LINK_LIBRARIES( accessibility-inspector-service Threads::Threads )
ENDIF()

# GDBus backend tests (require dbus-daemon and gio-2.0)
OPTION( BUILD_GDBUS_TESTS "Build GDBus backend tests (requires dbus-daemon)" OFF )
IF( BUILD_GDBUS_TESTS )
  IF( NOT ENABLE_PKG_CONFIGURE )
    MESSAGE( FATAL_ERROR "BUILD_GDBUS_TESTS requires ENABLE_PKG_CONFIGURE=ON (needs gio-2.0)" )
  ENDIF()
  IF( NOT GIO_FOUND )
    MESSAGE( FATAL_ERROR "BUILD_GDBUS_TESTS requires gio-2.0 (brew install dbus on macOS)" )
  ENDIF()

  SET( GDBUS_TEST_BRIDGE_SOURCES
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_gdbus_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
  )

  ADD_EXECUTABLE( accessibility-gdbus-unit-test
    ${GDBUS_TEST_BRIDGE_SOURCES}
    ${accessibility_common_root}/test/gdbus/test-gdbus-unit.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-gdbus-unit-test PRIVATE
    ${accessibility_common_root}
    ${GIO_INCLUDE_DIRS}
  )
  TARGET_LINK_LIBRARIES( accessibility-gdbus-unit-test ${GIO_LDFLAGS} )

  ADD_EXECUTABLE( accessibility-gdbus-integration-test
    ${GDBUS_TEST_BRIDGE_SOURCES}
    ${accessibility_common_root}/test/test-accessible.cpp
    ${accessibility_common_root}/test/gdbus/test-gdbus-integration.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-gdbus-integration-test PRIVATE
    ${accessibility_common_root}
    ${GIO_INCLUDE_DIRS}
  )
  TARGET_LINK_LIBRARIES( accessibility-gdbus-integration-test ${GIO_LDFLAGS} )
ENDIF()

# Direct Web Inspector (standalone binary using DirectQueryEngine)
OPTION( BUILD_WEB_INSPECTOR_DIRECT "Build direct web inspector (no D-Bus)" OFF )
IF( BUILD_WEB_INSPECTOR_DIRECT )
  SET( DIRECT_INSPECTOR_BRIDGE_SOURCES
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_stub_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
  )
  ADD_EXECUTABLE( accessibility-web-inspector-direct
    ${DIRECT_INSPECTOR_BRIDGE_SOURCES}
    ${accessibility_common_root}/test/mock/mock-dbus-wrapper.cpp
    ${accessibility_common_root}/test/test-accessible.cpp
    ${accessibility_common_root}/tools/inspector/direct-query-engine.cpp
    ${accessibility_common_root}/tools/inspector/web-inspector-server.cpp
    ${accessibility_common_root}/tools/inspector/web-inspector-direct-main.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-web-inspector-direct PRIVATE
    ${accessibility_common_root}
    ${accessibility_common_root}/third-party
  )
  FIND_PACKAGE( Threads REQUIRED )
  TARGET_LINK_LIBRARIES( accessibility-web-inspector-direct Threads::Threads )
ENDIF()

# GDBus Web Inspector (real D-Bus IPC)
OPTION( BUILD_WEB_INSPECTOR_GDBUS "Build GDBus web inspector (real D-Bus)" OFF )
IF( BUILD_WEB_INSPECTOR_GDBUS )
  IF( NOT GIO_FOUND )
    MESSAGE( FATAL_ERROR "BUILD_WEB_INSPECTOR_GDBUS requires gio-2.0 (brew install dbus on macOS)" )
  ENDIF()

  FIND_PACKAGE( Threads REQUIRED )

  SET( GDBUS_WEB_INSPECTOR_SOURCES
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_gdbus_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
  )

  ADD_EXECUTABLE( accessibility-web-inspector-gdbus
    ${GDBUS_WEB_INSPECTOR_SOURCES}
    ${accessibility_common_root}/test/test-accessible.cpp
    ${accessibility_common_root}/tools/inspector/query-engine.cpp
    ${accessibility_common_root}/tools/inspector/web-inspector-gdbus-main.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-web-inspector-gdbus PRIVATE
    ${accessibility_common_root}
    ${accessibility_common_root}/third-party
    ${GIO_INCLUDE_DIRS}
  )
  TARGET_COMPILE_DEFINITIONS( accessibility-web-inspector-gdbus PRIVATE INSPECTOR_NO_MOCK_DBUS )
  TARGET_LINK_LIBRARIES( accessibility-web-inspector-gdbus ${GIO_LDFLAGS} Threads::Threads )
ENDIF()

# ScreenReaderService unit tests
OPTION( BUILD_SCREEN_READER_TESTS "Build ScreenReaderService unit tests" OFF )
IF( BUILD_SCREEN_READER_TESTS )
  SET( SCREEN_READER_TEST_SOURCES
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_stub_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
    ${accessibility_common_service_src_files}
    ${accessibility_common_screen_reader_src_files}
  )
  ADD_EXECUTABLE( accessibility-screen-reader-test
    ${SCREEN_READER_TEST_SOURCES}
    ${accessibility_common_root}/test/mock/mock-dbus-wrapper.cpp
    ${accessibility_common_root}/test/test-accessible.cpp
    ${accessibility_common_root}/test/test-screen-reader-service.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( accessibility-screen-reader-test PRIVATE ${accessibility_common_root} )
ENDIF()

# Screen Reader Demo (requires DALi — real app with embedded ScreenReaderService)
SET( DESKTOP_PREFIX "$ENV{HOME}/tizen/dali-env" )
OPTION( BUILD_SCREEN_READER_DEMO "Build screen reader demo (requires DALi)" OFF )
IF( BUILD_SCREEN_READER_DEMO )
  FIND_LIBRARY( DALI_CORE_LIB dali2-core PATHS ${DESKTOP_PREFIX}/lib )
  FIND_LIBRARY( DALI_ADAPTOR_LIB dali2-adaptor PATHS ${DESKTOP_PREFIX}/lib )
  FIND_LIBRARY( DALI_TOOLKIT_LIB dali2-toolkit PATHS ${DESKTOP_PREFIX}/lib )

  IF( NOT DALI_CORE_LIB OR NOT DALI_ADAPTOR_LIB OR NOT DALI_TOOLKIT_LIB )
    MESSAGE( FATAL_ERROR "BUILD_SCREEN_READER_DEMO requires DALi (dali2-core, dali2-adaptor, dali2-toolkit) in ${DESKTOP_PREFIX}/lib" )
  ENDIF()

  SET( SCREEN_READER_DEMO_SOURCES
    ${accessibility_common_root}/tools/screen-reader/screen-reader-demo.cpp
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_stub_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
    ${accessibility_common_service_src_files}
    ${accessibility_common_screen_reader_src_files}
  )

  ADD_EXECUTABLE( accessibility-screen-reader-demo ${SCREEN_READER_DEMO_SOURCES} )
  TARGET_INCLUDE_DIRECTORIES( accessibility-screen-reader-demo PRIVATE
    ${accessibility_common_root}
    ${DESKTOP_PREFIX}/include
    ${DESKTOP_PREFIX}/include/dali
  )
  TARGET_LINK_LIBRARIES( accessibility-screen-reader-demo
    ${DALI_CORE_LIB}
    ${DALI_ADAPTOR_LIB}
    ${DALI_TOOLKIT_LIB}
  )

  IF( APPLE )
    TARGET_SOURCES( accessibility-screen-reader-demo PRIVATE
      ${accessibility_common_root}/tools/screen-reader/mac-tts-engine.mm
    )
    TARGET_LINK_LIBRARIES( accessibility-screen-reader-demo
      "-framework AVFoundation" "-framework Foundation" objc
    )
  ELSE()
    TARGET_SOURCES( accessibility-screen-reader-demo PRIVATE
      ${accessibility_common_root}/tools/screen-reader/espeak-tts-engine.cpp
    )
    TARGET_LINK_LIBRARIES( accessibility-screen-reader-demo espeak-ng )
  ENDIF()

  MESSAGE( STATUS "Screen Reader Demo: DALi core=${DALI_CORE_LIB}" )
  MESSAGE( STATUS "                    DALi adaptor=${DALI_ADAPTOR_LIB}" )
  MESSAGE( STATUS "                    DALi toolkit=${DALI_TOOLKIT_LIB}" )
ENDIF()

# TV Screen Reader Demo (requires DALi — KeyboardFocusManager + TvScreenReaderService)
OPTION( BUILD_SCREEN_READER_TV_DEMO "Build TV screen reader demo (requires DALi)" OFF )
IF( BUILD_SCREEN_READER_TV_DEMO )
  FIND_LIBRARY( DALI_CORE_LIB dali2-core PATHS ${DESKTOP_PREFIX}/lib )
  FIND_LIBRARY( DALI_ADAPTOR_LIB dali2-adaptor PATHS ${DESKTOP_PREFIX}/lib )
  FIND_LIBRARY( DALI_TOOLKIT_LIB dali2-toolkit PATHS ${DESKTOP_PREFIX}/lib )

  IF( NOT DALI_CORE_LIB OR NOT DALI_ADAPTOR_LIB OR NOT DALI_TOOLKIT_LIB )
    MESSAGE( FATAL_ERROR "BUILD_SCREEN_READER_TV_DEMO requires DALi (dali2-core, dali2-adaptor, dali2-toolkit) in ${DESKTOP_PREFIX}/lib" )
  ENDIF()

  SET( SCREEN_READER_TV_DEMO_SOURCES
    ${accessibility_common_root}/tools/screen-reader/screen-reader-tv-demo.cpp
    ${accessibility_common_atspi_bridge_src_files}
    ${accessibility_common_dbus_stub_src_files}
    ${accessibility_common_api_src_files}
    ${accessibility_common_internal_dir}/bridge/bridge-platform.cpp
    ${accessibility_common_service_src_files}
    ${accessibility_common_screen_reader_src_files}
  )

  ADD_EXECUTABLE( accessibility-screen-reader-tv-demo ${SCREEN_READER_TV_DEMO_SOURCES} )
  TARGET_INCLUDE_DIRECTORIES( accessibility-screen-reader-tv-demo PRIVATE
    ${accessibility_common_root}
    ${DESKTOP_PREFIX}/include
    ${DESKTOP_PREFIX}/include/dali
  )
  TARGET_LINK_LIBRARIES( accessibility-screen-reader-tv-demo
    ${DALI_CORE_LIB}
    ${DALI_ADAPTOR_LIB}
    ${DALI_TOOLKIT_LIB}
  )

  IF( APPLE )
    TARGET_SOURCES( accessibility-screen-reader-tv-demo PRIVATE
      ${accessibility_common_root}/tools/screen-reader/mac-tts-engine.mm
    )
    TARGET_LINK_LIBRARIES( accessibility-screen-reader-tv-demo
      "-framework AVFoundation" "-framework Foundation" objc
    )
  ELSE()
    TARGET_SOURCES( accessibility-screen-reader-tv-demo PRIVATE
      ${accessibility_common_root}/tools/screen-reader/espeak-tts-engine.cpp
    )
    TARGET_LINK_LIBRARIES( accessibility-screen-reader-tv-demo espeak-ng )
  ENDIF()

  MESSAGE( STATUS "TV Screen Reader Demo: DALi core=${DALI_CORE_LIB}" )
  MESSAGE( STATUS "                       DALi adaptor=${DALI_ADAPTOR_LIB}" )
  MESSAGE( STATUS "                       DALi toolkit=${DALI_TOOLKIT_LIB}" )
ENDIF()

MESSAGE( STATUS "" )
MESSAGE( STATUS "Configuration:" )
MESSAGE( STATUS "  Prefix:                   ${PREFIX}" )
MESSAGE( STATUS "  Enable Accessibility:     ${ENABLE_ACCESSIBILITY}" )
MESSAGE( STATUS "  Enable TIDL:              ${ENABLE_TIDL}" )
MESSAGE( STATUS "  IPC backend:              ${DBUS_BACKEND}" )
MESSAGE( STATUS "  Enable Debug:             ${ENABLE_DEBUG}" )
MESSAGE( STATUS "  Enable Coverage:          ${ENABLE_COVERAGE}" )
MESSAGE( STATUS "  Build Tests:              ${BUILD_TESTS}" )
MESSAGE( STATUS "  Build Inspector:          ${BUILD_INSPECTOR}" )
MESSAGE( STATUS "  Build Web Inspector:      ${BUILD_WEB_INSPECTOR}" )
MESSAGE( STATUS "  Build Inspector Lib:      ${BUILD_INSPECTOR_LIB}" )
MESSAGE( STATUS "  Build Direct Inspector:   ${BUILD_WEB_INSPECTOR_DIRECT}" )
MESSAGE( STATUS "  Build GDBus Tests:        ${BUILD_GDBUS_TESTS}" )
MESSAGE( STATUS "  Build GDBus Web Inspector: ${BUILD_WEB_INSPECTOR_GDBUS}" )
MESSAGE( STATUS "  Build Service Tests:      ${BUILD_SERVICE_TESTS}" )
MESSAGE( STATUS "  Build Inspector Service Tests: ${BUILD_INSPECTOR_SERVICE_TESTS}" )
MESSAGE( STATUS "  Build Inspector Service:  ${BUILD_INSPECTOR_SERVICE}" )
MESSAGE( STATUS "  Build Screen Reader Tests: ${BUILD_SCREEN_READER_TESTS}" )
MESSAGE( STATUS "  Build Screen Reader Demo: ${BUILD_SCREEN_READER_DEMO}" )
MESSAGE( STATUS "  Build TV Screen Reader Demo: ${BUILD_SCREEN_READER_TV_DEMO}" )
MESSAGE( STATUS "" )
