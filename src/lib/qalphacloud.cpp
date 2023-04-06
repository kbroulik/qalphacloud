/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "qalphacloud.h"

#include <QMetaType>

namespace QAlphaCloud
{

struct InitQAlphaCloud {
    InitQAlphaCloud()
    {
    }
};

static InitQAlphaCloud s_init;

} // namespace QAlphaCloud
