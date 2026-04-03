#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QVector>

#include <functional>

namespace CustomBackend {

struct UserDTO {
	QString id;
	QString login;
	QString displayName;
	QString role;
	bool banned = false;
	bool online = false;
	QDateTime lastSeenAt;
};

struct AttachmentDTO {
	QString id;
	QString messageId;
	QString chatId;
	QString fileName;
	QString mimeType;
	QString kind;
	QString url;
	qint64 size = 0;
	QDateTime uploadedAt;
};

struct ReactionDTO {
	QString messageId;
	QString userId;
	QString emoji;
	QDateTime createdAt;
};

struct MessageDTO {
	QString id;
	QString chatId;
	QString senderId;
	UserDTO sender;
	QString text;
	QString replyToMessageId;
	QVector<AttachmentDTO> attachments;
	QVector<ReactionDTO> reactions;
	QDateTime createdAt;
	QDateTime updatedAt;
	bool deleted = false;
};

struct ChatMemberDTO {
	QString userId;
	QString role;
	QString lastReadMessageId;
	QDateTime lastReadAt;
};

struct ChatDTO {
	QString id;
	QString kind;
	QString title;
	QString description;
	QString username;
	QVector<ChatMemberDTO> members;
	QString lastMessagePreview;
	int unreadCount = 0;
	QDateTime createdAt;
	QDateTime updatedAt;
};

struct ChannelDTO {
	QString id;
	QString title;
	QString description;
	QString username;
	QString ownerId;
	QDateTime createdAt;
	QDateTime updatedAt;
};

struct PresenceDTO {
	QString userId;
	bool online = false;
	QDateTime lastSeenAt;
	QString typingChatId;
};

struct AuthSessionDTO {
	QString accessToken;
	QString refreshToken;
	UserDTO user;
};

struct LoginRequest {
	QString login;
	QString password;
	QString displayName;
	QString inviteCode;
};

struct SendMessageRequest {
	QString text;
	QString replyToMessageId;
	QVector<QString> attachmentIds;
};

struct EventEnvelope {
	QString type;
	qint64 seq = 0;
	QDateTime at;
	QJsonValue payload;
};

using ErrorHandler = std::function<void(QString)>;
using EventHandler = std::function<void(EventEnvelope)>;

UserDTO ParseUser(const QJsonObject &data);
AttachmentDTO ParseAttachment(const QJsonObject &data);
ReactionDTO ParseReaction(const QJsonObject &data);
MessageDTO ParseMessage(const QJsonObject &data);
ChatDTO ParseChat(const QJsonObject &data);
ChannelDTO ParseChannel(const QJsonObject &data);
PresenceDTO ParsePresence(const QJsonObject &data);
AuthSessionDTO ParseAuthSession(const QJsonObject &data);
QJsonObject ToJson(const LoginRequest &request);
QJsonObject ToJson(const SendMessageRequest &request);

}
