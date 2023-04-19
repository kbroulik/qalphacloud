# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

add_library(ksystemstats_plugin_qalphacloud MODULE
    plugin.cpp
    plugin.h
    dailydataobject.cpp
    dailydataobject.h
    livedataobject.cpp
    livedataobject.h
    systemobject.cpp
    systemobject.h
)
target_link_libraries(ksystemstats_plugin_qalphacloud
    Qt${QT_MAJOR_VERSION}::Network
    KF${QT_MAJOR_VERSION}::CoreAddons
    KSysGuard::SystemStats
    QAlphaCloud
)
install(TARGETS ksystemstats_plugin_qalphacloud DESTINATION ${KSYSTEMSTATS_PLUGIN_INSTALL_DIR})
