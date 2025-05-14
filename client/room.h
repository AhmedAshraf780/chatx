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
<<<<<<< HEAD
=======
    
    // Setter for messages (for in-memory approach)
    void setMessages(const QVector<Message>& msgs) { messages = msgs; }
>>>>>>> bfd0fc2 (handle the settings)

    // Message management
    void addMessage(const Message& msg);
    void removeMessage(int index);
    void clearMessages();
<<<<<<< HEAD
    void loadMessages();
=======
    void loadMessages();  // Load from file to memory
    void saveMessages();  // Save from memory to file
>>>>>>> bfd0fc2 (handle the settings)
    void updateLastActivity();

    // Static helper method to generate consistent room ID based on usernames without hashing
    static QString generateRoomId(const QString& user1, const QString& user2);
};

#endif // ROOM_H