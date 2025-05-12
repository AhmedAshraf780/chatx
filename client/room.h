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
    
    // Setter for roomId (for ensuring consistency)
    void setRoomId(const QString& id) { roomId = id; }

    // Message management
    void addMessage(const Message& msg);
    void removeMessage(int index);
    void clearMessages();
    void loadMessages();
    void updateLastActivity();

    // Static helper method to generate consistent room ID based on usernames without hashing
    static QString generateRoomId(const QString& user1, const QString& user2);
};

#endif // ROOM_H