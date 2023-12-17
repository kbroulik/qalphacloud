/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "qalphacloud.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QMetaEnum>
#include <QNetworkReply>

namespace QAlphaCloud
{

QString errorText(ErrorCode code, const QVariant &details)
{
    const QString detailsString = details.toString();

    switch (code) {
    case ErrorCode::UnknownError:
        return QCoreApplication::translate("errorText", "An unknown error occurred.");
    case ErrorCode::NoError:
        return QCoreApplication::translate("errorText", "The operation completed successfully.");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
    case static_cast<ErrorCode>(QNetworkReply::TimeoutError):
        return QCoreApplication::translate("errorText", "Operation timed out.");
    case static_cast<ErrorCode>(QNetworkReply::OperationCanceledError):
        return QCoreApplication::translate("errorText", "Operation was canceled.");
#pragma GCC diagnostic pop

    case ErrorCode::JsonParseError:
        if (!detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Failed to parse JSON: %1").arg(detailsString);
        } else {
            return QCoreApplication::translate("errorText", "Failed to parse JSON.");
        }
    case ErrorCode::UnexpectedJsonDataError: {
        if (details.canConvert<QJsonDocument>()) {
            const auto jsonDocument = details.toJsonDocument();
            if (jsonDocument.isArray()) {
                return QCoreApplication::translate("errorText", "Unexpected JSON Array received.");
            }
        }
        return QCoreApplication::translate("errorText", "Unexpected JSON content received.");
    }

    case ErrorCode::ParameterError:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Invalid parameter provided.");
        }
        break;
    case ErrorCode::SnNotBoundToUser:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "The provided serial number is not associated with this user.");
        }
        break;
    case ErrorCode::CheckCodeError:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Check code error.");
        }
        break;
    case ErrorCode::AppIdNotBoundToSn:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "The provided application ID is not associated with this serial number.");
        }
        break;
    case ErrorCode::TimestampError:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "The provided time stamp is either invalid, or too far in the past.");
        }
        break;
    case ErrorCode::SignVerificationError:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "API secret verification error.");
        }
        break;
    case ErrorCode::SetFailed:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Failed to set requested configuration.");
        }
        break;
    case ErrorCode::SignEmpty:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "API secret was not provided.");
        }
        break;
    case ErrorCode::WhitelistVerificationFailed:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Whitelist verification failed.");
        }
        break;
    case ErrorCode::TimestampEmpty:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Request time stamp was not provided.");
        }
        break;
    case ErrorCode::AppIdEmpty:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Application ID was not provided.");
        }
        break;
    case ErrorCode::DataDoesNotExist:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Data does not exist or has been deleted.");
        }
        break;
    case ErrorCode::InvalidDate:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Invalid date provided.");
        }
        break;
    case ErrorCode::OperationFailed:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Operation failed.");
        }
        break;
    case ErrorCode::SystemSnDoesNotExist:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "System serial number does not exist.");
        }
        break;
    case ErrorCode::SystemOffline:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "System is offline.");
        }
        break;

    case ErrorCode::VerificationCode:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Verification code error.");
        }
        break;

    case ErrorCode::TooManyRequests:
        if (detailsString.isEmpty()) {
            return QCoreApplication::translate("errorText", "Too many requests, try again later.");
        }
        break;
    }

    if (!detailsString.isEmpty()) {
        return detailsString;
    }

    QMetaEnum metaEnum = QMetaEnum::fromType<ErrorCode>();
    return QString::fromUtf8(metaEnum.valueToKey(static_cast<int>(code)));
}

} // namespace QAlphaCloud
