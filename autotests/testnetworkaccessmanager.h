/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QNetworkAccessManager>

class TestNetworkAccessManager : public QNetworkAccessManager
{

public:
    TestNetworkAccessManager(QObject *parent = nullptr);
    ~TestNetworkAccessManager() override;

    QUrl overrideUrl() const;
    void setOverrideUrl(const QUrl &url);

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request,
                                 QIODevice *outgoingData = nullptr) override;

private:
    QUrl m_overrideUrl;
};
