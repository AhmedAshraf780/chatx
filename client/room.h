#ifndef ROOM_H
#define ROOM_H

#include <QString>
#include <QVector>
#include <QList> // 
#include <QDateTime>
#include "message.h"

class Room {
private:
    QString roomId;
    QString name;
    QList<Message> messages;
    QDateTime lastActivity;

public:
    Room(const QString& name);
    ~Room();
    
    // Getters
    QString getRoomId() const { return roomId; }
    QString getName() const { return name; }
    
    // Message access methods with List implementation
    QList<Message> getMessages() const { return messages; } // 

    
    QVector<Message> getMessagesAsVector() const;
    
    // Get the most recent message without removing it
    Message getLatestMessage() const;
    
    QDateTime getLastActivity() const { return lastActivity; }
    
    // Setter for roomId (for ensuring consistency)
    void setRoomId(const QString& id) { roomId = id; }
    
    // Message setters
    void setMessages(const QList<Message>& msgs) { messages = msgs; }
    void setMessages(const QVector<Message>& msgs);

    // Message management
    void addMessage(const Message& msg);
    void removeMessage(int index);
    void clearMessages();
    void loadMessages();  // Load from file to memory
    void saveMessages();  // Save from memory to file
    void updateLastActivity();

    // Static helper method to generate consistent room ID based on usernames without hashing
    static QString generateRoomId(const QString& user1, const QString& user2);
};

#endif // ROOM_H