#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QDateTime>

class Message {
private:
    QString content;
    QString sender;
    QDateTime timestamp;
    bool isRead;

public:
    // Add default constructor
    Message();
    Message(const QString &content, const QString &sender);
    
    // Getters
    QString getContent() const { return content; }
    QString getSender() const { return sender; }
    QDateTime getTimestamp() const { return timestamp; }
    bool getReadStatus() const { return isRead; }

    // Methods
    void markAsRead();
    bool operator==(const Message &other) const;
    
    // Serialization
    QString toString() const;
    static Message fromString(const QString &str);
};

#endif // MESSAGE_H