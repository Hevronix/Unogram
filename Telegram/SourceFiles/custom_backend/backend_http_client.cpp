#include "custom_backend/backend_http_client.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {

QDateTime ParseDateTime(const QJsonValue &value) {
	return QDateTime::fromString(value.toString(), Qt::ISODate);
}

QString ErrorText(const QByteArray &body, const QString &fallback) {
	const auto document = QJsonDocument::fromJson(body);
	if (document.isObject()) {
		const auto message = document.object().value("message").toString();
		if (!message.isEmpty()) {
			return message;
		}
	}
	return fallback;
}

} 

namespace CustomBackend {

UserDTO ParseUser(const QJsonObject &data) {
	auto result = UserDTO();
	result.id = data.value("id").toString();
	result.login = data.value("login").toString();
	result.displayName = data.value("display_name").toString();
	result.role = data.value("role").toString();
	result.banned = data.value("banned").toBool();
	result.online = data.value("online").toBool();
	result.lastSeenAt = ParseDateTime(data.value("last_seen_at"));
	return result;
}

AttachmentDTO ParseAttachment(const QJsonObject &data) {
	auto result = AttachmentDTO();
	result.id = data.value("id").toString();
	result.messageId = data.value("message_id").toString();
	result.chatId = data.value("chat_id").toString();
	result.fileName = data.value("file_name").toString();
	result.mimeType = data.value("mime_type").toString();
	result.kind = data.value("kind").toString();
	result.url = data.value("url").toString();
	result.size = data.value("size").toInteger();
	result.uploadedAt = ParseDateTime(data.value("uploaded_at"));
	return result;
}

ReactionDTO ParseReaction(const QJsonObject &data) {
	auto result = ReactionDTO();
	result.messageId = data.value("message_id").toString();
	result.userId = data.value("user_id").toString();
	result.emoji = data.value("emoji").toString();
	result.createdAt = ParseDateTime(data.value("created_at"));
	return result;
}

MessageDTO ParseMessage(const QJsonObject &data) {
	auto result = MessageDTO();
	result.id = data.value("id").toString();
	result.chatId = data.value("chat_id").toString();
	result.senderId = data.value("sender_id").toString();
	result.sender = ParseUser(data.value("sender").toObject());
	result.text = data.value("text").toString();
	result.replyToMessageId = data.value("reply_to_message_id").toString();
	result.createdAt = ParseDateTime(data.value("created_at"));
	result.updatedAt = ParseDateTime(data.value("updated_at"));
	result.deleted = data.value("deleted").toBool();
	for (const auto &entry : data.value("attachments").toArray()) {
		result.attachments.push_back(ParseAttachment(entry.toObject()));
	}
	for (const auto &entry : data.value("reactions").toArray()) {
		result.reactions.push_back(ParseReaction(entry.toObject()));
	}
	return result;
}

ChatDTO ParseChat(const QJsonObject &data) {
	auto result = ChatDTO();
	result.id = data.value("id").toString();
	result.kind = data.value("kind").toString();
	result.title = data.value("title").toString();
	result.description = data.value("description").toString();
	result.username = data.value("username").toString();
	result.lastMessagePreview = data.value("last_message_preview").toString();
	result.unreadCount = data.value("unread_count").toInt();
	result.createdAt = ParseDateTime(data.value("created_at"));
	result.updatedAt = ParseDateTime(data.value("updated_at"));
	for (const auto &entry : data.value("members").toArray()) {
		auto member = ChatMemberDTO();
		const auto object = entry.toObject();
		member.userId = object.value("user_id").toString();
		member.role = object.value("role").toString();
		member.lastReadMessageId = object.value("last_read_message_id").toString();
		member.lastReadAt = ParseDateTime(object.value("last_read_at"));
		result.members.push_back(member);
	}
	return result;
}

ChannelDTO ParseChannel(const QJsonObject &data) {
	auto result = ChannelDTO();
	result.id = data.value("id").toString();
	result.title = data.value("title").toString();
	result.description = data.value("description").toString();
	result.username = data.value("username").toString();
	result.ownerId = data.value("owner_id").toString();
	result.createdAt = ParseDateTime(data.value("created_at"));
	result.updatedAt = ParseDateTime(data.value("updated_at"));
	return result;
}

PresenceDTO ParsePresence(const QJsonObject &data) {
	auto result = PresenceDTO();
	result.userId = data.value("user_id").toString();
	result.online = data.value("online").toBool();
	result.lastSeenAt = ParseDateTime(data.value("last_seen_at"));
	result.typingChatId = data.value("typing_chat_id").toString();
	return result;
}

AuthSessionDTO ParseAuthSession(const QJsonObject &data) {
	auto result = AuthSessionDTO();
	result.accessToken = data.value("access_token").toString();
	result.refreshToken = data.value("refresh_token").toString();
	result.user = ParseUser(data.value("user").toObject());
	return result;
}

QJsonObject ToJson(const LoginRequest &request) {
	auto result = QJsonObject();
	result.insert("login", request.login);
	result.insert("password", request.password);
	if (!request.displayName.isEmpty()) {
		result.insert("display_name", request.displayName);
	}
	if (!request.inviteCode.isEmpty()) {
		result.insert("invite_code", request.inviteCode);
	}
	return result;
}

QJsonObject ToJson(const SendMessageRequest &request) {
	auto result = QJsonObject();
	result.insert("text", request.text);
	if (!request.replyToMessageId.isEmpty()) {
		result.insert("reply_to_message_id", request.replyToMessageId);
	}
	auto attachments = QJsonArray();
	for (const auto &id : request.attachmentIds) {
		attachments.push_back(id);
	}
	result.insert("attachment_ids", attachments);
	return result;
}

HttpClient::HttpClient(QUrl baseUrl)
: _baseUrl(std::move(baseUrl)) {
}

void HttpClient::setAccessToken(QString token) {
	_accessToken = std::move(token);
}

void HttpClient::setRefreshToken(QString token) {
	_refreshToken = std::move(token);
}

const QString &HttpClient::accessToken() const {
	return _accessToken;
}

const QString &HttpClient::refreshToken() const {
	return _refreshToken;
}

void HttpClient::login(const LoginRequest &request, std::function<void(AuthSessionDTO)> done, ErrorHandler fail) {
	post(QStringLiteral("/auth/login"), ToJson(request), [=](QJsonValue value) {
		const auto session = ParseAuthSession(value.toObject());
		if (!session.accessToken.isEmpty()) {
			setAccessToken(session.accessToken);
			setRefreshToken(session.refreshToken);
		}
		done(session);
	}, fail);
}

void HttpClient::refresh(std::function<void(AuthSessionDTO)> done, ErrorHandler fail) {
	auto body = QJsonObject();
	body.insert("refresh_token", _refreshToken);
	post(QStringLiteral("/auth/refresh"), body, [=](QJsonValue value) {
		const auto session = ParseAuthSession(value.toObject());
		if (!session.accessToken.isEmpty()) {
			setAccessToken(session.accessToken);
			setRefreshToken(session.refreshToken);
		}
		done(session);
	}, fail);
}

void HttpClient::fetchChats(std::function<void(QVector<ChatDTO>)> done, ErrorHandler fail) {
	get(QStringLiteral("/chats"), QUrlQuery(), [=](QJsonValue value) {
		auto result = QVector<ChatDTO>();
		for (const auto &entry : value.toArray()) {
			result.push_back(ParseChat(entry.toObject()));
		}
		done(result);
	}, fail);
}

void HttpClient::fetchMessages(const QString &chatId, const QString &before, int limit, std::function<void(QVector<MessageDTO>)> done, ErrorHandler fail) {
	auto query = QUrlQuery();
	if (!before.isEmpty()) {
		query.addQueryItem(QStringLiteral("before"), before);
	}
	if (limit > 0) {
		query.addQueryItem(QStringLiteral("limit"), QString::number(limit));
	}
	get(QStringLiteral("/chats/") + chatId + QStringLiteral("/messages"), query, [=](QJsonValue value) {
		auto result = QVector<MessageDTO>();
		for (const auto &entry : value.toArray()) {
			result.push_back(ParseMessage(entry.toObject()));
		}
		done(result);
	}, fail);
}

void HttpClient::sendMessage(const QString &chatId, const SendMessageRequest &request, std::function<void(MessageDTO)> done, ErrorHandler fail) {
	post(QStringLiteral("/chats/") + chatId + QStringLiteral("/messages"), ToJson(request), [=](QJsonValue value) {
		done(ParseMessage(value.toObject()));
	}, fail);
}

void HttpClient::fetchChannels(std::function<void(QVector<ChannelDTO>)> done, ErrorHandler fail) {
	get(QStringLiteral("/channels"), QUrlQuery(), [=](QJsonValue value) {
		auto result = QVector<ChannelDTO>();
		for (const auto &entry : value.toArray()) {
			result.push_back(ParseChannel(entry.toObject()));
		}
		done(result);
	}, fail);
}

void HttpClient::get(const QString &path, const QUrlQuery &query, JsonHandler done, ErrorHandler fail) {
	request(QNetworkAccessManager::GetOperation, path, query, QByteArray(), QByteArray(), std::move(done), std::move(fail));
}

void HttpClient::post(const QString &path, const QJsonObject &body, JsonHandler done, ErrorHandler fail) {
	request(QNetworkAccessManager::PostOperation, path, QUrlQuery(), QJsonDocument(body).toJson(QJsonDocument::Compact), "application/json", std::move(done), std::move(fail));
}

void HttpClient::request(QNetworkAccessManager::Operation operation, const QString &path, const QUrlQuery &query, const QByteArray &body, const QByteArray &contentType, JsonHandler done, ErrorHandler fail) {
	auto url = _baseUrl;
	url.setPath(path);
	url.setQuery(query);
	auto request = QNetworkRequest(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
	if (!_accessToken.isEmpty()) {
		request.setRawHeader("Authorization", QByteArray("Bearer ") + _accessToken.toUtf8());
	}
	auto *reply = _manager.sendCustomRequest(request, operation == QNetworkAccessManager::GetOperation ? "GET" : "POST", body);
	QObject::connect(reply, &QNetworkReply::finished, [reply, done = std::move(done), fail = std::move(fail)]() mutable {
		const auto payload = reply->readAll();
		const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		const auto networkError = reply->error();
		reply->deleteLater();
		if (networkError != QNetworkReply::NoError || status >= 400) {
			fail(ErrorText(payload, reply->errorString()));
			return;
		}
		const auto document = QJsonDocument::fromJson(payload);
		done(document.isArray() ? QJsonValue(document.array()) : QJsonValue(document.object()));
	});
}

}
