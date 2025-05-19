#include "message.h"
#include <QDateTime>
#include <QStringList>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>

// Add default constructor implementation
Message::Message()
    : content(""), 
      sender(""), 
      timestamp(QDateTime::currentDateTime()), 
      isRead(false) {}

Message::Message(const QString& content, const QString& sender) 
    : content(content), 
      sender(sender), 
      timestamp(QDateTime::currentDateTime()), 
      isRead(false) {}

QString Message::toString() const {
    // Improved serialization format with better separation
    return QString("%1|%2|%3|%4")
        .arg(sender)
        .arg(content)
        .arg(timestamp.toString(Qt::ISODate))
        .arg(isRead ? "1" : "0");
}

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
    qDebug() << "Invalid message format:" << str;
    return Message("", "");
}

void Message::markAsRead() {
    isRead = true;
}

bool Message::operator==(const Message& other) const {
    return content == other.content &&
           sender == other.sender &&
           timestamp == other.timestamp &&
           isRead == other.isRead;
}
