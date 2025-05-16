#include "client.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

/*
 * Contributor: Alaa and Malak
 * Function: Client constructor
 * Description: Initializes a new client with the given user ID, username, and email.
 * Loads existing contacts from storage during initialization.
 */
Client::Client(const QString &userId, const QString &username,
               const QString &email)
    : userId(userId), username(username), email(email) {
    loadContacts();
}

/*
 * Contributor: Alaa and Malak
 * Function: Client destructor
 * Description: Cleans up client resources by saving contacts and freeing memory
 * allocated for chat rooms before destruction.
 */
Client::~Client() {
    saveContacts();
    // Clean up rooms
    for (Room *room : rooms) {
        delete room;
    }
    rooms.clear();
}

/*
 * Contributor: Alaa and Malak
 * Function: addContact
 * Description: Adds a new contact to the client's contact list if it doesn't already exist,
 * and saves the updated contacts list to storage.
 */
void Client::addContact(const QString &contactId) {
    if (!hasContact(contactId)) {
        contacts.append(contactId);
        saveContacts();
    }
}

/*
 * Contributor: Alaa and Malak
 * Function: removeContact
 * Description: Removes a contact from the client's contact list and
 * saves the updated contacts list to storage.
 */
void Client::removeContact(const QString &contactId) {
    contacts.removeOne(contactId);
    saveContacts();
}

/*
 * Contributor: Alaa and Malak
 * Function: hasContact
 * Description: Checks if a specific user ID is in the client's contacts list.
 * Returns true if the contact exists, false otherwise.
 */
bool Client::hasContact(const QString &contactId) const {
    return contacts.contains(contactId);
}

/*
 * Contributor: Alaa and Malak
 * Function: getRoom
 * Description: Retrieves a room by its ID from the client's rooms collection.
 * Returns the room pointer if found, or nullptr if not found.
 */
Room *Client::getRoom(const QString &roomId) {
    if (rooms.contains(roomId)) {
        return rooms[roomId];
    }
    return nullptr;
}

/*
 * Contributor: Alaa and Malak
 * Function: getRoomWithUser
 * Description: Finds a chat room that connects the current client with the specified user.
 * Uses consistent room ID generation to ensure the same room is found regardless of who created it.
 */
Room *Client::getRoomWithUser(const QString &userId) {
    // Always use this user's email for consistency
    QString thisUserEmail = this->email;

    // For the other user, ensure we're using their email
    QString otherUserEmail = userId;

    // Find a room that contains this user using the consistent ID generation
    QString expectedRoomId =
        Room::generateRoomId(thisUserEmail, otherUserEmail);

    if (rooms.contains(expectedRoomId)) {
        return rooms[expectedRoomId];
    }

    // Legacy fallback - look by name or by user ID parts
    for (Room *room : rooms) {
        // Check room name parts
        QStringList roomUsers = room->getName().split("_");

        // Check if this room contains both users (using either email or user
        // ID)
        bool containsThisUser =
            roomUsers.contains(thisUserEmail) || roomUsers.contains(userId);
        bool containsOtherUser =
            roomUsers.contains(otherUserEmail) || roomUsers.contains(userId);

        if (containsThisUser && containsOtherUser) {
            return room;
        }
    }

    return nullptr;
}

/*
 * Contributor: Alaa and Malak
 * Function: createRoom
 * Description: Creates a new chat room between the current user and another user.
 * If a room already exists, returns the existing room instead of creating a new one.
 * Uses consistent room naming conventions for reliable room identification.
 */
Room *Client::createRoom(const QString &otherUserId) {
    // First check if a room already exists with this user
    Room *existingRoom = getRoomWithUser(otherUserId);
    if (existingRoom) {
        return existingRoom;
    }

    // Always use this user's email (not nickname) for consistency
    QString thisUserEmail = this->email; // Use the email property

    // For the other user, ensure we're using their email
    QString otherUserEmail = otherUserId;

    // Debug info

    // Create a consistent room name by combining user IDs in alphabetical order
    QString roomName = Room::generateRoomId(thisUserEmail, otherUserEmail);

    // Create new room with consistent ID
    Room *room = new Room(roomName);
    rooms[room->getRoomId()] = room;

    return room;
}

/*
 * Contributor: Alaa and Malak
 * Function: removeRoom
 * Description: Removes a chat room from the client's rooms collection and frees its memory.
 */
void Client::removeRoom(const QString &roomId) {
    if (rooms.contains(roomId)) {
        delete rooms[roomId];
        rooms.remove(roomId);
    }
}

/*
 * Contributor: Alaa and Malak
 * Function: getAllRooms
 * Description: Returns a vector containing all rooms that the client is participating in.
 */
QVector<Room *> Client::getAllRooms() const {
    return rooms.values().toVector();
}

/*
 * Contributor: Alaa and Malak
 * Function: saveContacts
 * Description: Persists the client's contacts and room information to a file.
 * Creates necessary directories if they don't exist.
 */
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
            out << "ROOM:" << room->getRoomId() << "|" << room->getName()
                << "\n";
        }

        file.close();
    } else {
    }
}

/*
 * Contributor: Alaa and Malak
 * Function: loadContacts
 * Description: Loads the client's contacts and room information from a file.
 * Recreates room objects from saved data and initializes the client's state.
 */
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
                        // Create room directly with the name from file
                        room = new Room(roomName);
                        // Make sure we use the exact roomId from the file
                        if (room->getRoomId() != roomId) {
                            room->setRoomId(roomId);
                        }
                        rooms[roomId] = room;
                    }

                    // Load room messages
                    room->loadMessages();
                }
            }
        }
        file.close();
    } else {
    }
}

/*
 * Contributor: Alaa and Malak
 * Function: addRoom
 * Description: Adds an existing room to the client's rooms collection.
 * Used when the client is added to a room created by someone else.
 */
void Client::addRoom(Room *room) {
    if (!room)
        return;

    // Store room by its ID
    rooms[room->getRoomId()] = room;
}
