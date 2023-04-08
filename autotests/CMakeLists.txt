# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

ecm_add_test(namespacetest.cpp
    TEST_NAME
    qalphacloud-namespacetest
    LINK_LIBRARIES
    Qt${QT_MAJOR_VERSION}::Test
    QAlphaCloud
)

ecm_add_test(configurationtest.cpp
    TEST_NAME
    qalphacloud-configurationtest
    LINK_LIBRARIES
    Qt${QT_MAJOR_VERSION}::Test
    QAlphaCloud
)

ecm_add_test(storagesystemsmodeltest.cpp
    testnetworkaccessmanager.cpp
    TEST_NAME
    qalphacloud-storagesystemsmodeltest
    LINK_LIBRARIES
    Qt${QT_MAJOR_VERSION}::Test
    QAlphaCloud
)

ecm_add_test(lastpowerdatatest.cpp
    testnetworkaccessmanager.cpp
    TEST_NAME
    qalphacloud-lastpowerdatatest
    LINK_LIBRARIES
    Qt${QT_MAJOR_VERSION}::Test
    QAlphaCloud
)

ecm_add_test(onedateenergytest.cpp
    testnetworkaccessmanager.cpp
    TEST_NAME
    qalphacloud-onedateenergytest
    LINK_LIBRARIES
    Qt${QT_MAJOR_VERSION}::Test
    QAlphaCloud
)

ecm_add_test(onedaypowermodeltest.cpp
    testnetworkaccessmanager.cpp
    TEST_NAME
    qalphacloud-onedaypowermodeltest
    LINK_LIBRARIES
    Qt${QT_MAJOR_VERSION}::Test
    QAlphaCloud
)
