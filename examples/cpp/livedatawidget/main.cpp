/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <QApplication>

#include "livedatawidget.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    LiveDataWidget widget;
    widget.show();

    return app.exec();
}
