# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

add_executable(qalphacloud-cli)

set_target_properties(qalphacloud-cli PROPERTIES
# There is no RENAME option for install TARGETS...
    OUTPUT_NAME qalphacloud
)

target_sources(qalphacloud-cli PRIVATE main.cpp)

target_link_libraries(qalphacloud-cli PUBLIC Qt::Core qalphacloud)

install(TARGETS qalphacloud-cli DESTINATION ${CMAKE_INSTALL_BINDIR})
