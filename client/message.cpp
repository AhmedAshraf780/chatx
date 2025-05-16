#include "message.h"
#include <QDateTime>
#include <QStringList>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>

/*
 * Contributor: Qobesy
 * Function: Message default constructor
 * Description: Creates an empty message with the current timestamp and unread status.
 * Used when creating messages without specifying content or sender.
 */
Message::Message()
    : content(""), 
      sender(""), 
      timestamp(QDateTime::currentDateTime()), 
      isRead(false) {}

/*
 * Contributor: Qobesy
 * Function: Message parameterized constructor
 * Description: Creates a message with the specified content and sender,
 * automatically setting the timestamp to the current time and marking as unread.
 */
Message::Message(const QString& content, const QString& sender) 
    : content(content), 
      sender(sender), 
      timestamp(QDateTime::currentDateTime()), 
      isRead(false) {}

/*
 * Contributor: Qobesy
 * Function: toString
 * Description: Serializes a message object to a string format for storage.
 * Uses a pipe delimiter to separate message components in the format:
 * sender|content|timestamp|read_status.
 */
QString Message::toString() const {
    // Improved serialization format with better separation
    return QString("%1|%2|%3|%4")
        .arg(sender)
        .arg(content)
        .arg(timestamp.toString(Qt::ISODate))
        .arg(isRead ? "1" : "0");
}

/*
 * Contributor: Qobesy
 * Function: fromString
 * Description: Deserializes a string back into a Message object.
 * Parses a pipe-delimited string containing sender, content, timestamp, and read status.
 * Returns an empty message if the string format is invalid.
 */
Message Message::fromString(const QString& str) {
    QStringList parts = str.split("|");
    if (parts.size() == 4) {
        Message msg(parts[1], parts[0]);
        msg.timestamp = QDateTime::fromString(parts[2], Qt::ISODate);
        if (parts[3] == "1") {
            msg.markAsRead();
        }
        return msg;
    }
    return Message("", "");
}

/*
 * Contributor: Qobesy
 * Function: markAsRead
 * Description: Marks the message as read.
 * Used when a user views a previously unread message.
 */
void Message::markAsRead() {
    isRead = true;
}

/*
 * Contributor: Qobesy
 * Function: operator==
 * Description: Compares two Message objects for equality.
 * Two messages are considered equal if they have the same content, 
 * sender, timestamp, and read status.
 */
bool Message::operator==(const Message& other) const {
    return content == other.content &&
           sender == other.sender &&
           timestamp == other.timestamp &&
           isRead == other.isRead;
}
