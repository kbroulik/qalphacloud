<!--
 SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 SPDX-License-Identifier: BSD-2-Clause
-->

# :sun_with_face: QAlphaCloud

[![Build Status](https://github.com/kbroulik/qalphacloud/actions/workflows/build-and-test.yml/badge.svg?branch=master)](https://github.com/kbroulik/qalphacloud/actions/workflows/build-and-test.yml?query=branch%3Amaster) [![REUSE compliance](https://github.com/kbroulik/qalphacloud/actions/workflows/reuse.yml/badge.svg?branch=master)](https://github.com/kbroulik/qalphacloud/actions/workflows/reuse.yml?query=branch%3Amaster) [![Doxygen status](https://github.com/kbroulik/qalphacloud/actions/workflows/doxygen.yml/badge.svg?branch=master)](https://github.com/kbroulik/qalphacloud/actions/workflows/doxygen.yml?query=branch%3Amaster)

*Qt bindings for the [AlphaCloud API](https://github.com/alphaess-developer/alphacloud_open_api) by Alpha ESS Co., Ltd.*

## Table of Contents

<!-- Can this be auto-generated? -->
- [Features](#features)
  - [libqalphacloud](#libqalphacloud)
  - [KSystemStats plug-in](#ksystemstats-plug-in)
  - [KInfoCenter Module](#kinfocenter-module)
  - [Command Line Interface](#command-line-interface)
- [Why?](#nerd_face-why)
- [Getting Started](#hammer-getting-started)
  - [API Keys](#api-keys)
  - [Build](#build)
  - [Configure Options](#configure-options)
  - [Logging Categories](#logging-categories)
  - [Dependencies](#dependencies)
- [API](#electric_plug-api)
  - [Classes](#classes)
  - [Examples](#examples)
- [To Do](#building_construction-to-do)
- [Licensing](#paperclips-licensing)

## Features

### libqalphacloud

A Qt library around the AlphaCloud API. It provides both C++ and QML APIs for fetching data.

### KSystemStats plug-in

A plug-in for feeding data into [KDE’s System Monitor](https://apps.kde.org/plasma-systemmonitor/).

It enables you to create both complex monitor pages for all aspects of your solar and energy storage installation as well as add relevant meters right to your desktop.

![Screenshot of Plasma System Monitor showing various line charts with live photovoltaic information](https://github.com/kbroulik/qalphacloud/raw/master/artwork/Screenshot_systemmonitor.png?raw=true "Plasma System Monitor plug-in")
*How cool is that? You can have both CPU load and photovoltaic energy production on the same page!*

### KInfoCenter module

A module for displaying historic energy production and consumption data in KInfoCenter.

![Screenshot of KInfoCenter module showing historic energy production data and live solar information](https://github.com/kbroulik/qalphacloud/raw/master/artwork/Screenshot_infocenter.png?raw=true "KInfoCenter module")
*KInfoCenter module showing both live and historic photovoltaic information*

### Command-Line Interface

The library comes with a command-line interface (CLI) for easily querying data and forwarding it to your own scripts and helpers in either human-readable or JSON formats.

```shell
$ qalphacloud essList
QAlphaCloud CLI
  API URL: https://openapi.alphaess.com/api/

List storage systems:
1 storage system(s) found:

SerialNumber: ABCDEF
Status: Normal
InverterModel: INV
InverterPower: 10000
BatteryModel: BAT
BatteryGrossCapacity: 5000
BatteryRemainingCapacity: 4750
BatteryUsableCapacity: 95
PhotovoltaicPower: 10000
```

The `--help` command describes all supported arguments and endpoints.

## :nerd_face: Why?

When I learned that there is an API for my solar installation, I immediately wanted to write a widget for the [KDE Plasma Desktop](https://kde.org/plasma-desktop/) so I could see live data anytime on my panel.

Over the course of just a few evenings I ended up creating a proper shared library that makes use of all the current best practices (automated tests and code coverage checks, full [REUSE-compliance](https://reuse.software/), documentation, API/ABI-compatibility guarantees, C++ _and_ QML bindings, etc.) to show off how simple it is for a 3rd party product to make use of KDE’s libraries and how versatile they are.

KDE’s [Extra CMake Modules](https://api.kde.org/frameworks/extra-cmake-modules/html/index.html) make creating libraries with CMake a lot nicer and their powerful yet easy-to-use [KQuickCharts](https://api.kde.org/frameworks/kquickcharts/html/index.html) Framework produces beautiful and fully hardware-accelerated diagrams. The _KSystemStats_ daemon is then used to feed data into [System Monitor](https://apps.kde.org/plasma-systemmonitor/) which offers both a fully customizable UI as well as simple desktop and panel widgets.

![Screenshot of a Plasma Notification popup: “First Ray of Sun: The first 500 Wh of solar energy have been produced today.”](https://github.com/kbroulik/qalphacloud/raw/master/artwork/Screenshot_kdednotifier.png?raw=true "Plasma Notification about the first way of sun")
*Plasma notifying about the first ray of sun. Go start the dishwasher! (Not included in this repository yet)*

While I am aware that the number of people who operate this particular system _and_ are interested in C++/Qt development is miniscule, it’s still a useful project for me personally and is a good exercise in system integration. More importantly, though, this should serve as an inspiration for the [KDE Eco initiative](https://eco.kde.org/), for example to build an infrastructure to do system maintenance tasks such as installing updates only when the sun is shining.

## :hammer: Getting Started

This project is configured using `cmake` and is built with [Qt](https://www.qt.io/). It also uses some of KDE’s fantastic [Frameworks](https://develop.kde.org/products/frameworks/).

### API Keys

The API access configuration should be placed in the configuration file `~/.config/qalphacloud.ini`:
```
[Api]
ApiUrl=https://... # optional
AppId=alpha...
AppSecret=...
```

To get access, sign up on [Alpha Cloud Open API website](https://open.alphaess.com/), register your device using the serial number and check code written on the inverter box. You will then find the relevant AppID and Secret on the [Developer Information](https://open.alphaess.com/developmentManagement/developerInformation/) page.

### Build

Configure and run the build through `cmake`, for example:
```
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=<prefix>
cmake --build .
cmake --install .
```

### Configure Options

By default, all features and dependencies (except example code) are enabled. You can disable any feature by passing the relevant `-DBUILD_...=OFF` switch to the `cmake` command:

| Option | Default | Description
| - | - | - |
| **BUILD_QML** | **ON** | Build QML bindings
| **BUILD_KSYSTEMSTATS** | **ON** | Build KSystemStats plug-in
| **BUILD_KINFOCENTER** | **ON** | Build KInfoCenter module
| **BUILD_TESTING** | **ON** | Build unit tests
| **BUILD_COVERAGE** | **OFF** | Build with test coverage (*gcov*) enabled
| **BUILD_EXAMPLES** | **OFF** | Build examples in the [examples](examples/) directory
| **PRESENTATION_BUILD** | **OFF** | Hide sensitive information, such as serial numbers, for use in a presentation
| **API_URL** | https://openapi.alphaess.com/api/ | API URL to use, defaults to the official endpoint

### Logging Categories

The library makes use of [Categorized Logging](https://doc.qt.io/qt-5/qloggingcategory.html#configuring-categories) under the **qalphacloud** identifier.

You can enable all debug output by running an application with:
```
QT_LOGGING_RULES='qalphacloud*=true'
```

### Dependencies

This project needs Qt 5.15 and KDE Frameworks 5.80 (most notably [ECM](https://api.kde.org/frameworks/extra-cmake-modules/html/index.html)) or newer. It is possible to build just the library without the GUI components.

#### TL;DR

On Ubuntu 22.04 run:
```
# apt install build-essential cmake qtbase5-dev qtdeclarative5-dev qtquickcontrols2-5-dev extra-cmake-modules libkf5coreaddons-dev libkf5declarative-dev libkf5sysguard-dev libsensors-dev qml-module-org-kde-kirigami2 qml-module-org-kde-quickcharts
```

The following development files are required (as well as their respective indirect dependencies):

#### libqalphacloud

* Qt Core and Qt Network
* Qt QML / Declarative (*optional*, for QML bindings)
* Qt Widgets (*optional*, for example code)

#### systemstats

* KCoreAddons (from KDE Frameworks)
* KSysGuard (from KDE Plasma)
* libsensors

#### infocenter

* KCoreAddons (from KDE Frameworks
* KDeclarative (from KDE Frameworks)
* KSysGuard (from KDE Plasma)
* libsensors
* QML module for Kirigami
* QML module for KQuickCharts

## :electric_plug: API

You can find an extensive Doxygen documentation on [GitHub Pages](https://kbroulik.github.io/qalphacloud).

The project aims to provide ABI and API stability, however it cannot be guaranteed at this early stage of development.

### Classes

All classes are found in the `QAlphaCloud` C++ namespace and the `de.broulik.qalphacloud` QML import.

#### QAlphaCloud *(namespace)*

Namespace with `RequestStatus`, `SystemStatus` and `ErrorCode` enums.

#### Configuration

Represents the configuration of an API consumer, i.e. the endpoint URL, App ID, and secret registered with the API.

This can either be programmatically set or read from the [Configuration file](#api-keys) mentioned above.

#### Connector

Represents a connection to the API with a given *Configuration*. This needs to be created in order to use any of the classes below.

#### StorageSystemsModel

Endpoint: `/getEssList`

Fetches the list of registered storage systems from the given *Connector* and provides them as a `QAbstractListModel`.

For convenience, the first (and typically only) serial number is offered in the `primarySerialNumber` property.

#### LastPowerData

Endpoint: `/getLastPowerData`

Fetches live data, such as current photovoltaic energy, battery state of charge, etc from the given *Connector* and serial number.

#### OneDateEnergy

Endpoint: `/getOneDateEnergy`

Fetches cumulative data energy information, such as amount of energy produced in a day, from the given *Connector*, serial number, and date.

#### OneDayPowerModel

Endpoint: `/getOneDayPower`

Fetches historic power data, such as a trend of photovoltaic production over a day, from the given *Connector*, serial number, and date, and provides them as a `QAbstractListModel`.

### Examples

You can find examples for both C++ and QML in the [examples](examples/) directory.

## :building_construction: To Do

* The project should be prepared for a Qt 6 build, however this has not been attempted, specifically as KDE Frameworks 6 is still in development.
* Running clang-tidy, clazy, and friends on CI
* None of the EV-related readouts are implemented.
* None of the charging configuration settings (`getChargeConfigInfo`, `updateChargeConfigInfo`, `getDisChargeConfigInfo`, `updateDisChargeConfigInfo`) can be read or altered.
* Localization. All user-visible strings are marked for translation with `tr` or `qsTr` but the infrastructure for extracting and importing them has not been set up.
* Sometimes, when rapidly switching between dates, the API returns `null` for all fields without returning an error code. I consider this an API issue but we could do a tentative reload when this happens.

## :paperclips: Licensing

This project is fully [REUSE-3.0-compliant](https://reuse.software/).

### libqalphacloud

```
Copyright (C) 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 2.1 of the License, or (at your option)
any later version.

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License along
with this library; if not, write to the Free Software Foundation, Inc., 51
Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
```

### systemstats and infocenter

```
Copyright (C) 2023 Kai Uwe Broulik <ghqalpha@broulik.de>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
Street, Fifth Floor, Boston, MA 02110-1301, USA.
```

### Example code and build system configuration

```
Copyright (C) 2023 Kai Uwe Broulik <ghqalpha@broulik.de> All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

*This project is not affiliated, associated, authorized, endorsed by, or in any way officially connected with Alpha ESS Co., Ltd., or any of its subsidiaries or its affiliates. The official Alpha ESS Monitoring dashboard can be found at [https://cloud.alpha.ess.com](https://cloud.alphaess.com/).*
