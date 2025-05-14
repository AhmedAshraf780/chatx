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
<<<<<<< HEAD
    loadUsersAccounts();
    migrateRoomFiles(); // Migrate any inconsistent room files
=======
    loadAllData();
>>>>>>> bfd0fc2 (handle the settings)
}

server *server::getInstance() {
    // Create the instance if it doesn't exist
    if (instance == nullptr) {
        instance = new server();
    }
    return instance;
}

void server::loadAllData() {
    qDebug() << "Loading all data from files...";
    
    // Create necessary directories
    QDir dir;
    dir.mkpath("../db/users_credentials");
    dir.mkpath("../db/settings");
    dir.mkpath("../db/rooms");
    dir.mkpath("../db/users");
    
    // Load user accounts
    loadUsersAccounts();
    
    // Create default settings files for all users if they don't exist
    createDefaultSettingsFiles();
    
    // Load user settings
    QDir settingsDir("../db/settings");
    QStringList settingsFiles = settingsDir.entryList(QDir::Files);
    
    for (const QString &settingsFile : settingsFiles) {
        if (settingsFile.endsWith("_settings.txt")) {
            // Extract email from filename
            QString email = settingsFile;
            email.chop(13); // Remove "_settings.txt"
            
            if (userMap.contains(email)) {
                QString settingsPath = "../db/settings/" + settingsFile;
                QFile file(settingsPath);
                
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    
                    while (!in.atEnd()) {
                        QString line = in.readLine();
                        
                        if (line.startsWith("NICKNAME:")) {
                            userMap[email].nickname = line.mid(9).trimmed();
                        } 
                        else if (line.startsWith("BIO:")) {
                            userMap[email].bio = line.mid(4).trimmed();
                        }
                        else if (line.startsWith("ONLINE:")) {
                            QString onlineValue = line.mid(7).trimmed();
                            userMap[email].isOnline = (onlineValue == "true");
                        }
                    }
                    
                    file.close();
                    qDebug() << "Loaded settings for user:" << email;
                }
            }
        }
    }
    
    // Migrate any inconsistent room files
    migrateRoomFiles();
    
    // Load room messages
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);
    qDebug() << "Loading" << roomFiles.size() << "room files...";
    
    for (const QString &roomFile : roomFiles) {
        QString roomId = roomFile;
        if (roomId.endsWith(".txt")) {
            roomId.chop(4); // Remove .txt extension
        }
        
        // Open room file and load messages
        QFile file(roomsDir.filePath(roomFile));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QVector<Message> messages;
            
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty()) {
                    Message msg = Message::fromString(line);
                    messages.append(msg);
                }
            }
            
            file.close();
            roomMessages[roomId] = messages;
            qDebug() << "Loaded" << messages.size() << "messages for room" << roomId;
        }
    }
    
    // Load user data (contacts, rooms, settings)
    QDir usersDir("../db/users");
    QStringList userFiles = usersDir.entryList(QDir::Files);
    qDebug() << "Loading" << userFiles.size() << "user files...";
    
    for (const QString &userFile : userFiles) {
        QString userId = userFile;
        if (userId.endsWith(".txt")) {
            userId.chop(4); // Remove .txt extension
        }
        
        loadUserContacts(userId);
    }
    
    qDebug() << "All data loaded successfully.";
}

void server::saveAllData() {
    qDebug() << "Saving all data to files...";
    
    // Create necessary directories
    QDir dir;
    dir.mkpath("../db/users_credentials");
    dir.mkpath("../db/settings");
    dir.mkpath("../db/rooms");
    dir.mkpath("../db/users");
    
    // Save user accounts - save to users_credentials directory
    QString credentialsPath = "../db/users_credentials/credentials.txt";
    QFile credFile(credentialsPath);
    
    if (credFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&credFile);
        QMapIterator<QString, UserData> i(userMap);
        while (i.hasNext()) {
            i.next();
            out << i.key() << "," << i.value().username << "," << i.value().password << "\n";
            qDebug() << "Saved user credentials:" << i.key() << i.value().username;
        }
        credFile.close();
        qDebug() << "Total user credentials saved:" << userMap.size();
    } else {
        qDebug() << "Failed to save user credentials:" << credFile.errorString();
    }
    
    // Save user settings - each user gets their own settings file
    QMapIterator<QString, UserData> userIter(userMap);
    while (userIter.hasNext()) {
        userIter.next();
        
        QString email = userIter.key();
        const UserData &userData = userIter.value();
        
        QString settingsPath = "../db/settings/" + email + "_settings.txt";
        QFile settingsFile(settingsPath);
        
        if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&settingsFile);
            out << "NICKNAME:" << userData.nickname << "\n";
            out << "BIO:" << userData.bio << "\n";
            out << "ONLINE:" << (userData.isOnline ? "true" : "false") << "\n";
            
            settingsFile.close();
            qDebug() << "Saved settings for user:" << email;
        } else {
            qDebug() << "Failed to save settings for user:" << email;
        }
    }
    
    // Save room messages
    QMapIterator<QString, QVector<Message>> roomIt(roomMessages);
    while (roomIt.hasNext()) {
        roomIt.next();
        
        QString roomId = roomIt.key();
        const QVector<Message> &messages = roomIt.value();
        
        QFile file("../db/rooms/" + roomId + ".txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            
            for (const Message &msg : messages) {
                out << msg.toString() << "\n";
            }
            
            file.close();
            qDebug() << "Saved" << messages.size() << "messages for room" << roomId;
        } else {
            qDebug() << "Failed to save messages for room" << roomId;
        }
    }
    
    // Save user data (contacts, rooms)
    QMapIterator<QString, QVector<QString>> userIt(userContacts);
    while (userIt.hasNext()) {
        userIt.next();
        
        QString userId = userIt.key();
        saveUserContacts(userId);
    }
    
    qDebug() << "All data saved successfully.";
}

void server::loadUsersAccounts() {
    QString filePath = "../db/users_credentials/credentials.txt";
    QFile file(filePath);
    qDebug() << "Loading users from:" << filePath;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading:" << file.errorString();
        return;
    }

    userMap.clear();
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(',');
        if (parts.size() >= 3) {
            QString email = parts[0].trimmed();
            QString username = parts[1].trimmed();
            QString password = parts[2].trimmed();
            
            UserData userData;
            userData.username = username;
            userData.password = password;
            userData.nickname = username; // Default nickname is username
            userData.bio = ""; // Default empty bio
            userData.isOnline = false; // Default offline status
            
            userMap.insert(email, userData);
            qDebug() << "Loaded user:" << email << username;
        }
    }
    file.close();
    qDebug() << "Total users loaded:" << userMap.size();
}

void server::loadUserContacts(const QString& userId) {
    // Load the user file
    QString filename = "../db/users/" + userId + ".txt";
    QFile file(filename);
    
    QVector<QString> contacts;
    QMap<QString, Room*> rooms;
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            
            if (line.startsWith("CONTACT:")) {
                QString contactId = line.mid(8);
                contacts.append(contactId);
                qDebug() << "Loaded contact for" << userId << ":" << contactId;
            }
            else if (line.startsWith("ROOM:")) {
                QStringList parts = line.mid(5).split('|');
                if (parts.size() >= 2) {
                    QString roomId = parts[0];
                    QString roomName = parts[1];
                    
                    // Create room and add it to the rooms map
                    Room *room = new Room(roomName);
                    room->setRoomId(roomId);
                    rooms[roomId] = room;
                    
                    qDebug() << "Loaded room for" << userId << ":" << roomId << roomName;
                }
            }
            else if (line.startsWith("NICKNAME:")) {
                QString nickname = line.mid(9).trimmed();
                if (userMap.contains(userId)) {
                    userMap[userId].nickname = nickname;
                    qDebug() << "Loaded nickname for" << userId << ":" << nickname;
                }
            }
            else if (line.startsWith("BIO:")) {
                QString bio = line.mid(4).trimmed();
                if (userMap.contains(userId)) {
                    userMap[userId].bio = bio;
                    qDebug() << "Loaded bio for" << userId << ":" << bio;
                }
            }
        }
        
        file.close();
    }
    
    userContacts[userId] = contacts;
    userRooms[userId] = rooms;
}

void server::saveUserContacts(const QString& userId) {
    QString filename = "../db/users/" + userId + ".txt";
    QFile file(filename);
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        
        // Save contacts
        if (userContacts.contains(userId)) {
            const QVector<QString> &contacts = userContacts[userId];
            for (const QString &contactId : contacts) {
                out << "CONTACT:" << contactId << "\n";
            }
        }
        
        // Save rooms
        if (userRooms.contains(userId)) {
            const QMap<QString, Room*> &rooms = userRooms[userId];
            QMapIterator<QString, Room*> i(rooms);
            while (i.hasNext()) {
                i.next();
                out << "ROOM:" << i.key() << "|" << i.value()->getName() << "\n";
            }
        }
        
        // Save nickname and bio
        if (userMap.contains(userId)) {
            const UserData &userData = userMap[userId];
            if (!userData.nickname.isEmpty()) {
                out << "NICKNAME:" << userData.nickname << "\n";
            }
            if (!userData.bio.isEmpty()) {
                out << "BIO:" << userData.bio << "\n";
            }
        }
        
        file.close();
        qDebug() << "Saved user data for" << userId;
    } else {
        qDebug() << "Failed to save user data for" << userId << ":" << file.errorString();
    }
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
    userData.username = email.split('@')[0]; // Default username is part before @
    userData.password = password;
    userData.nickname = userData.username;
    userData.bio = "";
    userData.isOnline = false;
    
    userMap.insert(email, userData);
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
    userData.nickname = username;
    userData.bio = "";
    userData.isOnline = false;
    
    // Add user to in-memory map
    userMap.insert(email, userData);
    
    // Save to credentials file immediately
    saveUsersAccounts();
    
    // Create default settings file for the new user
    QString settingsPath = "../db/settings/" + email + "_settings.txt";
    QFile settingsFile(settingsPath);
    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << "NICKNAME:" << username << "\n";
        out << "BIO:Welcome to my profile!\n";  // Default bio
        out << "ONLINE:false\n";  // Default to offline
        settingsFile.close();
        qDebug() << "Created settings file for new user:" << email;
    }
    
    qDebug() << "Registered new user:" << email << username;
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
    return true;
}

QVector<QPair<QString, QString>> server::getAllUsers() const {
    QVector<QPair<QString, QString>> users;
    QMapIterator<QString, UserData> i(userMap);
    while (i.hasNext()) {
        i.next();
        users.append(qMakePair(i.key(), i.value().nickname));
    }
    return users;
}

Client *server::loginUser(const QString &email, const QString &password) {
    if (!checkUser(email, password)) {
        return nullptr;
    }

    // Set user as offline by default
    userMap[email].isOnline = false;
    
    // Create new client if doesn't exist
    if (!clients.contains(email)) {
        // IMPORTANT: Always use email as the userId to ensure consistency
        Client *newClient = new Client(email, userMap[email].username, email);
        clients[email] = newClient;
<<<<<<< HEAD
        loadClientData(newClient);
        qDebug() << "Created new client with userId/email:" << email << "and username:" << userMap[email].username;
=======
        
        // Load client data from our in-memory structures
        if (userContacts.contains(email)) {
            // Add contacts
            const QVector<QString> &contacts = userContacts[email];
            for (const QString &contactId : contacts) {
                newClient->addContact(contactId);
            }
        }
        
        // Load rooms
        if (userRooms.contains(email)) {
            const QMap<QString, Room*> &rooms = userRooms[email];
            QMapIterator<QString, Room*> i(rooms);
            while (i.hasNext()) {
                i.next();
                Room *room = i.value();
                
                // Add room to client
                newClient->addRoom(room);
                
                // Add messages to room
                if (roomMessages.contains(room->getRoomId())) {
                    const QVector<Message> &messages = roomMessages[room->getRoomId()];
                    room->setMessages(messages);
                }
            }
        }
        
        qDebug() << "Created new client with userId/email:" << email 
                << "and username:" << userMap[email].username;
>>>>>>> bfd0fc2 (handle the settings)
    }

    currentClient = clients[email];
    return currentClient;
}

void server::logoutUser() {
    if (currentClient) {
        // Set user as offline
        QString userId = currentClient->getUserId();
        if (userMap.contains(userId)) {
            userMap[userId].isOnline = false;
        }
        
        // Save client data to our in-memory structures
        saveClientData(currentClient);
        currentClient = nullptr;
    }
}

void server::loadClientData(Client *client) {
<<<<<<< HEAD
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
=======
    if (!client) return;
    
    QString userId = client->getUserId();
    
    // Load client data from our in-memory structures
    if (userContacts.contains(userId)) {
        // Add contacts
        const QVector<QString> &contacts = userContacts[userId];
        for (const QString &contactId : contacts) {
            client->addContact(contactId);
>>>>>>> bfd0fc2 (handle the settings)
        }
        file.close();
    }
<<<<<<< HEAD

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
=======
    
    // Load rooms
    if (userRooms.contains(userId)) {
        const QMap<QString, Room*> &rooms = userRooms[userId];
        QMapIterator<QString, Room*> i(rooms);
        while (i.hasNext()) {
            i.next();
            Room *room = i.value();
            
            // Add room to client
            client->addRoom(room);
            
            // Add messages to room
            if (roomMessages.contains(room->getRoomId())) {
                const QVector<Message> &messages = roomMessages[room->getRoomId()];
                room->setMessages(messages);
            }
>>>>>>> bfd0fc2 (handle the settings)
        }
    }
}

void server::saveClientData(Client *client) {
<<<<<<< HEAD
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
=======
    if (!client) return;
    
    QString userId = client->getUserId();
    
    // Save contacts
    QVector<QString> contacts = client->getContacts();
    userContacts[userId] = contacts;
    
    // Save rooms
    QMap<QString, Room*> rooms;
    QVector<Room*> clientRooms = client->getAllRooms();
    
    for (Room *room : clientRooms) {
        rooms[room->getRoomId()] = room;
        
        // Save room messages
        const QVector<Message> &messages = room->getMessages();
        roomMessages[room->getRoomId()] = messages;
>>>>>>> bfd0fc2 (handle the settings)
    }
    
    userRooms[userId] = rooms;
}

void server::saveAllClientsData() {
    for (Client *client : clients) {
        saveClientData(client);
    }
}

bool server::deleteUser(const QString &userId) {
    // Check if the user exists in our userMap
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for deletion";
        return false;
    }
    
    qDebug() << "Deleting user account:" << userId;
    
    // Remove from clients map and delete the client object
    if (clients.contains(userId)) {
        Client* clientToDelete = clients[userId];
        
        // If this is the current client, set to null first
        if (currentClient == clientToDelete) {
            currentClient = nullptr;
        }
        
        // Delete the client object
        delete clientToDelete;
        clients.remove(userId);
    }
    
    // Remove all user data from in-memory structures
    userMap.remove(userId);
    userContacts.remove(userId);
    
    // Remove rooms associated with this user and their messages
    if (userRooms.contains(userId)) {
        QMap<QString, Room*> &rooms = userRooms[userId];
        QMapIterator<QString, Room*> i(rooms);
        while (i.hasNext()) {
            i.next();
            QString roomId = i.key();
            delete i.value(); // Delete Room object
            roomMessages.remove(roomId); // Remove messages
        }
        
        userRooms.remove(userId);
    }
    
    qDebug() << "User" << userId << "deleted successfully";
    return true;
}

bool server::updateUserSettings(const QString &userId, const QString &nickname, const QString &bio) {
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for updating settings";
        return false;
    }
    
    userMap[userId].nickname = nickname;
    userMap[userId].bio = bio;
    
    // Save settings immediately to ensure persistence
    QString settingsPath = "../db/settings/" + userId + "_settings.txt";
    QFile settingsFile(settingsPath);
    
    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << "NICKNAME:" << nickname << "\n";
        out << "BIO:" << bio << "\n";
        out << "ONLINE:" << (userMap[userId].isOnline ? "true" : "false") << "\n";
        
        settingsFile.close();
        qDebug() << "Saved settings for user:" << userId;
    }
    
    qDebug() << "Updated settings for user" << userId << "- Nickname:" << nickname << "Bio:" << bio;
    return true;
}

bool server::getUserSettings(const QString &userId, QString &nickname, QString &bio) {
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for getting settings";
        return false;
    }
    
    nickname = userMap[userId].nickname;
    bio = userMap[userId].bio;
    
    return true;
}

bool server::setUserOnlineStatus(const QString &userId, bool isOnline) {
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for updating online status";
        return false;
    }
    
    userMap[userId].isOnline = isOnline;
    qDebug() << "Set user" << userId << "online status to" << (isOnline ? "online" : "offline");
    
    // Immediately save the settings to ensure persistence
    QString settingsPath = "../db/settings/" + userId + "_settings.txt";
    QFile settingsFile(settingsPath);
    
    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << "NICKNAME:" << userMap[userId].nickname << "\n";
        out << "BIO:" << userMap[userId].bio << "\n";
        out << "ONLINE:" << (isOnline ? "true" : "false") << "\n";
        
        settingsFile.close();
        qDebug() << "Saved online status for user:" << userId;
    }
    
    return true;
}

bool server::isUserOnline(const QString &userId) const {
    if (!userMap.contains(userId)) {
        return false;
    }
    
    return userMap[userId].isOnline;
}

server::~server() {
    // Save all data before shutting down
    saveAllData();
    
    // Clean up clients
    for (Client *client : clients) {
        delete client;
    }
    clients.clear();
    currentClient = nullptr;
}

// Migrate inconsistent room files (unchanged)
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

// Re-add saveUsersAccounts method for backward compatibility
void server::saveUsersAccounts() {
    QString filePath = "../db/users_credentials/credentials.txt";
    QFile file(filePath);
    qDebug() << "Saving users to:" << filePath;

    // Create the directory if it doesn't exist
    QDir dir("../db/users_credentials");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

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

void server::createDefaultSettingsFiles() {
    qDebug() << "Creating default settings files for all users...";
    
    // Create settings directory if it doesn't exist
    QDir dir;
    if (!dir.exists("../db/settings")) {
        dir.mkpath("../db/settings");
    }
    
    QMapIterator<QString, UserData> i(userMap);
    int filesCreated = 0;
    
    while (i.hasNext()) {
        i.next();
        
        QString email = i.key();
        const UserData &userData = i.value();
        
        // Check if settings file already exists
        QString settingsPath = "../db/settings/" + email + "_settings.txt";
        QFile settingsFile(settingsPath);
        
        if (!settingsFile.exists()) {
            // Create default settings file
            if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&settingsFile);
                
                // Use username as default nickname if nickname is empty
                QString nickname = userData.nickname.isEmpty() ? userData.username : userData.nickname;
                
                out << "NICKNAME:" << nickname << "\n";
                out << "BIO:Welcome to my profile!\n";  // Default bio
                out << "ONLINE:false\n";  // Default to offline
                
                settingsFile.close();
                filesCreated++;
                qDebug() << "Created default settings file for user:" << email;
            } else {
                qDebug() << "Failed to create settings file for user:" << email << ":" << settingsFile.errorString();
            }
        }
    }
    
    qDebug() << "Created" << filesCreated << "default settings files.";
}

// Add the missing updateRoomMessages method
void server::updateRoomMessages(const QString &roomId, const QVector<Message> &messages) {
    roomMessages[roomId] = messages;
    qDebug() << "Updated in-memory messages for room:" << roomId << "with" << messages.size() << "messages";
}

// Add the missing getRoomMessages method
QVector<Message> server::getRoomMessages(const QString &roomId) const {
    if (roomMessages.contains(roomId)) {
        return roomMessages[roomId];
    }
    return QVector<Message>();
}

bool server::addContactForUser(const QString &userId, const QString &contactId) {
    if (!userMap.contains(userId)) {
        qDebug() << "User" << userId << "not found for adding contact";
        return false;
    }
    
    // Create contacts vector if it doesn't exist
    if (!userContacts.contains(userId)) {
        userContacts[userId] = QVector<QString>();
    }
    
    // Add the contact if it's not already there
    if (!userContacts[userId].contains(contactId)) {
        userContacts[userId].append(contactId);
        
        // Save to disk immediately
        saveUserContacts(userId);
        qDebug() << "Added contact" << contactId << "for user" << userId;
        return true;
    }
    
    return false; // Contact already existed
}

bool server::hasContactForUser(const QString &userId, const QString &contactId) {
    if (!userContacts.contains(userId)) {
        return false;
    }
    
    return userContacts[userId].contains(contactId);
}

bool server::addRoomToUser(const QString &userId, Room *room) {
    if (!userMap.contains(userId) || !room) {
        qDebug() << "User" << userId << "not found or invalid room for adding room";
        return false;
    }
    
    // Create rooms map if it doesn't exist
    if (!userRooms.contains(userId)) {
        userRooms[userId] = QMap<QString, Room*>();
    }
    
    // Add the room if it's not already there
    QString roomId = room->getRoomId();
    if (!userRooms[userId].contains(roomId)) {
        userRooms[userId][roomId] = room;
        
        // Also ensure the messages are in the server's message store
        roomMessages[roomId] = room->getMessages();
        
        // Save to disk immediately
        saveUserContacts(userId);
        qDebug() << "Added room" << roomId << "for user" << userId;
        return true;
    }
    
    return false; // Room already existed
}

bool server::hasRoomForUser(const QString &userId, const QString &roomId) {
    if (!userRooms.contains(userId)) {
        return false;
    }
    
    return userRooms[userId].contains(roomId);
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
