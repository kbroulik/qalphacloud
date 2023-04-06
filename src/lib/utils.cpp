/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "utils_p.h"

#include <QMetaEnum>

namespace QAlphaCloud
{

namespace Utils
{

QHash<int, QByteArray> roleNamesFromEnum(const QMetaEnum &metaEnum)
{
    Q_ASSERT(metaEnum.isValid());

    QHash<int, QByteArray> roleNames;

    roleNames.reserve(metaEnum.keyCount());
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        QByteArray key = metaEnum.key(i);
        key[0] = key.at(0) + 32; // yuck, toLowerCase().

        roleNames.insert(metaEnum.value(i), key);
    }

    return roleNames;
}

} // namespace Utils

} // namespace QAlphaCloud
