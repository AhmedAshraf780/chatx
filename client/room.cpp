#include "room.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>

/*
 * Contributor: Qobesy
 * Function: generateRoomId
 * Description: Creates a consistent room ID for two users regardless of the order.
 * Sorts user IDs alphabetically to ensure the same room ID is generated
 * whether user1 contacts user2 or vice versa.
 */
QString Room::generateRoomId(const QString& user1, const QString& user2) {
    // Ensure we have complete email addresses
    
    // Normalize user IDs to ensure they're complete email addresses
    QString normalizedUser1 = user1;
    QString normalizedUser2 = user2;
    
    // Check if email addresses are complete - but don't add @gmail.com suffix
    // as it may cause inconsistency with how users are saved in the database
    
    // Sort the user IDs to ensure consistent room ID regardless of order
    QString roomId = normalizedUser1 < normalizedUser2 ? 
                    normalizedUser1 + "_" + normalizedUser2 : 
                    normalizedUser2 + "_" + normalizedUser1;
    
    return roomId;
}

/*
 * Contributor: Qobesy
 * Function: Room constructor
 * Description: Creates a new room with the given name.
 * Extracts user IDs from the room name and generates a consistent room ID.
 * Sets the last activity timestamp to the current time.
 */
Room::Room(const QString& name) : name(name) {
    // Extract user IDs from the room name
    QStringList userIds = name.split("_");
    if (userIds.size() == 2) {
        // Use the sorted user IDs to ensure consistency
        roomId = generateRoomId(userIds[0], userIds[1]);
    } else {
        // Fallback if name format is unexpected
        roomId = name;
    }
    lastActivity = QDateTime::currentDateTime();
}

/*
 * Contributor: Qobesy
 * Function: addMessage
 * Description: Adds a new message to the room if it doesn't already exist.
 * Updates the last activity timestamp to the current time.
 */
void Room::addMessage(const Message& msg) {
    // Check if the message already exists in memory
    if (!messages.contains(msg)) {
        messages.append(msg);
        lastActivity = QDateTime::currentDateTime();
    }
}

/*
 * Contributor: Qobesy
 * Function: removeMessage
 * Description: Removes a message at the specified index from the room's message list.
 */
void Room::removeMessage(int index) {
    if (index >= 0 && index < messages.size()) {
        messages.removeAt(index);
    }
}

/*
 * Contributor: Qobesy
 * Function: clearMessages
 * Description: Safely clears all messages from the room.
 * Creates a new empty vector to replace the existing messages collection.
 */
void Room::clearMessages() {
    // Safely clear messages
    messages = QVector<Message>(); // Replace with a new, empty vector
}

/*
 * Contributor: Qobesy
 * Function: loadMessages
 * Description: Loads messages from the room's file into memory.
 * Reads each line, parses it into a Message object, and adds it to the messages collection.
 */
void Room::loadMessages() {
    messages.clear(); // Clear existing messages to avoid duplicates

    QString roomFile = "../db/rooms/" + roomId + ".txt";
    QFile file(roomFile);

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
    } else {
    }
}

/*
 * Contributor: Qobesy
 * Function: updateLastActivity
 * Description: Updates the room's last activity timestamp to the current time.
 * Used when a new action occurs in the room, such as sending a message.
 */
void Room::updateLastActivity() {
    lastActivity = QDateTime::currentDateTime();
}

/*
 * Contributor: Qobesy
 * Function: saveMessages
 * Description: Saves all messages in the room to a persistent file.
 * Creates the necessary directories if they don't exist.
 * Safely handles the serialization process to prevent data loss.
 */
void Room::saveMessages() {
    // Skip if roomId is empty to prevent errors
    if (roomId.isEmpty()) {
        return;
    }
    
    // Skip if there are no messages to save
    if (messages.isEmpty()) {
        return;
    }
    
    // Make a safe copy of the messages to prevent concurrent modification issues
    QVector<Message> messagesToSave = safelyCopyMessages();
    
    QString roomFile = "../db/rooms/" + roomId + ".txt";
    
    // Create the rooms directory if it doesn't exist
    QDir dir("../db/rooms");
    if (!dir.exists()) {
        bool created = dir.mkpath(".");
        if (!created) {
            return;
        }
    }
    
    QFile file(roomFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        
        for (const Message& msg : messagesToSave) {
            out << msg.toString() << "\n";
        }
        
        file.close();
    } else {
    }
}

/*
 * Contributor: Qobesy
 * Function: safelyCopyMessages
 * Description: Creates a safe copy of the messages vector to prevent concurrent modification issues.
 * Returns a new vector containing copies of all messages in the room.
 */
QVector<Message> Room::safelyCopyMessages() const {
    QVector<Message> copy;
    
    // Check if original vector is valid before copying
    if (!messages.isEmpty()) {
        // Create a fresh vector of the right size
        copy.reserve(messages.size());
        
        // Copy each message individually 
        for (int i = 0; i < messages.size(); ++i) {
            copy.append(messages.at(i));
        }
    }
    
    return copy;
}

/*
 * Contributor: Qobesy
 * Function: Room destructor
 * Description: Cleans up resources and saves messages before the room is destroyed.
 * Includes safe exception handling to prevent crashes during application shutdown.
 */
Room::~Room() {
    try {
        // Only proceed if we have messages and the directory exists
        QDir roomsDir("../db/rooms");
        if (roomsDir.exists() && !messages.isEmpty()) {
            // Make a safe copy of messages before clearing the original vector
            QVector<Message> messagesToSave = safelyCopyMessages();
            
            // Clear original vector first to prevent any issues during destruction
            clearMessages();
            
            // Save the copied messages
            if (!messagesToSave.isEmpty() && !roomId.isEmpty()) {
                QString roomFile = "../db/rooms/" + roomId + ".txt";
                QFile file(roomFile);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&file);
                    for (const Message& msg : messagesToSave) {
                        out << msg.toString() << "\n";
                    }
                    file.close();
                }
            }
        } else {
            // Just clear messages to be safe
            clearMessages();
        }
    } catch (const std::exception& e) {
        // Just clear messages to be safe in case of exception
        clearMessages();
    }
}
