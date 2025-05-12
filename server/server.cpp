// server.cpp
#include "server.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <qglobal.h>

// Initialize static member
server *server::instance = nullptr;

server::server() : currentClient(nullptr) {
    loadUsersAccounts();
    migrateRoomFiles(); // Migrate any inconsistent room files
}

server *server::getInstance() {
    // Create the instance if it doesn't exist
    if (instance == nullptr) {
        instance = new server();
    }
    return instance;
}

void server::loadUsersAccounts() {
    QString filePath = "../db/users_credentials.txt";
    QFile file(filePath);
    qDebug() << "Loading users from:" << filePath;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading:" << file.errorString();
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(',');
        if (parts.size() == 3) {
            QString email = parts[0].trimmed();
            QString username = parts[1].trimmed();
            QString password = parts[2].trimmed();
            UserData userData;
            userData.username = username;
            userData.password = password;
            userMap.insert(email, userData);
            qDebug() << "Loaded user:" << email << username;
        }
    }
    file.close();
    qDebug() << "Total users loaded:" << userMap.size();
}

void server::saveUsersAccounts() {
    QString filePath = "../db/users_credentials.txt";
    QFile file(filePath);
    qDebug() << "Saving users to:" << filePath;

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for writing:" << file.errorString();
        return;
    }

    QTextStream out(&file);
    QMapIterator<QString, UserData> i(userMap);
    while (i.hasNext()) {
        i.next();
        out << i.key() << "," << i.value().username << "," << i.value().password
            << "\n";
        qDebug() << "Saved user:" << i.key() << i.value().username;
    }

    file.close();
    qDebug() << "Total users saved:" << userMap.size();
}

bool server::checkUser(const QString &email, const QString &password) {
    if (!isValidEmail(email)) {
        return false;
    }
    if (userMap.contains(email)) {
        if (userMap.value(email).password == password) {
            return true;
        }
    }
    return false;
}

void server::addUser(const QString &email, const QString &password) {
    UserData userData;
    userData.password = password;
    userMap.insert(email, userData);
    saveUsersAccounts();
}

bool server::isValidEmail(const QString &email) {
    // Basic email validation
    QRegExp emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    return emailRegex.exactMatch(email);
}

bool server::isValidPassword(const QString &password) {
    // Password must be at least 4 characters
    return password.length() >= 4;
}

bool server::registerUser(const QString &username, const QString &email,
                          const QString &password,
                          const QString &confirmPassword,
                          QString &errorMessage) {
    // Validate email format
    if (!isValidEmail(email)) {
        errorMessage =
            "Please enter a valid email address (e.g., user@example.com)";
        return false;
    }

    // Check if email already exists
    if (userMap.contains(email)) {
        errorMessage = "This email is already registered. Please use a "
                       "different email or try logging in.";
        return false;
    }

    // Validate username
    if (username.trimmed().isEmpty()) {
        errorMessage = "Please enter a nickname";
        return false;
    }

    // Validate password
    if (!isValidPassword(password)) {
        errorMessage = "Password must be at least 4 characters long";
        return false;
    }

    // Check if passwords match
    if (password != confirmPassword) {
        errorMessage = "Passwords do not match. Please make sure both "
                       "passwords are identical.";
        return false;
    }

    // Create new user
    UserData userData;
    userData.username = username;
    userData.password = password;
    userMap.insert(email, userData);

    // Save to file
    saveUsersAccounts();
    return true;
}

bool server::resetPassword(const QString &email, const QString &newPassword,
                           const QString &confirmPassword,
                           QString &errorMessage) {
    // Check if email exists
    if (!isValidEmail(email)) {
        errorMessage =
            "Please enter a valid email address (e.g., user@example.com)";
        return false;
    }

    if (!userMap.contains(email)) {
        errorMessage = "No account found with this email address. Please check "
                       "your email or sign up.";
        return false;
    }

    // Validate new password
    if (!isValidPassword(newPassword)) {
        errorMessage = "Password must be at least 4 characters long";
        return false;
    }

    // Check if passwords match
    if (newPassword != confirmPassword) {
        errorMessage = "Passwords do not match. Please make sure both "
                       "passwords are identical.";
        return false;
    }

    // Update the password
    userMap[email].password = newPassword;

    // Save changes to file
    saveUsersAccounts();
    return true;
}

QVector<QPair<QString, QString>> server::getAllUsers() const {
    QVector<QPair<QString, QString>> users;
    QMapIterator<QString, UserData> i(userMap);
    while (i.hasNext()) {
        i.next();
        users.append(qMakePair(i.key(), i.value().username));
    }
    return users;
}

Client *server::loginUser(const QString &email, const QString &password) {
    if (!checkUser(email, password)) {
        return nullptr;
    }

    // Create new client if doesn't exist
    if (!clients.contains(email)) {
        // IMPORTANT: Always use email as the userId to ensure consistency
        Client *newClient = new Client(email, userMap[email].username, email);
        clients[email] = newClient;
        loadClientData(newClient);
        qDebug() << "Created new client with userId/email:" << email << "and username:" << userMap[email].username;
    }

    currentClient = clients[email];
    return currentClient;
}

void server::logoutUser() {
    if (currentClient) {
        saveClientData(currentClient);
        currentClient = nullptr;
    }
}

void server::loadClientData(Client *client) {
    // Create necessary directories
    QDir dir;
    dir.mkpath("../db/users");
    dir.mkpath("../db/rooms");

    // Load user's data file
    QString filename = "../db/users/" + client->getUserId() + ".txt";
    QFile file(filename);
    qDebug() << "Server: Loading client data from:" << filename;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);

        // Load contacts and rooms
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("CONTACT:")) {
                QString contactId = line.mid(8);
                client->addContact(contactId);
                qDebug() << "Server: Added contact:" << contactId;
            } else if (line.startsWith("ROOM:")) {
                QStringList parts = line.mid(5).split('|');
                if (parts.size() >= 2) {
                    QString roomId = parts[0];
                    QString roomName = parts[1];
                    qDebug() << "Server: Loading room:" << roomId << roomName;

                    // Create room if it doesn't exist
                    Room *room = client->getRoom(roomId);
                    if (!room) {
                        // Extract the other user's ID from the room name
                        QStringList userIds = roomName.split("_");
                        QString otherUserId = userIds[0] == client->getUserId() ? userIds[1] : userIds[0];
                        room = client->createRoom(otherUserId);
                        qDebug() << "Server: Created room with" << otherUserId << "ID:" << room->getRoomId();
                        
                        // Make sure we're using the right room ID
                        if (room->getRoomId() != roomId) {
                            qDebug() << "Server: Warning - Room ID mismatch! File:" << roomId << "Generated:" << room->getRoomId();
                            // We should create a migration function to handle this
                        }
                    }
                }
            }
        }
        file.close();
    }

    // Load all rooms with messages
    QVector<Room *> rooms = client->getAllRooms();
    for (Room *room : rooms) {
        qDebug() << "Server: Loading messages for room:" << room->getRoomId();
        room->loadMessages();
    }

    // Also check for rooms where this user is a participant but not in their contacts
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);
    qDebug() << "Server: Checking" << roomFiles.size() << "room files for participation";
    
    for (const QString &roomFile : roomFiles) {
        QString roomId = roomFile;
        if (roomId.endsWith(".txt")) {
            roomId.chop(4); // Remove .txt extension
        }
        
        // Skip rooms we already loaded
        if (client->getRoom(roomId)) {
            continue;
        }
        
        // Check if this roomId contains the user's ID (for direct name format)
        QStringList parts = roomId.split("_");
        if (parts.contains(client->getUserId())) {
            // This is a room involving this user
            QString otherUserId;
            for (const QString &part : parts) {
                if (part != client->getUserId()) {
                    otherUserId = part;
                    break;
                }
            }
            
            if (!otherUserId.isEmpty()) {
                qDebug() << "Server: Found room with user:" << otherUserId << "ID:" << roomId;
                // Create the room
                Room *room = client->createRoom(otherUserId);
                // Load messages
                room->loadMessages();
            }
            continue;
        }
        
        // If room name is not in the direct format, we need to check message content
        QFile file(roomsDir.filePath(roomFile));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            bool hasMessages = false;
            QString otherUserId;

            // Check messages to see if this user is a participant
            while (!in.atEnd()) {
                QString msgLine = in.readLine().trimmed();
                if (!msgLine.isEmpty()) {
                    Message msg = Message::fromString(msgLine);
                    // If this user is either sender or receiver, create the room
                    if (msg.getSender() == client->getUserId()) {
                        hasMessages = true;
                        
                        // Extract room name to get other user ID
                        QStringList parts = roomId.split("_");
                        for (const QString &part : parts) {
                            if (part != client->getUserId()) {
                                otherUserId = part;
                                break;
                            }
                        }
                        break;
                    }
                }
            }

            if (hasMessages && !otherUserId.isEmpty()) {
                qDebug() << "Server: Found messages for user in room:" << roomId;
                // Create room if it doesn't exist
                Room *room = client->getRoomWithUser(otherUserId);
                if (!room) {
                    room = client->createRoom(otherUserId);
                }
                
                // Load all messages
                room->loadMessages();
            }
            file.close();
        }
    }
}

void server::saveClientData(Client *client) {
    // Create necessary directories
    QDir dir;
    dir.mkpath("../db/users");
    dir.mkpath("../db/rooms");

    // Save user's data file
    QString filename = "../db/users/" + client->getUserId() + ".txt";
    QFile file(filename);
    qDebug() << "Server: Saving client data to:" << filename;
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);

        // Save contacts
        QVector<QString> contacts = client->getContacts();
        for (const QString &contactId : contacts) {
            out << "CONTACT:" << contactId << "\n";
            qDebug() << "Server: Saved contact:" << contactId;
        }

        // Save rooms
        QVector<Room *> rooms = client->getAllRooms();
        for (Room *room : rooms) {
            out << "ROOM:" << room->getRoomId() << "|" << room->getName() << "\n";
            qDebug() << "Server: Saved room:" << room->getRoomId() << room->getName();
            
            // Room messages are now saved directly by the Room class
            // in the addMessage method, so we don't need to save them here
        }
        
        file.close();
    } else {
        qDebug() << "Server: Failed to open file for writing:" << file.errorString();
    }
}

void server::saveAllClientsData() {
    for (Client *client : clients) {
        saveClientData(client);
    }
}

server::~server() {
    saveAllClientsData();
    // Clean up clients
    for (Client *client : clients) {
        delete client;
    }
    clients.clear();
    currentClient = nullptr;
    saveUsersAccounts();
}

void server::migrateRoomFiles() {
    qDebug() << "Checking for inconsistent room files to migrate...";
    
    // Create rooms directory if it doesn't exist
    QDir dir;
    dir.mkpath("../db/rooms");
    
    // Get all room files
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);
    
    QStringList migratedFiles;
    QStringList problemFiles;
    
    // First handle nickname vs email conflicts
    qDebug() << "Looking for nickname vs email conflicts...";
    
    // Group room files by user pairs (ignoring formatting differences)
    QMap<QString, QStringList> userPairToRoomIds;
    
    for (const QString &filename : roomFiles) {
        if (!filename.endsWith(".txt")) continue;
        
        QString roomId = filename;
        roomId.chop(4); // Remove .txt extension
        
        // Check if this is a valid room ID format
        if (!roomId.contains("_")) {
            problemFiles << roomId;
            continue;
        }
        
        QStringList users = roomId.split("_");
        if (users.size() != 2) {
            problemFiles << roomId;
            continue;
        }
        
        // Normalize users to emails for consistent matching
        QString user1 = users[0];
        QString user2 = users[1];
        
        // Normalize to email format
        if (!user1.contains("@")) {
            user1 += "@gmail.com";
        }
        
        if (!user2.contains("@")) {
            user2 += "@gmail.com";
        }
        
        // Create a consistent key regardless of order
        QString userPairKey = user1 < user2 ? user1 + "+" + user2 : user2 + "+" + user1;
        
        if (!userPairToRoomIds.contains(userPairKey)) {
            userPairToRoomIds[userPairKey] = QStringList();
        }
        
        userPairToRoomIds[userPairKey].append(roomId);
    }
    
    // For any user pair with multiple room IDs, merge them
    QMapIterator<QString, QStringList> i(userPairToRoomIds);
    while (i.hasNext()) {
        i.next();
        
        if (i.value().size() > 1) {
            qDebug() << "Found conflict for users:" << i.key() << "with" << i.value().size() << "room files";
            
            // Extract the canonical users from the key
            QStringList canonicalUsers = i.key().split("+");
            QString canonicalUser1 = canonicalUsers[0];
            QString canonicalUser2 = canonicalUsers[1];
            
            // Generate the correct room ID
            QString correctRoomId = Room::generateRoomId(canonicalUser1, canonicalUser2);
            QString correctPath = "../db/rooms/" + correctRoomId + ".txt";
            
            qDebug() << "Canonical room ID should be:" << correctRoomId;
            
            // Create the correct file if it doesn't exist
            if (!QFile::exists(correctPath)) {
                QFile::copy(
                    "../db/rooms/" + i.value().first() + ".txt", 
                    correctPath
                );
            }
            
            // Merge all files into the canonical one
            for (const QString &roomId : i.value()) {
                if (roomId == correctRoomId) continue; // Skip if it's the canonical one
                
                QString roomPath = "../db/rooms/" + roomId + ".txt";
                
                qDebug() << "Merging" << roomId << "into" << correctRoomId;
                
                QFile roomFile(roomPath);
                QFile correctFile(correctPath);
                
                if (roomFile.open(QIODevice::ReadOnly | QIODevice::Text) && 
                    correctFile.open(QIODevice::Append | QIODevice::Text)) {
                    
                    QTextStream in(&roomFile);
                    QTextStream out(&correctFile);
                    
                    // Copy all messages
                    while (!in.atEnd()) {
                        QString line = in.readLine();
                        if (!line.trimmed().isEmpty()) {
                            out << line << "\n";
                        }
                    }
                    
                    roomFile.close();
                    correctFile.close();
                    
                    // Delete the duplicate file
                    QFile::remove(roomPath);
                    migratedFiles << roomId + " -> " << correctRoomId;
                }
            }
        }
    }
    
    // Now update user files to reference the correct room IDs
    QDir usersDir("../db/users");
    QStringList userFiles = usersDir.entryList(QDir::Files);
    
    for (const QString &userFile : userFiles) {
        if (!userFile.endsWith(".txt")) continue;
        
        QString userId = userFile;
        userId.chop(4); // Remove .txt extension
        
        QString filePath = "../db/users/" + userFile;
        QFile file(filePath);
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        
        QStringList lines;
        QTextStream in(&file);
        bool needsUpdate = false;
        
        while (!in.atEnd()) {
            QString line = in.readLine();
            
            if (line.startsWith("ROOM:")) {
                QStringList parts = line.mid(5).split('|');
                if (parts.size() >= 2) {
                    QString roomId = parts[0];
                    QString roomName = parts[1];
                    
                    // Check if this room ID needs normalization
                    QStringList users = roomName.split("_");
                    if (users.size() == 2) {
                        QString user1 = users[0];
                        QString user2 = users[1];
                        
                        // Normalize user IDs
                        if (!user1.contains("@")) {
                            user1 += "@gmail.com";
                        }
                        
                        if (!user2.contains("@")) {
                            user2 += "@gmail.com";
                        }
                        
                        QString correctRoomId = Room::generateRoomId(user1, user2);
                        QString correctRoomName = user1 < user2 ? user1 + "_" + user2 : user2 + "_" + user1;
                        
                        if (roomId != correctRoomId || roomName != correctRoomName) {
                            line = "ROOM:" + correctRoomId + "|" + correctRoomName;
                            needsUpdate = true;
                            qDebug() << "Updated room reference:" << roomId << "->" << correctRoomId;
                        }
                    }
                }
            }
            
            lines.append(line);
        }
        
        file.close();
        
        if (needsUpdate) {
            // Write the updated file
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                for (const QString &line : lines) {
                    out << line << "\n";
                }
                file.close();
                qDebug() << "Updated room references in user file:" << userId;
            }
        }
    }
    
    if (!migratedFiles.isEmpty()) {
        qDebug() << "Migrated" << migratedFiles.size() << "room files:";
        for (const QString &info : migratedFiles) {
            qDebug() << "  " << info;
        }
    } else {
        qDebug() << "No room files needed migration.";
    }
    
    if (!problemFiles.isEmpty()) {
        qDebug() << "Problem files that couldn't be fixed automatically:";
        for (const QString &info : problemFiles) {
            qDebug() << "  " << info;
        }
    }
}
