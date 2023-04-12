# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

cmake_minimum_required(VERSION 3.18)

set(QALPHACLOUD_VERSION 0.0.1)
project(qalphacloud VERSION ${QALPHACLOUD_VERSION})

set(QT_MIN_VERSION "5.15.2")
set(KF_MIN_VERSION "5.80.0")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FeatureSummary)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED
    DESCRIPTION "Extra CMake Modules."
    URL "https://invent.kde.org/frameworks/extra-cmake-modules")
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

include(CMakePackageConfigHelpers)
include(ECMAddTests)
include(ECMGenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMCoverageOption)
include(ECMFindQmlModule)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(QtVersionOption)

option(BUILD_QML "Build QML bindings" ON)
add_feature_info(QML ${BUILD_QML} "QML bindings")

option(BUILD_KSYSTEMSTATS "Build plug-in for KDE's System Monitor" ON)
add_feature_info(KSYSTEMSTATS ${BUILD_KSYSTEMSTATS} "Plug-in for KDE's System Monitor")

option(BUILD_KINFOCENTER "Build plug-in for KDE's Info Center" ON)
add_feature_info(KINFOCENTER ${BUILD_KINFOCENTER} "Plug-in for KDE's Info Center")

option(BUILD_EXAMPLES "Build examples" OFF)
add_feature_info(BUILD_EXAMPLES ${BUILD_EXAMPLES} "Example code")

option(CLAZY_RUN "Clazy run" OFF)
if (CLAZY_RUN)
    add_compile_definitions(-DCLAZY_RUN=1)
    # We want any Clazy warning to be fatal.
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Werror)
endif()

set(API_URL "https://openapi.alphaess.com/api/" CACHE STRING "Default API URL")
add_feature_info(API_URL On "Default API URL is '${API_URL}'")

option(PRESENTATION_BUILD "Hide sensitive information, such as serial numbers, for use in a presentation" OFF)
add_feature_info(PRESENTATION_BUILD ${PRESENTATION_BUILD} "Presentation build")

# Those options come from ECM
add_feature_info(BUILD_TESTING ${BUILD_TESTING} "Unit tests")
add_feature_info(BUILD_COVERAGE ${BUILD_COVERAGE} "Code coverage (gcov)")

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Network)

if (BUILD_QML)
    find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED Qml)
endif()

if (BUILD_KSYSTEMSTATS)
    find_package(KF${QT_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons) # KPluginFactory

    find_package(KSysGuard REQUIRED)
    # TODO upstream this
    set(KSYSTEMSTATS_PLUGIN_INSTALL_DIR ${KDE_INSTALL_PLUGINDIR}/ksystemstats)
endif()

if (BUILD_KINFOCENTER)
    find_package(KF${QT_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons Declarative Package)

    find_package(Qt${QT_MAJOR_VERSION}QuickControls2 ${QT_MIN_VERSION})
    set_package_properties(Qt${QT_MAJOR_VERSION}QuickControls2 PROPERTIES
        TYPE RUNTIME
    )

    ecm_find_qmlmodule(org.kde.kirigami 2.20)
    ecm_find_qmlmodule(org.kde.ksysguard.faces 1.0)
    ecm_find_qmlmodule(org.kde.ksysguard.formatter 1.0)
    ecm_find_qmlmodule(org.kde.quickcharts 1.0)
    ecm_find_qmlmodule(org.kde.quickcharts.controls 1.0)
endif()

ecm_setup_version(PROJECT VARIABLE_PREFIX QALPHACLOUD
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qalphacloud_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/QAlphaCloudConfigVersion.cmake"
    SOVERSION 0
)

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/QAlphaCloud")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/QAlphaCloudConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/QAlphaCloudConfig.cmake"
  INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/QAlphaCloudConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/QAlphaCloudConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT QAlphaCloudTargets
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  FILE QAlphaCloudTargets.cmake
  NAMESPACE QAlphaCloud::
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/qalphacloud_version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QAlphaCloud
    COMPONENT Devel
)

configure_file(config-alphacloud.h.cmake config-alphacloud.h)

# TODO cmake configs and pkgconfig

add_subdirectory(src)

if (BUILD_TESTING)
    find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)
    add_subdirectory(autotests)
endif()

if (BUILD_EXAMPLES)
    find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} CONFIG REQUIRED Widgets)
    add_subdirectory(examples)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
