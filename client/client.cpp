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
    
    // Log detailed room lookup information - ENHANCED DEBUG
    qDebug() << "\n\n======== ENHANCED ROOM LOOKUP DEBUG ========";
    qDebug() << "Current user: " << thisUserEmail;
    qDebug() << "Target user: " << otherUserEmail;
    
    // First try exact match with the consistent ID generation
    QString expectedRoomId = Room::generateRoomId(thisUserEmail, otherUserEmail);
    qDebug() << "Generated expected room ID: '" << expectedRoomId << "'";
    
    // Debug all available rooms with more details
    qDebug() << "Available rooms in memory (" << rooms.size() << "):";
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        qDebug() << " - Room ID: '" << it.key() << "' Room Name: '" << it.value()->getName() << "'";
        qDebug() << "   Room ID equal to expected? " << (it.key() == expectedRoomId ? "YES" : "NO");
        qDebug() << "   Case insensitive match? " << (it.key().compare(expectedRoomId, Qt::CaseInsensitive) == 0 ? "YES" : "NO");
    }
    
    // Check for exact match (case insensitive)
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        if (it.key().compare(expectedRoomId, Qt::CaseInsensitive) == 0) {
            qDebug() << "Found existing room by case-insensitive ID: '" << it.key() << "'";
            qDebug() << "======== END ENHANCED ROOM LOOKUP DEBUG ========\n\n";
            return it.value();
        }
    }
    
    if (rooms.contains(expectedRoomId)) {
        qDebug() << "Found existing room by exact ID match: '" << expectedRoomId << "'";
        qDebug() << "======== END ENHANCED ROOM LOOKUP DEBUG ========\n\n";
        return rooms[expectedRoomId];
    }
    
    // Log that we're falling back to alternative search methods
    qDebug() << "Room not found by expected ID, trying fallback methods...";
    
    // Check for an exact file match first - NEW DIRECT FILE CHECK
    QString roomFilePath = "../db/rooms/" + expectedRoomId + ".txt";
    QFile roomFile(roomFilePath);
    if (roomFile.exists()) {
        qDebug() << "FOUND ROOM FILE DIRECTLY at: " << roomFilePath;
        qDebug() << "But no matching room object in memory! Creating new room object.";
        
        // Create a new room object for this file
        Room *newRoom = new Room(expectedRoomId);
        newRoom->loadMessages();
        rooms[expectedRoomId] = newRoom;
        
        qDebug() << "Created and loaded new room object from file: " << expectedRoomId;
        qDebug() << "======== END ENHANCED ROOM LOOKUP DEBUG ========\n\n";
        return newRoom;
    } else {
        qDebug() << "Room file does NOT exist directly at: " << roomFilePath;
    }
    
    // Try to read all files in the rooms directory
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);
    qDebug() << "All room files in directory (" << roomFiles.size() << "):";
    for (const QString &file : roomFiles) {
        qDebug() << " - '" << file << "'";
        
        QString fileNameWithoutExt = file;
        if (fileNameWithoutExt.endsWith(".txt")) {
            fileNameWithoutExt.chop(4);
        }
        
        // Check if this file matches our expected room ID
        if (fileNameWithoutExt.compare(expectedRoomId, Qt::CaseInsensitive) == 0) {
            qDebug() << "FOUND MATCHING ROOM FILE: '" << file << "'";
            qDebug() << "But no matching room object in memory! Creating new room object.";
            
            // Create a new room object for this file
            Room *newRoom = new Room(fileNameWithoutExt);
            newRoom->loadMessages();
            rooms[fileNameWithoutExt] = newRoom;
            
            qDebug() << "Created and loaded new room object from file: " << fileNameWithoutExt;
            qDebug() << "======== END ENHANCED ROOM LOOKUP DEBUG ========\n\n";
            return newRoom;
        }
    }
    
    // Legacy fallback - try to find a room containing both user IDs exactly as they are
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        Room* room = it.value();
        QString roomId = room->getRoomId();
        qDebug() << "Checking room ID: '" << roomId << "'";
        
        // Check room ID parts
        QStringList roomUsers = roomId.split('_');
        qDebug() << "  Room parts:";
        for (const QString &part : roomUsers) {
            qDebug() << "    - '" << part << "'";
        }
        
        // Check if this room contains both users (using exact ID matches)
        bool containsThisUser = false;
        bool containsOtherUser = false;
        
        // Case insensitive comparison for each part
        for (const QString &part : roomUsers) {
            if (part.compare(thisUserEmail, Qt::CaseInsensitive) == 0) {
                containsThisUser = true;
                qDebug() << "  Found current user in room part: '" << part << "'";
            }
            if (part.compare(otherUserEmail, Qt::CaseInsensitive) == 0) {
                containsOtherUser = true;
                qDebug() << "  Found target user in room part: '" << part << "'";
            }
        }
        
        if (containsThisUser && containsOtherUser) {
            qDebug() << "Found existing room by matching both users: '" << roomId << "'";
            qDebug() << "======== END ENHANCED ROOM LOOKUP DEBUG ========\n\n";
            return room;
        }
    }
    
    // If we reach here, we still haven't found a room
    // Check if we should create a new room
    qDebug() << "No room found for users " << thisUserEmail << " and " << otherUserEmail;
    qDebug() << "======== END ENHANCED ROOM LOOKUP DEBUG ========\n\n";
    return nullptr;
}

Room *Client::createRoom(const QString &otherUserId) {
    qDebug() << "\n\n======== CREATING ROOM ========";
    qDebug() << "Attempting to create room with user: " << otherUserId;
    
    // First check if a room already exists with this user
    Room *existingRoom = getRoomWithUser(otherUserId);
    if (existingRoom) {
        qDebug() << "Using existing room: '" << existingRoom->getRoomId() << "'";
        qDebug() << "======== COMPLETED CREATING ROOM ========\n\n";
        return existingRoom;
    }

    // Always use this user's email (not nickname) for consistency
    QString thisUserEmail = this->email; // Use the email property
    
    // For the other user, ensure we're using their email
    QString otherUserEmail = otherUserId;
    
    // Debug info
    qDebug() << "Creating new room between '" << thisUserEmail << "' and '" << otherUserEmail << "'";

    // Create a consistent room name by combining user IDs in alphabetical order
    QString roomName = Room::generateRoomId(thisUserEmail, otherUserEmail);
    qDebug() << "Generated room name/id: '" << roomName << "'";

    // Create new room with consistent ID
    Room *room = new Room(roomName);
    
    // Make sure the room ID is correctly set (should be the same as roomName)
    if (room->getRoomId() != roomName) {
        qDebug() << "WARNING: Room ID mismatch! Expected: '" << roomName << "' Got: '" << room->getRoomId() << "'";
        room->setRoomId(roomName);
        qDebug() << "Fixed room ID to: '" << room->getRoomId() << "'";
    }
    
    // Add to room map
    rooms[room->getRoomId()] = room;
    qDebug() << "Added room to memory with ID: '" << room->getRoomId() << "'";
    
    // Create the room file on disk if it doesn't exist
    QString roomFilePath = "../db/rooms/" + roomName + ".txt";
    QFile roomFile(roomFilePath);
    if (!roomFile.exists()) {
        if (roomFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            roomFile.close();
            qDebug() << "Created new empty room file at: " << roomFilePath;
        } else {
            qDebug() << "ERROR: Failed to create room file: " << roomFile.errorString();
        }
    }
    
    // Save contacts to update the user file with the new room
    saveContacts();
    qDebug() << "Saved contacts with new room information";
    
    qDebug() << "======== COMPLETED CREATING ROOM ========\n\n";
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
    qDebug() << "\n\n======== LOADING CONTACTS AND ROOMS ========";
    qDebug() << "Loading contacts and rooms for user:" << userId;
    qDebug() << "Loading from file:" << filename;
    
    // First, debug all room files in the database
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);
    qDebug() << "Available room files in database (" << roomFiles.size() << "):";
    for (const QString &roomFile : roomFiles) {
        QString fileId = roomFile;
        if (fileId.endsWith(".txt")) fileId.chop(4);
        qDebug() << " - Room file: '" << fileId << "'";
    }
    
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        contacts.clear();
        rooms.clear(); // Clear existing rooms to prevent duplicates
        qDebug() << "Cleared existing contacts and rooms in memory.";
        
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("CONTACT:")) {
                QString contactId = line.mid(8).trimmed();
                contacts.append(contactId);
                qDebug() << "Loaded contact: '" << contactId << "'";
            } else if (line.startsWith("ROOM:")) {
                QStringList parts = line.mid(5).split('|');
                if (parts.size() >= 2) {
                    QString roomId = parts[0].trimmed(); // Trim any whitespace
                    QString roomName = parts[1].trimmed(); // Trim any whitespace
                    qDebug() << "Found room in user file - ID: '" << roomId << "' Name: '" << roomName << "'";

                    // Check if room file exists first (in any case)
                    bool roomFileExists = false;
                    QString actualRoomFileName = "";
                    
                    // Direct check first
                    QString directPath = "../db/rooms/" + roomId + ".txt";
                    if (QFile::exists(directPath)) {
                        roomFileExists = true;
                        actualRoomFileName = roomId;
                        qDebug() << "Room file exists directly at: " << directPath;
                    } else {
                        // Case-insensitive search
                        for (const QString &roomFile : roomFiles) {
                            QString fileId = roomFile;
                            if (fileId.endsWith(".txt")) fileId.chop(4);
                            
                            if (fileId.compare(roomId, Qt::CaseInsensitive) == 0) {
                                roomFileExists = true;
                                actualRoomFileName = fileId;
                                qDebug() << "Found room file with case-insensitive match: '" << fileId << "'";
                                break;
                            }
                        }
                    }
                    
                    if (roomFileExists) {
                        // Create room directly with the name from file
                        Room *room = new Room(roomName);
                        
                        // Set the exact room ID to match what's on disk
                        if (!actualRoomFileName.isEmpty()) {
                            room->setRoomId(actualRoomFileName);
                            qDebug() << "Set room ID from actual file: '" << actualRoomFileName << "'";
                        } else {
                            room->setRoomId(roomId);
                            qDebug() << "Set room ID from user file: '" << roomId << "'";
                        }
                        
                        // Store the room in the rooms map
                        rooms[room->getRoomId()] = room;
                        qDebug() << "Added room to memory - ID: '" << room->getRoomId() << "' Name: '" << room->getName() << "'";
                        
                        // Pre-load messages for rooms - OPTIONAL BUT HELPFUL
                        room->loadMessages();
                        qDebug() << "Loaded " << room->getMessages().size() << " messages for room";
                    } else {
                        qDebug() << "WARNING: Room file does not exist for: '" << roomId << "', creating empty file";
                        
                        // Generate correct room ID for consistency
                        QStringList userParts = roomName.split('_');
                        if (userParts.size() == 2) {
                            // Create the file if it doesn't exist
                            QFile newRoomFile("../db/rooms/" + roomId + ".txt");
                            if (newRoomFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                                newRoomFile.close();
                                qDebug() << "Created new empty room file: '" << roomId << ".txt'";
                                
                                // Create the room in memory
                                Room *room = new Room(roomName);
                                room->setRoomId(roomId);
                                rooms[roomId] = room;
                                qDebug() << "Added new empty room to memory";
                            } else {
                                qDebug() << "ERROR: Failed to create room file: " << newRoomFile.errorString();
                            }
                        } else {
                            qDebug() << "ERROR: Room name is not in expected format: '" << roomName << "'";
                        }
                    }
                }
            }
        }
        
        file.close();
        qDebug() << "Completed loading " << contacts.size() << " contacts and " << rooms.size() << " rooms";
    } else {
        qDebug() << "ERROR: Failed to open user file: " << file.errorString();
    }
    
    // Verify rooms were properly loaded
    qDebug() << "Rooms after loading (" << rooms.size() << "):";
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        qDebug() << " - Room ID: '" << it.key() << "' Name: '" << it.value()->getName() << "'";
    }
    
    qDebug() << "======== COMPLETED LOADING CONTACTS AND ROOMS ========\n\n";
}

void Client::addRoom(Room* room) {
    if (!room) return;
    
    // Store room by its ID
    rooms[room->getRoomId()] = room;
    qDebug() << "Added existing room:" << room->getRoomId() << "to client" << userId;
}
