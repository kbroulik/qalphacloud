# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

if(QT_MAJOR_VERSION STREQUAL "6")
  kcmutils_add_qml_kcm(kcm_qalphacloud
    DISABLE_DESKTOP_FILE_GENERATION

    SOURCES
    kcm.cpp
    kcm.h

    INSTALL_NAMESPACE
    "plasma/kcms/kinfocenter"
  )

  target_link_libraries(kcm_qalphacloud KF6::KCMUtilsQuick)
else()
  kcoreaddons_add_plugin(kcm_qalphacloud
    SOURCES
    kcm.cpp
    kcm.h

    INSTALL_NAMESPACE
    "plasma/kcms/kinfocenter"
  )

  target_link_libraries(kcm_qalphacloud KF5::QuickAddons)
endif()

target_link_libraries(kcm_qalphacloud QAlphaCloud)
kpackage_install_package(package kcm_qalphacloud kcms)
