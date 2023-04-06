# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

add_subdirectory(lib)
add_subdirectory(cli)

if (BUILD_QML)
    add_subdirectory(qml)
endif()

if (BUILD_KSYSTEMSTATS)
    add_subdirectory(systemstats)
endif()

if (BUILD_KINFOCENTER)
    add_subdirectory(infocenter)
endif()
