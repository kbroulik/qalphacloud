# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

add_executable(livedatawidget)

set(LIVEDATAWIDGET_SRCS
    main.cpp
    livedatawidget.cpp
    livedatawidget.h
)

# No AUTOUIC in ECM yet and qt_wrap_ui needs a variable to write to.
qt_wrap_ui(LIVEDATAWIDGET_SRCS livedatawidget.ui)

target_sources(livedatawidget PRIVATE ${LIVEDATAWIDGET_SRCS})

target_link_libraries(livedatawidget PUBLIC Qt${QT_MAJOR_VERSION}::Network Qt${QT_MAJOR_VERSION}::Widgets qalphacloud)
