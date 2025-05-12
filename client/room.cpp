#include "room.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>

QString Room::generateRoomId(const QString& user1, const QString& user2) {
    // Ensure we have complete email addresses
    qDebug() << "Generating room ID for users:" << user1 << "and" << user2;
    
    // Normalize user IDs to ensure they're complete email addresses
    QString normalizedUser1 = user1;
    QString normalizedUser2 = user2;
    
    // Check if email addresses are complete
    if (!normalizedUser1.contains("@")) {
        normalizedUser1 = normalizedUser1 + "@gmail.com";
        qDebug() << "Normalized user1 to:" << normalizedUser1;
    }
    
    if (!normalizedUser2.contains("@")) {
        normalizedUser2 = normalizedUser2 + "@gmail.com";
        qDebug() << "Normalized user2 to:" << normalizedUser2;
    }
    
    // Sort the user IDs to ensure consistent room ID regardless of order
    QString roomId = normalizedUser1 < normalizedUser2 ? 
                    normalizedUser1 + "_" + normalizedUser2 : 
                    normalizedUser2 + "_" + normalizedUser1;
    
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

void Room::addMessage(const Message& msg) {
    // Check if the message already exists in memory
    if (!messages.contains(msg)) {
        messages.append(msg);
        lastActivity = QDateTime::currentDateTime();

        // Save the message to the room's file
        QString roomFile = "../db/rooms/" + roomId + ".txt";
        qDebug() << "Saving message to file:" << roomFile;
        QFile file(roomFile);

        // Open the file in append mode
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << msg.toString() << "\n";
            file.close();
        } else {
            qDebug() << "Failed to open file for writing:" << file.errorString();
        }
    }
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

    QString roomFile = "../db/rooms/" + roomId + ".txt";
    qDebug() << "Loading messages from file:" << roomFile;
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
        qDebug() << "Loaded" << messages.size() << "messages from file";
    } else {
        qDebug() << "Failed to open file for reading:" << file.errorString();
    }
}

void Room::updateLastActivity() {
    lastActivity = QDateTime::currentDateTime();
}

Room::~Room() {

}
