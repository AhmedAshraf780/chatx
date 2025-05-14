#ifndef CLIENT_H
#define CLIENT_H

#include <QString>
#include <QVector>
#include <QMap>
#include "room.h"

class Client {
private:
    QString userId;
    QString username;
    QString email;
    QVector<QString> contacts;
    QMap<QString, Room*> rooms;  // Map of roomId to Room pointer

public:
    Client(const QString& userId, const QString& username, const QString& email);
    ~Client();

    // Getters
    QString getUserId() const { return userId; }
    QString getUsername() const { return username; }
    QString getEmail() const { return email; }
    QVector<QString> getContacts() const { return contacts; }

    // Contact management
    void addContact(const QString& contactId);
    void removeContact(const QString& contactId);
    bool hasContact(const QString& contactId) const;

    // Room management
    Room* getRoom(const QString& roomId);
    Room* getRoomWithUser(const QString& userId);  // Find room by user
    Room* createRoom(const QString& otherUserId);  // Create room with a user
    void removeRoom(const QString& roomId);
    QVector<Room*> getAllRooms() const;
    
    // Direct room object management (for memory-based approach)
    void addRoom(Room* room);  // Add an existing room object

    // File operations
    void saveContacts();
    void loadContacts();
};

#endif // CLIENT_H 