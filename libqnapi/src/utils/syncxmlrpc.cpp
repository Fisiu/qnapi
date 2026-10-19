/*****************************************************************************
** QNapi
** Copyright (C) 2008-2017 Piotr Krzemiński <pio.krzeminski@gmail.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
*****************************************************************************/

#include "syncxmlrpc.h"

#include <QDateTime>
#include <QDomDocument>
#include <QEventLoop>
#include <QMetaType>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamWriter>
#include <limits>

namespace {

bool writeValue(QXmlStreamWriter& writer, const QVariant& value,
                QString* errorMessage) {
  writer.writeStartElement("value");

  const int typeId = value.metaType().id();
  if (!value.isValid()) {
    writer.writeEmptyElement("nil");
  } else if (typeId == QMetaType::QString) {
    writer.writeTextElement("string", value.toString());
  } else if (typeId == QMetaType::Bool) {
    writer.writeTextElement("boolean", value.toBool() ? "1" : "0");
  } else if (typeId == QMetaType::Int || typeId == QMetaType::UInt ||
             typeId == QMetaType::LongLong ||
             typeId == QMetaType::ULongLong) {
    writer.writeTextElement("int", value.toString());
  } else if (typeId == QMetaType::Double || typeId == QMetaType::Float) {
    writer.writeTextElement(
        "double", QString::number(value.toDouble(), 'g',
                                  std::numeric_limits<double>::max_digits10));
  } else if (typeId == QMetaType::QByteArray) {
    writer.writeTextElement("base64", value.toByteArray().toBase64());
  } else if (typeId == QMetaType::QDateTime) {
    writer.writeTextElement(
        "dateTime.iso8601",
        value.toDateTime().toString("yyyyMMdd'T'hh:mm:ss"));
  } else if (typeId == QMetaType::QVariantList) {
    writer.writeStartElement("array");
    writer.writeStartElement("data");
    const QVariantList values = value.toList();
    for (const QVariant& item : values) {
      if (!writeValue(writer, item, errorMessage)) return false;
    }
    writer.writeEndElement();
    writer.writeEndElement();
  } else if (typeId == QMetaType::QVariantMap) {
    writer.writeStartElement("struct");
    const QVariantMap values = value.toMap();
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
      writer.writeStartElement("member");
      writer.writeTextElement("name", it.key());
      if (!writeValue(writer, it.value(), errorMessage)) return false;
      writer.writeEndElement();
    }
    writer.writeEndElement();
  } else {
    *errorMessage =
        QString("Unsupported XML-RPC argument type: %1")
            .arg(QString::fromLatin1(value.metaType().name()));
    return false;
  }

  writer.writeEndElement();
  return true;
}

bool parseValue(const QDomElement& valueElement, QVariant* value,
                QString* errorMessage) {
  if (valueElement.tagName().compare("value", Qt::CaseInsensitive) != 0) {
    *errorMessage = "XML-RPC value element is missing";
    return false;
  }

  const QDomElement typeElement = valueElement.firstChildElement();
  if (typeElement.isNull()) {
    *value = valueElement.text();
    return true;
  }

  const QString type = typeElement.tagName().toLower();
  const QString text = typeElement.text().trimmed();
  if (type == "string") {
    *value = typeElement.text();
  } else if (type == "i8") {
    bool ok = false;
    const qlonglong integer = text.toLongLong(&ok);
    if (!ok) {
      *errorMessage = "Invalid XML-RPC integer value";
      return false;
    }
    *value = integer;
  } else if (type == "int" || type == "i4") {
    bool ok = false;
    const int integer = text.toInt(&ok);
    if (!ok) {
      *errorMessage = "Invalid XML-RPC integer value";
      return false;
    }
    *value = integer;
  } else if (type == "double") {
    bool ok = false;
    const double number = text.toDouble(&ok);
    if (!ok) {
      *errorMessage = "Invalid XML-RPC double value";
      return false;
    }
    *value = number;
  } else if (type == "boolean") {
    if (text == "1" || text.compare("true", Qt::CaseInsensitive) == 0) {
      *value = true;
    } else if (text == "0" || text.compare("false", Qt::CaseInsensitive) == 0) {
      *value = false;
    } else {
      *errorMessage = "Invalid XML-RPC boolean value";
      return false;
    }
  } else if (type == "base64") {
    *value = QByteArray::fromBase64(text.toLatin1());
  } else if (type == "datetime.iso8601" || type == "datetime") {
    const QDateTime dateTime =
        QDateTime::fromString(text, "yyyyMMdd'T'hh:mm:ss");
    if (!dateTime.isValid()) {
      *errorMessage = "Invalid XML-RPC date-time value";
      return false;
    }
    *value = dateTime;
  } else if (type == "nil") {
    *value = QVariant();
  } else if (type == "array") {
    const QDomElement dataElement = typeElement.firstChildElement("data");
    if (dataElement.isNull()) {
      *errorMessage = "XML-RPC array has no data element";
      return false;
    }

    QVariantList values;
    for (QDomElement item = dataElement.firstChildElement(); !item.isNull();
         item = item.nextSiblingElement()) {
      QVariant itemValue;
      if (!parseValue(item, &itemValue, errorMessage)) return false;
      values.push_back(itemValue);
    }
    *value = values;
  } else if (type == "struct") {
    QVariantMap values;
    for (QDomElement member = typeElement.firstChildElement("member");
         !member.isNull(); member = member.nextSiblingElement("member")) {
      const QDomElement nameElement = member.firstChildElement("name");
      const QDomElement memberValue = member.firstChildElement("value");
      if (nameElement.isNull() || memberValue.isNull()) {
        *errorMessage = "Malformed XML-RPC struct member";
        return false;
      }

      QVariant memberResult;
      if (!parseValue(memberValue, &memberResult, errorMessage)) return false;
      values.insert(nameElement.text(), memberResult);
    }
    *value = values;
  } else {
    *errorMessage = QString("Unsupported XML-RPC value type: %1").arg(type);
    return false;
  }

  return true;
}

bool createRequest(const QString& method, const QVariantList& args,
                   QByteArray* payload, QString* errorMessage) {
  QXmlStreamWriter writer(payload);
  writer.writeStartDocument();
  writer.writeStartElement("methodCall");
  writer.writeTextElement("methodName", method);
  writer.writeStartElement("params");
  for (const QVariant& arg : args) {
    writer.writeStartElement("param");
    if (!writeValue(writer, arg, errorMessage)) return false;
    writer.writeEndElement();
  }
  writer.writeEndElement();
  writer.writeEndElement();
  writer.writeEndDocument();
  return true;
}

}  // namespace

SyncXmlRpc::SyncXmlRpc(const QUrl& endPoint) : endPoint(endPoint) {}

bool SyncXmlRpc::request(const QString &method, const QVariantList &args) {
  error = 0;
  message.clear();
  resp.clear();

  QByteArray payload;
  if (!createRequest(method, args, &payload, &message)) {
    error = -32600;
    return false;
  }

  QNetworkAccessManager manager;
  QNetworkRequest request(endPoint);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
  request.setRawHeader("User-Agent", "QNapi");

  QNetworkReply* reply = manager.post(request, payload);
  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (reply->error() != QNetworkReply::NoError) {
    error = static_cast<int>(reply->error());
    message = reply->errorString();
    reply->deleteLater();
    return false;
  }

  const QByteArray response = reply->readAll();
  reply->deleteLater();

  QDomDocument document;
  const QDomDocument::ParseResult parseResult = document.setContent(response);
  if (!parseResult) {
    error = -32700;
    message = QString("XML-RPC response parse error at line %1, column %2: %3")
                  .arg(parseResult.errorLine)
                  .arg(parseResult.errorColumn)
                  .arg(parseResult.errorMessage);
    return false;
  }

  const QDomElement root = document.documentElement();
  if (root.tagName().compare("methodResponse", Qt::CaseInsensitive) != 0) {
    error = -32600;
    message = "Invalid XML-RPC response";
    return false;
  }

  const QDomElement faultElement = root.firstChildElement("fault");
  if (!faultElement.isNull()) {
    QVariant fault;
    if (!parseValue(faultElement.firstChildElement("value"), &fault,
                    &message)) {
      error = -32600;
      return false;
    }

    const QVariantMap faultMap = fault.toMap();
    if (!faultMap.contains("faultCode") || !faultMap.contains("faultString")) {
      error = -32600;
      message = "Malformed XML-RPC fault response";
      return false;
    }
    error = faultMap.value("faultCode").toInt();
    message = faultMap.value("faultString").toString();
    return false;
  }

  const QDomElement param =
      root.firstChildElement("params").firstChildElement("param");
  if (param.isNull()) return true;

  if (!parseValue(param.firstChildElement("value"), &resp, &message)) {
    error = -32600;
    return false;
  }
  return true;
}

QVariant SyncXmlRpc::getResponse() const { return resp; }

QVariant SyncXmlRpc::getError() const {
  return QString::number(error) + " " + message;
}
