#include "custom_backend/backend_realtime_client.h"

#include <QAbstractSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace CustomBackend {

RealtimeClient::RealtimeClient(QUrl baseUrl)
: _baseUrl(std::move(baseUrl)) {
	QObject::connect(&_socket, &QWebSocket::textMessageReceived, [=](const QString &message) {
		handleTextMessage(message);
	});
	QObject::connect(&_socket, &QWebSocket::errorOccurred, [=](QAbstractSocket::SocketError) {
		if (_errorHandler) {
			_errorHandler(_socket.errorString());
		}
	});
}

void RealtimeClient::setAccessToken(QString token) {
	_accessToken = std::move(token);
}

void RealtimeClient::setHandlers(EventHandler eventHandler, ErrorHandler errorHandler) {
	_eventHandler = std::move(eventHandler);
	_errorHandler = std::move(errorHandler);
}

void RealtimeClient::start(qint64 sinceSeq) {
	auto url = _baseUrl;
	url.setScheme(url.scheme() == QStringLiteral("https") ? QStringLiteral("wss") : QStringLiteral("ws"));
	url.setPath(QStringLiteral("/realtime/ws"));
	auto query = QUrlQuery();
	if (!_accessToken.isEmpty()) {
		query.addQueryItem(QStringLiteral("access_token"), _accessToken);
	}
	if (sinceSeq > 0) {
		query.addQueryItem(QStringLiteral("since"), QString::number(sinceSeq));
	}
	url.setQuery(query);
	_socket.open(url);
}

void RealtimeClient::stop() {
	_socket.close();
}

void RealtimeClient::handleTextMessage(const QString &message) {
	const auto document = QJsonDocument::fromJson(message.toUtf8());
	if (!document.isObject()) {
		if (_errorHandler) {
			_errorHandler(QStringLiteral("Invalid realtime payload"));
		}
		return;
	}
	const auto object = document.object();
	auto event = EventEnvelope();
	event.type = object.value("type").toString();
	event.seq = static_cast<qint64>(object.value("seq").toDouble());
	event.at = QDateTime::fromString(object.value("at").toString(), Qt::ISODate);
	event.payload = object.value("payload");
	if (_eventHandler) {
		_eventHandler(std::move(event));
	}
}

}
