/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "qalphacloud_export.h"

#include <QLocale>
#include <QNetworkReply>
#include <QObject>

#include <optional>
#include <ratio>
#include <type_traits>

/**
 * @brief Utility namespace
 */
namespace QAlphaCloud
{
Q_NAMESPACE_EXPORT(QALPHACLOUD_EXPORT)

/**
 * @brief Request status
 */
enum class RequestStatus {
    NoRequest = 0, ///< No request has been issued.
    Loading, ///< The request is being loaded from the server.
    Finished, ///< The request has finished successfully.
    Error, ///< The request failed with an error.
};
Q_ENUM_NS(RequestStatus)

/**
 * @brief Error codes
 *
 * This can either be
 * * NoError
 * * A value from QNetworkReply::Error (not documented here).
 * * Errors specific to this library (in the 1xxx range).
 * * Errors from the API (in the 6xxx range).
 */
enum class ErrorCode {
    UnknownError = -1,
    NoError = 0, ///< API returns 200

    // ... QNetworkReply::Error ...

    // Our own errors.
    JsonParseError = 1001, ///< Failed to parse JSON received.
    UnexpectedJsonDataError = 1002, ///< Valid JSON received but it was not an Object (perhaps null, or an Array).
    EmptyJsonObjectError = 1002, ///< Valid JSON object was received but it was empty.

    // API errors
    ParameterError = 6001, ///< "Parameter error"
    SnNotBoundToUser = 6002, ///< "The SN is not bound to the user"
    // ///< "You have bound this SN"
    CheckCodeError = 6004, ///< "CheckCode error"
    AppIdNotBoundToSn = 6005, ///< "This appId is not bound to the SN"
    TimestampError = 6006, ///< "Timestamp error"
    SignVerificationError = 6007, ///< "Sign verification error"
    SetFailed = 6008, ///< "Set failed"
    WhitelistVerificationFailed = 6009, ///< "Whitelist verification failed"
    SignEmpty = 6010, ///< "Sign is empty"
    TimestampEmpty = 6011, ///< "timestamp is empty"
    AppIdEmpty = 6012, ///< "AppId is empty"

    InvalidDate = 6026, ///< Date is invalid, undocumented. Also returned when requesting historic data for a future date.

    VerificationCode = 6046, ///< "Verification code error"
};
Q_ENUM_NS(ErrorCode)

/**
 * @brief Human-readable error text
 * @param code The error code
 * @param details Details about the error, for example a QString with the JSON parser error message,
 * or a QJsonValue whose type is printed when a different one was expected, etc.
 * @return A human-readable error description
 */
QALPHACLOUD_EXPORT QString errorText(ErrorCode code, const QVariant &details = QVariant());

/**
 * @brief System status
 *
 * Corresponds to the @c emsStatus field on the @c /getEssList endpoint.
 *
 * @note No values are officially documented, this enumeration is incomplete.
 */
enum class SystemStatus {
    UnknownStatus = -1, ///< Unknown status
    Normal, ///< The system is performing normally
    // TODO Find out the others.
};
Q_ENUM_NS(SystemStatus)

} // namespace QAlphaCloud
