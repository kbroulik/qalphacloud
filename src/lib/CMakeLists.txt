# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

add_library(qalphacloud)
add_library(QAlphaCloud ALIAS qalphacloud)

set_target_properties(qalphacloud PROPERTIES
    VERSION     ${QALPHACLOUD_VERSION}
    SOVERSION   ${QALPHACLOUD_SOVERSION}
    EXPORT_NAME QAlphaCloud
)

target_sources(qalphacloud PRIVATE
    qalphacloud.cpp
    qalphacloud.h
    apirequest.cpp
    apirequest.h
    configuration.cpp
    configuration.h
    connector.cpp
    connector.h
    lastpowerdata.cpp
    lastpowerdata.h
    onedateenergy.cpp
    onedateenergy.h
    onedaypowermodel.cpp
    onedaypowermodel.h
    storagesystemsmodel.cpp
    storagesystemsmodel.h
    utils.cpp
    utils_p.h
)

target_include_directories(qalphacloud PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

ecm_qt_declare_logging_category(qalphacloud
    HEADER qalphacloud_log.h
    IDENTIFIER QALPHACLOUD_LOG
    CATEGORY_NAME qalphacloud
)

ecm_generate_export_header(qalphacloud
    BASE_NAME QAlphaCloud
    VERSION ${QALPHACLOUD_VERSION}
)

target_link_libraries(qalphacloud
    PUBLIC
    Qt::Network
)

install(TARGETS qalphacloud EXPORT QAlphaCloudTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

ecm_generate_headers(QAlphaCloud_CamelCase_HEADERS
    HEADER_NAMES
    Configuration
    Connector
    LastPowerData
    OneDateEnergy
    OneDayPowerModel
    QAlphaCloud
    StorageSystemsModel
    REQUIRED_HEADERS QAlphaCloud_HEADERS
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/QAlphaCloud
)

set(_all_headers
    ${QAlphaCloud_HEADERS}
    ${QAlphaCloud_CamelCase_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/qalphacloud_export.h
)

# So that the headers are found at build time
target_include_directories(qalphacloud PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

install(
    FILES ${_all_headers}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QAlphaCloud
    COMPONENT Devel
)
