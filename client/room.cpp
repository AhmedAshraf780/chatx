#include "room.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>

QString Room::generateRoomId(const QString& user1, const QString& user2) {
    // Ensure we have complete email addresses
    qDebug() << "Generating room ID for users:" << user1 << "and" << user2;
    
    // No need to normalize - we should always be getting email addresses
    // Sort the user IDs to ensure consistent room ID regardless of order
    QString roomId = user1 < user2 ? 
                    user1 + "_" + user2 : 
                    user2 + "_" + user1;
    
    qDebug() << "Generated room ID:" << roomId;
    return roomId;
}

Room::Room(const QString& name) : name(name) {
    // Extract user IDs from the room name
    QStringList userIds = name.split("_");
    if (userIds.size() == 2) {
        // Use the sorted user IDs directly as the room ID
        roomId = generateRoomId(userIds[0], userIds[1]);
    } else {
        // Fallback if name format is unexpected
        roomId = name;
    }
    lastActivity = QDateTime::currentDateTime();
}

// Convert QVector to QList and set messages
void Room::setMessages(const QVector<Message>& msgs) {
    messages.clear();
    
    // Add messages in chronological order to the list
    for (int i = 0; i < msgs.size(); i++) {
        messages.append(msgs.at(i));
    }
}

// Convert messages list to vector for compatibility
QVector<Message> Room::getMessagesAsVector() const {
    QVector<Message> result;
    
    // Simply copy from list to vector (already in chronological order)
    for (const Message& msg : messages) {
        result.append(msg);
    }
    
    return result;
}

// Get the latest message
Message Room::getLatestMessage() const {
    if (!messages.isEmpty()) {
        return messages.last();  // Return last message (newest) in the list
    }
    return Message(); // Return empty message if list is empty
}

void Room::addMessage(const Message& msg) {
    // Add new message to the end of the list
    messages.append(msg);
    lastActivity = QDateTime::currentDateTime();
}

void Room::removeMessage(int index) {
    if (index >= 0 && index < messages.size()) {
        messages.removeAt(index);
    }
}

void Room::clearMessages() {
    messages.clear();
}

void Room::loadMessages() {
    messages.clear(); // Clear existing messages to avoid duplicates

    // Make sure roomId is clean (no whitespace)
    roomId = roomId.trimmed();
    
    // First attempt with the direct room ID
    QString roomFile = "../db/rooms/" + roomId + ".txt";
    QFile file(roomFile);
    
    // If the file doesn't exist directly, try a case-insensitive search
    if (!file.exists()) {
        qDebug() << "Room file not found directly:" << roomFile;
        QDir roomsDir("../db/rooms");
        QStringList roomFiles = roomsDir.entryList(QDir::Files);
        
        bool foundFile = false;
        for (const QString &possibleFile : roomFiles) {
            QString fileId = possibleFile;
            if (fileId.endsWith(".txt")) fileId.chop(4);
            
            if (fileId.compare(roomId, Qt::CaseInsensitive) == 0) {
                roomFile = "../db/rooms/" + possibleFile;
                qDebug() << "Found room file with case-insensitive match:" << possibleFile;
                foundFile = true;
                file.setFileName(roomFile);
                break;
            }
        }
        
        if (!foundFile) {
            qDebug() << "WARNING: No matching room file found for ID:" << roomId;
        }
    }

    qDebug() << "Loading messages from file:" << roomFile;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                Message msg = Message::fromString(line);
                messages.append(msg);
            }
        }
        file.close();
        
        qDebug() << "Loaded" << messages.size() << "messages from file";
    } else {
        qDebug() << "Failed to open file for reading:" << file.errorString();
    }
}

void Room::updateLastActivity() {
    lastActivity = QDateTime::currentDateTime();
}

void Room::saveMessages() {
    QString roomFile = "../db/rooms/" + roomId + ".txt";
    qDebug() << "Saving all messages to file:" << roomFile;
    
    // Create the rooms directory if it doesn't exist
    QDir dir("../db/rooms");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QFile file(roomFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        
        // Just iterate through the list directly (already in chronological order)
        for (const Message& msg : messages) {
            out << msg.toString() << "\n";
        }
        
        file.close();
        qDebug() << "Saved" << messages.size() << "messages to file";
    } else {
        qDebug() << "Failed to open file for writing:" << file.errorString();
    }
}

Room::~Room() {
    // Save messages before destruction
    saveMessages();
    qDebug() << "Room" << roomId << "destroyed and messages saved";
}
