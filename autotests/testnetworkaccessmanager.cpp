/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "testnetworkaccessmanager.h"

TestNetworkAccessManager::TestNetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
}

TestNetworkAccessManager::~TestNetworkAccessManager() = default;

QUrl TestNetworkAccessManager::overrideUrl() const
{
    return m_overrideUrl;
}

void TestNetworkAccessManager::setOverrideUrl(const QUrl &url)
{
    m_overrideUrl = url;
}

QNetworkReply *TestNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    QNetworkRequest newRequest(request);

    newRequest.setUrl(m_overrideUrl);

    return QNetworkAccessManager::createRequest(op, newRequest, outgoingData);
}
