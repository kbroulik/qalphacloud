# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

add_library(qalphacloudqmlplugin SHARED
    qmlplugin.cpp
)
target_link_libraries(qalphacloudqmlplugin PRIVATE Qt${QT_MAJOR_VERSION}::Qml QAlphaCloud)
install(TARGETS qalphacloudqmlplugin DESTINATION ${KDE_INSTALL_QMLDIR}/de/broulik/qalphacloud)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/de/broulik/qalphacloud)
