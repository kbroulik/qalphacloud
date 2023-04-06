/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QByteArray>
#include <QHash>

class QAbstractItemModel;
class QMetaEnum;

namespace QAlphaCloud
{

namespace Utils
{

QHash<int, QByteArray> roleNamesFromEnum(const QMetaEnum &metaEnum);

template<typename FieldType, typename EmitterType>
bool updateField(FieldType &field, const FieldType &newValue, EmitterType *emitter, void (EmitterType::*changeSignal)(FieldType))
{
    if (field != newValue) {
        field = newValue;
        Q_EMIT((emitter)->*changeSignal)(newValue);
        return true;
    }
    return false;
}

} // namespace Utils

} // namespace QAlphaCloud
