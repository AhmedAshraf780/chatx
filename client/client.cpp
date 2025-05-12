#include "client.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QDebug>

Client::Client(const QString &userId, const QString &username,
               const QString &email)
    : userId(userId), username(username), email(email) {
    qDebug() << "Creating client for user:" << userId;
    loadContacts();
}

Client::~Client() {
    saveContacts();
    // Clean up rooms
    for (Room *room : rooms) {
        delete room;
    }
    rooms.clear();
}

void Client::addContact(const QString &contactId) {
    if (!hasContact(contactId)) {
        contacts.append(contactId);
        saveContacts();
    }
}

void Client::removeContact(const QString &contactId) {
    contacts.removeOne(contactId);
    saveContacts();
}

bool Client::hasContact(const QString &contactId) const {
    return contacts.contains(contactId);
}

Room *Client::getRoom(const QString &roomId) {
    if (rooms.contains(roomId)) {
        return rooms[roomId];
    }
    return nullptr;
}

Room *Client::getRoomWithUser(const QString &userId) {
    // Always use this user's email for consistency
    QString thisUserEmail = this->email;
    
    // For the other user, ensure we're using their email
    QString otherUserEmail = userId;
    
    // Find a room that contains this user using the consistent ID generation
    QString expectedRoomId = Room::generateRoomId(thisUserEmail, otherUserEmail);
    qDebug() << "Looking for room with ID:" << expectedRoomId;
    
    if (rooms.contains(expectedRoomId)) {
        qDebug() << "Found existing room by ID";
        return rooms[expectedRoomId];
    }
    
    // Legacy fallback - look by name or by user ID parts
    for (Room *room : rooms) {
        // Check room name parts
        QStringList roomUsers = room->getName().split("_");
        
        // Check if this room contains both users (using either email or user ID)
        bool containsThisUser = roomUsers.contains(thisUserEmail) || roomUsers.contains(userId);
        bool containsOtherUser = roomUsers.contains(otherUserEmail) || roomUsers.contains(userId);
        
        if (containsThisUser && containsOtherUser) {
            qDebug() << "Found existing room by name:" << room->getName();
            return room;
        }
    }
    
    qDebug() << "No room found with user:" << userId;
    return nullptr;
}

Room *Client::createRoom(const QString &otherUserId) {
    // First check if a room already exists with this user
    Room *existingRoom = getRoomWithUser(otherUserId);
    if (existingRoom) {
        qDebug() << "Using existing room:" << existingRoom->getRoomId();
        return existingRoom;
    }

    // Always use this user's email (not nickname) for consistency
    QString thisUserEmail = this->email; // Use the email property
    
    // For the other user, ensure we're using their email
    QString otherUserEmail = otherUserId;
    
    // Debug info
    qDebug() << "Creating new room between" << thisUserEmail << "and" << otherUserEmail;

    // Create a consistent room name by combining user IDs in alphabetical order
    QString roomName = Room::generateRoomId(thisUserEmail, otherUserEmail);
    qDebug() << "Creating new room with name:" << roomName;

    // Create new room with consistent ID
    Room *room = new Room(roomName);
    rooms[room->getRoomId()] = room;
    qDebug() << "Created room with ID:" << room->getRoomId();
    
    return room;
}

void Client::removeRoom(const QString &roomId) {
    if (rooms.contains(roomId)) {
        delete rooms[roomId];
        rooms.remove(roomId);
    }
}

QVector<Room *> Client::getAllRooms() const {
    return rooms.values().toVector();
}

void Client::saveContacts() {
    // Create users directory if it doesn't exist
    QDir dir;
    if (!dir.exists("../db/users")) {
        dir.mkpath("../db/users");
    }

    QString filename = "../db/users/" + userId + ".txt";
    qDebug() << "Saving contacts to file:" << filename;
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        
        // Save contacts
        for (const QString &contactId : contacts) {
            out << "CONTACT:" << contactId << "\n";
        }

        // Save room information
        for (Room *room : rooms) {
            out << "ROOM:" << room->getRoomId() << "|" << room->getName() << "\n";
            qDebug() << "Saved room info:" << room->getRoomId() << "|" << room->getName();
        }
        
        file.close();
    } else {
        qDebug() << "Failed to open file for writing:" << file.errorString();
    }
}

void Client::loadContacts() {
    QString filename = "../db/users/" + userId + ".txt";
    qDebug() << "Loading contacts from file:" << filename;
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        contacts.clear();
        rooms.clear(); // Clear existing rooms to prevent duplicates
        
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("CONTACT:")) {
                QString contactId = line.mid(8);
                contacts.append(contactId);
                qDebug() << "Loaded contact:" << contactId;
            } else if (line.startsWith("ROOM:")) {
                QStringList parts = line.mid(5).split('|');
                if (parts.size() >= 2) {
                    QString roomId = parts[0];
                    QString roomName = parts[1];
                    qDebug() << "Loading room data:" << roomId << "|" << roomName;

                    // Create room if it doesn't exist
                    Room *room = getRoom(roomId);
                    if (!room) {
                        // Create room directly with the name from file
                        room = new Room(roomName);
                        // Make sure we use the exact roomId from the file
                        if (room->getRoomId() != roomId) {
                            qDebug() << "Warning: Room ID mismatch. File:" << roomId 
                                    << "Generated:" << room->getRoomId();
                            room->setRoomId(roomId);
                        }
                        rooms[roomId] = room;
                        qDebug() << "Created room with ID:" << roomId;
                    }

                    // Load room messages
                    room->loadMessages();
                }
            }
        }
        file.close();
    } else {
        qDebug() << "Could not open file for reading:" << file.errorString();
    }
}
