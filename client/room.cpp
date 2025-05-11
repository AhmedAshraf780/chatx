#include "room.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCryptographicHash>

Room::Room(const QString& name) : name(name) {
    // Create a consistent room ID based on the room name
    QByteArray hash = QCryptographicHash::hash(name.toUtf8(), QCryptographicHash::Md5);
    roomId = hash.toHex();
    lastActivity = QDateTime::currentDateTime();
}

void Room::addMessage(const Message& msg) {
    // Check if the message already exists in memory
    if (!messages.contains(msg)) {
        messages.append(msg);
        lastActivity = QDateTime::currentDateTime();

        // Save the message to the room's file
        QString roomFile = "../db/rooms/" + roomId + ".txt";
        QFile file(roomFile);

        // Open the file in append mode
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << msg.toString() << "\n";
            file.close();
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
    QFile file(roomFile);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                Message msg = Message::fromString(line);
                // if (!messages.contains(msg)) {
                    messages.append(msg);
                // }
            }
        }
        file.close();
    }
}

Room::~Room() {

}
