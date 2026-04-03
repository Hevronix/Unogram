#pragma once

#include "custom_backend/backend_types.h"

#include <QUrl>
#include <QtWebSockets/QWebSocket>

namespace CustomBackend {

class RealtimeClient final {
public:
	explicit RealtimeClient(QUrl baseUrl);

	void setAccessToken(QString token);
	void setHandlers(EventHandler eventHandler, ErrorHandler errorHandler);
	void start(qint64 sinceSeq = 0);
	void stop();

private:
	void handleTextMessage(const QString &message);

	QWebSocket _socket;
	QUrl _baseUrl;
	QString _accessToken;
	EventHandler _eventHandler;
	ErrorHandler _errorHandler;
};

}