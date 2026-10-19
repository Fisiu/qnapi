#include "utils/syncxmlrpc.h"

#include <QCoreApplication>
#include <QDomDocument>
#include <QTcpServer>
#include <QTcpSocket>

#include <iostream>

class LocalXmlRpcServer : public QTcpServer {
 public:
  explicit LocalXmlRpcServer(QByteArray response) : response(response) {
    QObject::connect(this, &QTcpServer::newConnection, this, [this] {
      QTcpSocket* socket = nextPendingConnection();
      QObject::connect(socket, &QTcpSocket::readyRead, this,
                       [this, socket] { readRequest(socket); });
    });
  }

  QByteArray requestBody;
  QString error;

 private:
  QByteArray response;
  QByteArray requestBuffer;

  void readRequest(QTcpSocket* socket) {
    requestBuffer += socket->readAll();
    const qsizetype headerEnd = requestBuffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) return;

    qsizetype contentLength = -1;
    const QList<QByteArray> headers =
        requestBuffer.left(headerEnd).split('\n');
    for (QByteArray header : headers) {
      header = header.trimmed();
      if (header.toLower().startsWith("content-length:")) {
        bool ok = false;
        contentLength =
            header.mid(header.indexOf(':') + 1).trimmed().toLongLong(&ok);
        if (!ok) contentLength = -1;
        break;
      }
    }

    if (contentLength < 0) {
      error = "Request has no valid Content-Length header";
      socket->disconnectFromHost();
      return;
    }

    const qsizetype bodyStart = headerEnd + 4;
    if (requestBuffer.size() < bodyStart + contentLength) return;
    requestBody = requestBuffer.mid(bodyStart, contentLength);

    QByteArray httpResponse =
        "HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nConnection: close\r\n"
        "Content-Length: " +
        QByteArray::number(response.size()) + "\r\n\r\n" + response;
    socket->write(httpResponse);
    socket->disconnectFromHost();
  }
};

bool runRequest(const QByteArray& response, bool expectSuccess,
                QVariant* result, QVariant* error) {
  LocalXmlRpcServer server(response);
  if (!server.listen(QHostAddress::LocalHost)) {
    std::cerr << "Could not start local XML-RPC test server\n";
    return false;
  }

  SyncXmlRpc rpc(QUrl(QString("http://127.0.0.1:%1/")
                          .arg(server.serverPort())));
  QVariantMap requestMap;
  requestMap.insert("title", QStringLiteral("A&B <C>"));
  requestMap.insert("tags", QVariantList{QStringLiteral("one"),
                                         QStringLiteral("two")});
  QVariantList args{QString(), requestMap};

  const bool success = rpc.request("SearchSubtitles", args);
  if (success != expectSuccess) {
    std::cerr << "Unexpected XML-RPC request result\n";
    return false;
  }
  if (!server.error.isEmpty() || server.requestBody.isEmpty()) {
    std::cerr << server.error.toStdString() << '\n';
    return false;
  }

  QDomDocument requestDocument;
  const auto parseResult = requestDocument.setContent(server.requestBody);
  if (!parseResult) {
    std::cerr << "Client sent invalid XML-RPC XML\n";
    return false;
  }

  const QDomElement call = requestDocument.documentElement();
  if (call.tagName() != "methodCall" ||
      call.firstChildElement("methodName").text() != "SearchSubtitles") {
    std::cerr << "Client sent an invalid XML-RPC method call\n";
    return false;
  }

  const QDomElement params = call.firstChildElement("params");
  const QDomElement firstValue =
      params.firstChildElement("param").firstChildElement("value");
  if (firstValue.firstChildElement().tagName() != "string" ||
      !firstValue.firstChildElement().text().isEmpty()) {
    std::cerr << "Empty string argument was not serialized as a string\n";
    return false;
  }

  const QDomElement secondValue =
      params.firstChildElement("param").nextSiblingElement("param")
          .firstChildElement("value");
  QDomElement titleValue;
  for (QDomElement member =
           secondValue.firstChildElement("struct").firstChildElement("member");
       !member.isNull(); member = member.nextSiblingElement("member")) {
    if (member.firstChildElement("name").text() == "title")
      titleValue = member.firstChildElement("value");
  }
  if (titleValue.text() != "A&B <C>") {
    std::cerr << "XML-RPC string argument was not escaped and restored\n";
    return false;
  }

  *result = rpc.getResponse();
  *error = rpc.getError();
  return true;
}

int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);
  const QByteArray successResponse =
      "<?xml version=\"1.0\"?><methodResponse><params><param><value><struct>"
      "<member><name>token</name><value><string>t&amp;k</string></value></member>"
      "<member><name>data</name><value><array><data><value><struct>"
      "<member><name>id</name><value><int>42</int></value></member>"
      "<member><name>active</name><value><boolean>1</boolean></value></member>"
      "</struct></value></data></array></value></member>"
      "</struct></value></param></params></methodResponse>";

  QVariant response;
  QVariant error;
  if (!runRequest(successResponse, true, &response, &error)) return 1;

  const QVariantMap responseMap = response.toMap();
  const QVariantList data = responseMap.value("data").toList();
  if (responseMap.value("token").toString() != "t&k" || data.size() != 1 ||
      data.first().toMap().value("id").toInt() != 42 ||
      !data.first().toMap().value("active").toBool()) {
    std::cerr << "XML-RPC response values were not decoded correctly\n";
    return 1;
  }

  const QByteArray faultResponse =
      "<?xml version=\"1.0\"?><methodResponse><fault><value><struct>"
      "<member><name>faultCode</name><value><int>42</int></value></member>"
      "<member><name>faultString</name><value><string>access denied</string>"
      "</value></member></struct></value></fault></methodResponse>";
  if (!runRequest(faultResponse, false, &response, &error) ||
      error.toString() != "42 access denied") {
    std::cerr << "XML-RPC fault response was not reported correctly\n";
    return 1;
  }

  return 0;
}
