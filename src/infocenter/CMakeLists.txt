# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

kcoreaddons_add_plugin(kcm_qalphacloud SOURCES
    kcm.cpp
    kcm.h

    INSTALL_NAMESPACE
    "plasma/kcms"
)

target_link_libraries(kcm_qalphacloud
  KF${QT_MAJOR_VERSION}::QuickAddons
  QAlphaCloud
)

kpackage_install_package(package kcm_qalphacloud kcms)
