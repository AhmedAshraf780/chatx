#include "client.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

Client::Client(const QString &userId, const QString &username,
               const QString &email)
    : userId(userId), username(username), email(email) {
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
    // Find a room that contains this user
    for (Room *room : rooms) {
        QStringList roomUsers = room->getName().split("_");
        if (roomUsers.contains(userId) && roomUsers.contains(this->userId)) {
            return room;
        }
    }
    return nullptr;
}

Room *Client::createRoom(const QString &otherUserId) {
    // First check if a room already exists with this user
    Room *existingRoom = getRoomWithUser(otherUserId);
    if (existingRoom) {
        return existingRoom;
    }

    // Create a consistent room name by combining user IDs in alphabetical order
    QString roomName;
    if (userId < otherUserId) {
        roomName = userId + "_" + otherUserId;
    } else {
        roomName = otherUserId + "_" + userId;
    }

    // Create new room
    Room *room = new Room(roomName);
    rooms[room->getRoomId()] = room;
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
        }
    }
}

void Client::loadContacts() {
    QString filename = "../db/users/" + userId + ".txt";
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
            } else if (line.startsWith("ROOM:")) {
                QStringList parts = line.mid(5).split('|');
                if (parts.size() >= 2) {
                    QString roomId = parts[0];
                    QString roomName = parts[1];

                    // Create room if it doesn't exist
                    Room *room = getRoom(roomId);
                    if (!room) {
                        // Extract the other user's ID from the room name
                        QStringList userIds = roomName.split("_");
                        QString otherUserId = userIds[0] == this->userId ? userIds[1] : userIds[0];
                        room = createRoom(otherUserId);
                    }

                    // Load room messages
                    QString roomFile = "../db/rooms/" + roomId + ".txt";
                    QFile roomDataFile(roomFile);
                    if (roomDataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QTextStream roomIn(&roomDataFile);
                        while (!roomIn.atEnd()) {
                            QString msgLine = roomIn.readLine().trimmed();
                            if (!msgLine.isEmpty()) {
                                Message msg = Message::fromString(msgLine);
                                room->addMessage(msg);
                            }
                        }
                    }
                }
            }
        }
    }
}
