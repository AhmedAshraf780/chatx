#ifndef ROOM_H
#define ROOM_H

#include <QString>
#include <QVector>
#include <QDateTime>
#include "message.h"

class Room {
private:
    QString roomId;
    QString name;
    QVector<Message> messages;
    QDateTime lastActivity;

public:
    Room(const QString& name);
    ~Room();
    
    // Getters
    QString getRoomId() const { return roomId; }
    QString getName() const { return name; }
    QVector<Message> getMessages() const { return messages; }
    QDateTime getLastActivity() const { return lastActivity; }

    // Message management
    void addMessage(const Message& msg);
    void removeMessage(int index);
    void clearMessages();
    void loadMessages(); // Added declaration for loadMessages
    void updateLastActivity(); // Added declaration for updateLastActivity
};

#endif // ROOM_H