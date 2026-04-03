#pragma once

#include "custom_backend/backend_types.h"

#include <QNetworkAccessManager>
#include <QUrl>
#include <QUrlQuery>
#include <QVector>

class QNetworkReply;

namespace CustomBackend {

class HttpClient final {
public:
	explicit HttpClient(QUrl baseUrl);

	void setAccessToken(QString token);
	void setRefreshToken(QString token);
	[[nodiscard]] const QString &accessToken() const;
	[[nodiscard]] const QString &refreshToken() const;

	void login(const LoginRequest &request, std::function<void(AuthSessionDTO)> done, ErrorHandler fail);
	void refresh(std::function<void(AuthSessionDTO)> done, ErrorHandler fail);
	void fetchChats(std::function<void(QVector<ChatDTO>)> done, ErrorHandler fail);
	void fetchMessages(const QString &chatId, const QString &before, int limit, std::function<void(QVector<MessageDTO>)> done, ErrorHandler fail);
	void sendMessage(const QString &chatId, const SendMessageRequest &request, std::function<void(MessageDTO)> done, ErrorHandler fail);
	void fetchChannels(std::function<void(QVector<ChannelDTO>)> done, ErrorHandler fail);

private:
	using JsonHandler = std::function<void(QJsonValue)>;

	void get(const QString &path, const QUrlQuery &query, JsonHandler done, ErrorHandler fail);
	void post(const QString &path, const QJsonObject &body, JsonHandler done, ErrorHandler fail);
	void request(QNetworkAccessManager::Operation operation, const QString &path, const QUrlQuery &query, const QByteArray &body, const QByteArray &contentType, JsonHandler done, ErrorHandler fail);

	QNetworkAccessManager _manager;
	QUrl _baseUrl;
	QString _accessToken;
	QString _refreshToken;
};

}