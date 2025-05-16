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

/*
 * Contributor: Ashraf
 * Function: server constructor
 * Description: Private constructor for the singleton server class.
 * Initializes the server and loads all data from persistent storage.
 */
server::server() : currentClient(nullptr) { loadAllData(); }

/*
 * Contributor: Ashraf
 * Function: getInstance
 * Description: Implementation of the singleton pattern for the server class.
 * Creates the instance if it doesn't exist and returns the single server instance.
 */
server *server::getInstance() {
    // Create the instance if it doesn't exist
    if (instance == nullptr) {
        instance = new server();
    }
    return instance;
}

/*
 * Contributor: Ashraf
 * Function: loadAllData
 * Description: Loads all application data from persistent storage.
 * Creates necessary directories, loads user accounts, settings, and chat history.
 * Migrates inconsistent room files to standardize the database.
 */
void server::loadAllData() {
    // Qobesy

    // Create necessary directories
    QDir dir;
    dir.mkpath("../db/users_credentials");
    dir.mkpath("../db/settings");
    dir.mkpath("../db/rooms");
    dir.mkpath("../db/users");
    dir.mkpath("../db/avatars");

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
                        } else if (line.startsWith("BIO:")) {
                            userMap[email].bio = line.mid(4).trimmed();
                        } else if (line.startsWith("ONLINE:")) {
                            QString onlineValue = line.mid(7).trimmed();
                            userMap[email].isOnline = (onlineValue == "true");
                        } else if (line.startsWith("LAST_CHANGE:")) {
                            QString lastChangeStr = line.mid(12).trimmed();
                            userMap[email].lastStatusChange =
                                QDateTime::fromString(lastChangeStr,
                                                      Qt::ISODate);
                            if (!userMap[email].lastStatusChange.isValid()) {
                                userMap[email].lastStatusChange =
                                    QDateTime::currentDateTime();
                            }
                        } else if (line.startsWith("AVATAR:")) {
                            userMap[email].avatarPath = line.mid(7).trimmed();
                        }
                    }

                    file.close();
                }
            }
        }
    }

    // Migrate any inconsistent room files
    migrateRoomFiles();

    // Load room messages
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);

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
        }
    }

    // Load user data (contacts, rooms, settings)
    QDir usersDir("../db/users");
    QStringList userFiles = usersDir.entryList(QDir::Files);

    for (const QString &userFile : userFiles) {
        QString userId = userFile;
        if (userId.endsWith(".txt")) {
            userId.chop(4); // Remove .txt extension
        }
        // qobesy

        loadUserContacts(userId);
    }
}

/*
 * Contributor: Ashraf
 * Function: saveAllData
 * Description: Persists all application data to storage.
 * Saves user accounts, settings, room messages, and contact information.
 * Creates necessary directories if they don't exist.
 */
void server::saveAllData() {

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
            out << i.key() << "," << i.value().username << ","
                << i.value().password << "\n";
        }
        credFile.close();
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

            // Save avatar path if available
            if (!userData.avatarPath.isEmpty()) {
                out << "AVATAR:" << userData.avatarPath << "\n";
            }

            settingsFile.close();
        } else {
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
        } else {
        }
    }

    // Save user data (contacts, rooms)
    QMapIterator<QString, QVector<QString>> userIt(userContacts);
    while (userIt.hasNext()) {
        userIt.next();

        QString userId = userIt.key();
        saveUserContacts(userId);
    }
}

void server::loadUsersAccounts() {
    QString filePath = "../db/users_credentials/credentials.txt";
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
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
            userData.bio = "";            // Default empty bio
            userData.isOnline = false;    // Default offline status

            userMap.insert(email, userData);
        }
    }
    file.close();
}

void server::loadUserContacts(const QString &userId) {
    // Load the user file
    QString filename = "../db/users/" + userId + ".txt";
    QFile file(filename);

    QVector<QString> contacts;
    QMap<QString, Room *> rooms;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);

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

                    // Create room and add it to the rooms map
                    Room *room = new Room(roomName);
                    room->setRoomId(roomId);
                    rooms[roomId] = room;
                }
            } else if (line.startsWith("NICKNAME:")) {
                QString nickname = line.mid(9).trimmed();
                if (userMap.contains(userId)) {
                    userMap[userId].nickname = nickname;
                }
            } else if (line.startsWith("BIO:")) {
                QString bio = line.mid(4).trimmed();
                if (userMap.contains(userId)) {
                    userMap[userId].bio = bio;
                }
            }
        }

        file.close();
    }

    userContacts[userId] = contacts;
    userRooms[userId] = rooms;

    // Also load blocked users if available
    QString blockedPath = "../db/users/" + userId + "_blocked.txt";
    QFile blockedFile(blockedPath);

    if (blockedFile.exists() &&
        blockedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QVector<QString> blocked;
        QTextStream in(&blockedFile);

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                blocked.append(line);
            }
        }

        blockedFile.close();
        blockedUsers[userId] = blocked;
    }
}

void server::saveUserContacts(const QString &userId) {
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
            const QMap<QString, Room *> &rooms = userRooms[userId];
            QMapIterator<QString, Room *> i(rooms);
            while (i.hasNext()) {
                i.next();
                out << "ROOM:" << i.key() << "|" << i.value()->getName()
                    << "\n";
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
    userData.username =
        email.split('@')[0]; // Default username is part before @
    userData.password = password;
    userData.nickname = userData.username;
    userData.bio = "";
    userData.isOnline = false;
    userData.lastStatusChange = QDateTime::currentDateTime();

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
    userData.lastStatusChange = QDateTime::currentDateTime();

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
        out << "BIO:Welcome to my profile!\n"; // Default bio
        out << "ONLINE:false\n";               // Default to offline
        settingsFile.close();
    }

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

/*
 * Contributor: Ashraf
 * Function: loginUser
 * Description: Authenticates a user and creates a client object.
 * Loads client data for the authenticated user and sets them as the current client.
 * Returns null if authentication fails.
 */
Client *server::loginUser(const QString &email, const QString &password) {
    if (!checkUser(email, password)) {
        return nullptr;
    }

    // Read current online status before doing anything else
    bool wasOnline = userMap[email].isOnline;

    // Create new client if doesn't exist
    if (!clients.contains(email)) {
        // IMPORTANT: Always use email as the userId to ensure consistency
        Client *newClient = new Client(email, userMap[email].username, email);
        clients[email] = newClient;
        loadClientData(newClient);
    }

    currentClient = clients[email];

    // Preserve the previous online status instead of forcing offline

    return currentClient;
}

/*
 * Contributor: Ashraf
 * Function: logoutUser
 * Description: Logs out the current user.
 * Saves the client data before logging out to ensure persistence.
 * Clears the current client reference.
 */
void server::logoutUser() {
    if (currentClient) {
        // Get the user ID before we reset currentClient
        QString userId = currentClient->getUserId();
        bool currentStatus = false;

        // Remember the current online status
        if (userMap.contains(userId)) {
            currentStatus = userMap[userId].isOnline;
        }

        // Save client data
        saveClientData(currentClient);

        // Clear currentClient without changing online status
        currentClient = nullptr;
    }
}

void server::loadClientData(Client *client) {
    if (!client)
        return;

    QString userId = client->getUserId();

    // Load client data from our in-memory structures
    if (userContacts.contains(userId)) {
        // Add contacts
        const QVector<QString> &contacts = userContacts[userId];
        for (const QString &contactId : contacts) {
            client->addContact(contactId);
        }
    }

    // Load rooms
    if (userRooms.contains(userId)) {
        const QMap<QString, Room *> &rooms = userRooms[userId];
        QMapIterator<QString, Room *> i(rooms);
        while (i.hasNext()) {
            i.next();
            Room *room = i.value();

            // Add room to client
            client->addRoom(room);

            // Add messages to room
            if (roomMessages.contains(room->getRoomId())) {
                const QVector<Message> &messages =
                    roomMessages[room->getRoomId()];
                room->setMessages(messages);
            }
        }
    }
}

void server::saveClientData(Client *client) {
    if (!client)
        return;

    QString userId = client->getUserId();

    // Save contacts
    QVector<QString> contacts = client->getContacts();
    userContacts[userId] = contacts;

    // Save rooms
    QMap<QString, Room *> rooms;
    QVector<Room *> clientRooms = client->getAllRooms();

    for (Room *room : clientRooms) {
        rooms[room->getRoomId()] = room;

        // Save room messages
        const QVector<Message> &messages = room->getMessages();
        roomMessages[room->getRoomId()] = messages;
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
        return false;
    }

    try {
        // IMPORTANT: First pre-emptively clear all room files to prevent issues
        QDir roomsDir("../db/rooms");
        if (roomsDir.exists()) {
            QStringList roomFiles = roomsDir.entryList(QDir::Files);
            for (const QString &roomFile : roomFiles) {
                // Check if this room involves the user being deleted
                if (roomFile.contains(userId) && roomFile.endsWith(".txt")) {
                    QString roomPath = "../db/rooms/" + roomFile;
                    QFile file(roomPath);
                    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        // Write an empty file
                        file.close();
                    }
                }
            }
        }

        // Remove from clients map and delete the client object
        if (clients.contains(userId)) {
            Client *clientToDelete = clients[userId];

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

        // Safely remove rooms associated with this user
        if (userRooms.contains(userId)) {
            QMap<QString, Room *> &rooms = userRooms[userId];

            // Create a list of room pointers and room IDs to handle separately
            QList<QPair<QString, Room *>> roomsToDelete;
            QMapIterator<QString, Room *> roomIter(rooms);
            while (roomIter.hasNext()) {
                roomIter.next();
                roomsToDelete.append(
                    qMakePair(roomIter.key(), roomIter.value()));
            }

            // Clear the rooms map first to prevent any further access
            userRooms[userId].clear();

            // Now safely delete each room
            for (const QPair<QString, Room *> &roomPair : roomsToDelete) {
                QString roomId = roomPair.first;
                Room *room = roomPair.second;

                // Remove messages from our cache
                roomMessages.remove(roomId);

                // Safely delete the Room object (if not null)
                if (room) {
                    try {
                        // First clear the messages (which should be safer now)
                        room->clearMessages();

                        // Then delete the room
                        delete room;
                    } catch (const std::exception &e) {
                    }
                }
            }

            // Remove the user's rooms map
            userRooms.remove(userId);
        }

        // Also remove from blockedUsers
        if (blockedUsers.contains(userId)) {
            blockedUsers.remove(userId);
        }

        return true;
    } catch (const std::exception &e) {
        return false;
    }
}

bool server::updateUserSettings(const QString &userId, const QString &nickname,
                                const QString &bio) {
    if (!userMap.contains(userId)) {
        return false;
    }

    // Save the current avatar path before updating other settings
    QString avatarPath = userMap[userId].avatarPath;

    userMap[userId].nickname = nickname;
    userMap[userId].bio = bio;

    // Save settings immediately to ensure persistence
    QString settingsPath = "../db/settings/" + userId + "_settings.txt";
    QFile settingsFile(settingsPath);

    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << "NICKNAME:" << nickname << "\n";
        out << "BIO:" << bio << "\n";
        out << "ONLINE:" << (userMap[userId].isOnline ? "true" : "false")
            << "\n";

        // Preserve the avatar path in the settings file
        if (!avatarPath.isEmpty()) {
            out << "AVATAR:" << avatarPath << "\n";
        }

        settingsFile.close();
    }

    return true;
}

bool server::getUserSettings(const QString &userId, QString &nickname,
                             QString &bio) {
    // Call the new version with a dummy avatar path
    QString dummyPath;
    return getUserSettings(userId, nickname, bio, dummyPath);
}

bool server::getUserSettings(const QString &userId, QString &nickname,
                             QString &bio, QString &avatarPath) {
    if (!userMap.contains(userId)) {
        return false;
    }

    nickname = userMap[userId].nickname;
    bio = userMap[userId].bio;
    avatarPath = userMap[userId].avatarPath;

    return true;
}

bool server::setUserOnlineStatus(const QString &userId, bool isOnline) {
    if (!userMap.contains(userId)) {
        return false;
    }

    // Save the current avatar path before updating status
    QString avatarPath = userMap[userId].avatarPath;

    // Only update if status actually changes
    if (userMap[userId].isOnline != isOnline) {
        userMap[userId].isOnline = isOnline;
        userMap[userId].lastStatusChange = QDateTime::currentDateTime();
    }

    // Immediately save the settings to ensure persistence
    QString settingsPath = "../db/settings/" + userId + "_settings.txt";
    QFile settingsFile(settingsPath);

    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << "NICKNAME:" << userMap[userId].nickname << "\n";
        out << "BIO:" << userMap[userId].bio << "\n";
        out << "ONLINE:" << (isOnline ? "true" : "false") << "\n";
        out << "LAST_CHANGE:"
            << userMap[userId].lastStatusChange.toString(Qt::ISODate) << "\n";

        // Preserve the avatar path in the settings file
        if (!avatarPath.isEmpty()) {
            out << "AVATAR:" << avatarPath << "\n";
        }

        settingsFile.close();
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

    // Create rooms directory if it doesn't exist
    QDir dir;
    dir.mkpath("../db/rooms");

    // Get all room files
    QDir roomsDir("../db/rooms");
    QStringList roomFiles = roomsDir.entryList(QDir::Files);

    QStringList migratedFiles;
    QStringList problemFiles;

    // First handle nickname vs email conflicts

    // Group room files by user pairs (ignoring formatting differences)
    QMap<QString, QStringList> userPairToRoomIds;

    for (const QString &filename : roomFiles) {
        if (!filename.endsWith(".txt"))
            continue;

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
        QString userPairKey =
            user1 < user2 ? user1 + "+" + user2 : user2 + "+" + user1;

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

            // Extract the canonical users from the key
            QStringList canonicalUsers = i.key().split("+");
            QString canonicalUser1 = canonicalUsers[0];
            QString canonicalUser2 = canonicalUsers[1];

            // Generate the correct room ID
            QString correctRoomId =
                Room::generateRoomId(canonicalUser1, canonicalUser2);
            QString correctPath = "../db/rooms/" + correctRoomId + ".txt";

            // Create the correct file if it doesn't exist
            if (!QFile::exists(correctPath)) {
                QFile::copy("../db/rooms/" + i.value().first() + ".txt",
                            correctPath);
            }

            // Merge all files into the canonical one
            for (const QString &roomId : i.value()) {
                if (roomId == correctRoomId)
                    continue; // Skip if it's the canonical one

                QString roomPath = "../db/rooms/" + roomId + ".txt";

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
        if (!userFile.endsWith(".txt"))
            continue;

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

                        QString correctRoomId =
                            Room::generateRoomId(user1, user2);
                        QString correctRoomName = user1 < user2
                                                      ? user1 + "_" + user2
                                                      : user2 + "_" + user1;

                        if (roomId != correctRoomId ||
                            roomName != correctRoomName) {
                            line =
                                "ROOM:" + correctRoomId + "|" + correctRoomName;
                            needsUpdate = true;
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
            }
        }
    }

    if (!migratedFiles.isEmpty()) {
        for (const QString &info : migratedFiles) {
        }
    } else {
    }

    if (!problemFiles.isEmpty()) {
        for (const QString &info : problemFiles) {
        }
    }
}

// Re-add saveUsersAccounts method for backward compatibility
void server::saveUsersAccounts() {
    QString filePath = "../db/users_credentials/credentials.txt";
    QFile file(filePath);

    // Create the directory if it doesn't exist
    QDir dir("../db/users_credentials");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    QMapIterator<QString, UserData> i(userMap);
    while (i.hasNext()) {
        i.next();
        out << i.key() << "," << i.value().username << "," << i.value().password
            << "\n";
    }

    file.close();
}

void server::createDefaultSettingsFiles() {

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
                QString nickname = userData.nickname.isEmpty()
                                       ? userData.username
                                       : userData.nickname;

                out << "NICKNAME:" << nickname << "\n";
                out << "BIO:Welcome to my profile!\n"; // Default bio
                out << "ONLINE:false\n";               // Default to offline

                settingsFile.close();
                filesCreated++;
            } else {
            }
        }
    }
}

/*
 * Contributor: Ashraf
 * Function: updateRoomMessages
 * Description: Updates the messages for a specific room in the server.
 * Stores messages in both the Qt collection and the STL-based map for compatibility.
 * Also adds messages to the global history deque for pagination.
 */
void server::updateRoomMessages(const QString &roomId,
                                const QVector<Message> &messages) {
    roomMessages[roomId] = messages;
    
    // Also update the STL version
    std::string roomIdStr = roomId.toStdString();
    std::vector<Message> messagesVec(messages.begin(), messages.end());
    roomMessagesSTL[roomIdStr] = messagesVec;
    
    // Add messages to the global history deque
    for (const Message &msg : messages) {
        addToMessageHistory(msg);
    }
}

/*
 * Contributor: Ashraf
 * Function: updateRoomMessagesSTL
 * Description: STL-based version of updateRoomMessages using std::map.
 * Required for the college project to demonstrate STL containers.
 * Keeps both Qt and STL versions synchronized.
 */
void server::updateRoomMessagesSTL(const std::string &roomId, const std::vector<Message> &messages) {
    // Store in the STL map
    roomMessagesSTL[roomId] = messages;
    
    // Also update the QMap version for compatibility
    QString qRoomId = QString::fromStdString(roomId);
    QVector<Message> qMessages(messages.begin(), messages.end());
    roomMessages[qRoomId] = qMessages;
}

/*
 * Contributor: Ashraf
 * Function: getRoomMessages
 * Description: Retrieves messages for a specific room.
 * Returns an empty vector if the room doesn't exist or has no messages.
 */
QVector<Message> server::getRoomMessages(const QString &roomId) const {
    if (roomMessages.contains(roomId)) {
        return roomMessages[roomId];
    }
    return QVector<Message>();
}

bool server::addContactForUser(const QString &userId,
                               const QString &contactId) {
    if (!userMap.contains(userId)) {
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
        return true;
    }

    return false; // Contact already existed
}

bool server::hasContactForUser(const QString &userId,
                               const QString &contactId) {
    if (!userContacts.contains(userId)) {
        return false;
    }

    return userContacts[userId].contains(contactId);
}

bool server::addRoomToUser(const QString &userId, Room *room) {
    if (!userMap.contains(userId) || !room) {
        return false;
    }

    // Create rooms map if it doesn't exist
    if (!userRooms.contains(userId)) {
        userRooms[userId] = QMap<QString, Room *>();
    }

    // Add the room if it's not already there
    QString roomId = room->getRoomId();
    if (!userRooms[userId].contains(roomId)) {
        userRooms[userId][roomId] = room;

        // Also ensure the messages are in the server's message store
        roomMessages[roomId] = room->getMessages();

        // Save to disk immediately
        saveUserContacts(userId);
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

bool server::updateUserAvatar(const QString &userId,
                              const QString &avatarPath) {
    if (!userMap.contains(userId)) {
        return false;
    }

    userMap[userId].avatarPath = avatarPath;

    // Save settings immediately to ensure persistence
    QString settingsPath = "../db/settings/" + userId + "_settings.txt";
    QFile settingsFile(settingsPath);

    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << "NICKNAME:" << userMap[userId].nickname << "\n";
        out << "BIO:" << userMap[userId].bio << "\n";
        out << "ONLINE:" << (userMap[userId].isOnline ? "true" : "false")
            << "\n";
        out << "AVATAR:" << avatarPath << "\n"; // Add avatar path to settings

        settingsFile.close();
    }

    return true;
}

QString server::getUserAvatar(const QString &userId) const {
    if (!userMap.contains(userId)) {
        return QString();
    }

    return userMap[userId].avatarPath;
}

bool server::changePassword(const QString &userId,
                            const QString &currentPassword,
                            const QString &newPassword,
                            const QString &confirmPassword,
                            QString &errorMessage) {
    // Check if user exists
    if (!userMap.contains(userId)) {
        errorMessage = "User not found. Please try again.";
        return false;
    }

    // Verify current password
    if (userMap[userId].password != currentPassword) {
        errorMessage = "Current password is incorrect. Please try again.";
        return false;
    }

    // Validate new password
    if (!isValidPassword(newPassword)) {
        errorMessage = "New password must be at least 4 characters long";
        return false;
    }

    // Check if passwords match
    if (newPassword != confirmPassword) {
        errorMessage = "New passwords do not match. Please make sure both "
                       "passwords are identical.";
        return false;
    }

    // Update the password in memory
    userMap[userId].password = newPassword;

    // Save to credentials file immediately
    saveUsersAccounts();

    return true;
}

// Story-related methods
void server::loadStories() {

    // Create stories directory if it doesn't exist
    QDir dir;
    dir.mkpath("../db/stories");

    // Get all story metadata files
    QDir storiesDir("../db/stories");
    QStringList filters;
    filters << "*.meta";
    storiesDir.setNameFilters(filters);
    QStringList storyFiles = storiesDir.entryList(QDir::Files);

    // Clear current stories
    stories.clear();

    // Load each story
    for (const QString &storyFile : storyFiles) {
        QString storyId = storyFile;
        storyId.chop(5); // Remove ".meta" extension

        QFile file(storiesDir.filePath(storyFile));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);

            StoryData story;
            story.id = storyId;

            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();

                if (line.startsWith("USERID:")) {
                    story.userId = line.mid(7).trimmed();
                } else if (line.startsWith("IMAGE:")) {
                    story.imagePath = line.mid(6).trimmed();
                } else if (line.startsWith("CAPTION:")) {
                    story.caption = line.mid(8).trimmed();
                } else if (line.startsWith("TIMESTAMP:")) {
                    QString timestampStr = line.mid(10).trimmed();
                    story.timestamp =
                        QDateTime::fromString(timestampStr, Qt::ISODate);
                    if (!story.timestamp.isValid()) {
                        story.timestamp = QDateTime::currentDateTime();
                    }
                } else if (line.startsWith("VIEWER:")) {
                    // Parse viewer ID and view time
                    QString viewerInfo = line.mid(7).trimmed();
                    QStringList parts = viewerInfo.split('|');
                    QString viewerId = parts.first().trimmed();
                    QDateTime viewTime;

                    if (parts.size() > 1) {
                        viewTime = QDateTime::fromString(parts.last().trimmed(),
                                                         Qt::ISODate);
                    }

                    if (!viewerId.isEmpty()) {
                        story.viewers.insert(viewerId);
                        // Also store the view time if valid
                        if (viewTime.isValid()) {
                            story.viewTimes[viewerId] = viewTime;
                        } else {
                            story.viewTimes[viewerId] =
                                QDateTime::currentDateTime();
                        }
                    }
                }
            }

            file.close();

            // Only add valid stories that aren't expired
            if (!story.userId.isEmpty() && !story.imagePath.isEmpty() &&
                story.timestamp.isValid()) {

                // Check if story is expired (older than 1 hour, changed from 30
                // seconds)
                QDateTime now = QDateTime::currentDateTime();
                QDateTime expirationTime = story.timestamp.addSecs(
                    3600); // Changed from 30 to 3600 seconds (1 hour)

                if (now < expirationTime) {
                    stories.append(story);
                } else {
                    // Delete expired story files
                    QFile::remove(storiesDir.filePath(storyFile));
                    QFile::remove(story.imagePath);
                }
            }
        }
    }
}

void server::saveStories() {

    // Create stories directory if it doesn't exist
    QDir dir;
    dir.mkpath("../db/stories");

    // Clean up old stories first
    cleanupOldStories();

    // Save each story
    for (const StoryData &story : stories) {
        QString metaPath = "../db/stories/" + story.id + ".meta";
        QFile file(metaPath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);

            out << "USERID:" << story.userId << "\n";
            out << "IMAGE:" << story.imagePath << "\n";
            out << "CAPTION:" << story.caption << "\n";
            out << "TIMESTAMP:" << story.timestamp.toString(Qt::ISODate)
                << "\n";

            // Save viewers with view time
            QSetIterator<QString> i(story.viewers);
            while (i.hasNext()) {
                QString viewerId = i.next();
                QDateTime viewTime = story.viewTimes.value(viewerId);
                if (viewTime.isValid()) {
                    out << "VIEWER:" << viewerId << "|"
                        << viewTime.toString(Qt::ISODate) << "\n";
                } else {
                    out << "VIEWER:" << viewerId << "\n";
                }
            }

            file.close();
        } else {
        }
    }
}

QString server::addStory(const QString &userId, const QString &imagePath,
                         const QString &caption) {
    if (!userMap.contains(userId)) {
        return QString();
    }

    // Create a unique ID for the story
    QString storyId =
        userId + "_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

    // Create a new story
    StoryData story;
    story.id = storyId;
    story.userId = userId;
    story.imagePath = imagePath;
    story.caption = caption;
    story.timestamp = QDateTime::currentDateTime();

    // Add to the stories list
    stories.append(story);

    // Save to disk
    saveStories();

    return storyId;
}

bool server::deleteStory(const QString &storyId) {
    for (int i = 0; i < stories.size(); ++i) {
        if (stories[i].id == storyId) {
            // Get the image path before removing
            QString imagePath = stories[i].imagePath;

            // Remove from the list
            stories.remove(i);

            // Delete the meta file
            QString metaPath = "../db/stories/" + storyId + ".meta";
            QFile::remove(metaPath);

            // Delete the image file
            QFile::remove(imagePath);

            // Save changes
            saveStories();
            return true;
        }
    }

    return false;
}

bool server::markStoryAsViewed(const QString &storyId,
                               const QString &viewerId) {
    for (int i = 0; i < stories.size(); ++i) {
        if (stories[i].id == storyId) {
            // Add viewer to the set
            stories[i].viewers.insert(viewerId);

            // Record the view time
            stories[i].viewTimes[viewerId] = QDateTime::currentDateTime();

            // Save changes
            saveStories();

            return true;
        }
    }

    return false;
}

QVector<StoryData> server::getStoriesForUser(const QString &userId) const {
    QVector<StoryData> userStories;

    for (const StoryData &story : stories) {
        if (story.userId == userId) {
            userStories.append(story);
        }
    }

    return userStories;
}

StoryData server::getStory(const QString &storyId) const {
    for (const StoryData &story : stories) {
        if (story.id == storyId) {
            return story;
        }
    }

    return StoryData(); // Return empty story if not found
}

bool server::hasViewedStory(const QString &storyId,
                            const QString &userId) const {
    for (const StoryData &story : stories) {
        if (story.id == storyId) {
            return story.viewers.contains(userId);
        }
    }

    return false; // Story not found
}

void server::cleanupOldStories() {
    QDateTime now = QDateTime::currentDateTime();
    QVector<StoryData> validStories;
    QSet<QString> storiesToRemove; // Track stories that need to be deleted

    // First identify which stories to keep and which to remove
    for (const StoryData &story : stories) {
        // Check if story is expired (older than 1 hour, changed from 30
        // seconds)
        QDateTime expirationTime = story.timestamp.addSecs(
            3600); // Changed from 30 to 3600 seconds (1 hour)

        if (now < expirationTime) {
            // Story is still valid
            validStories.append(story);
        } else {
            // Mark for removal but don't delete files yet
            storiesToRemove.insert(story.id);
        }
    }

    // Update the stories list first (important for thread safety)
    stories = validStories;

    // Now that the in-memory list is updated, we can safely delete the files
    for (const QString &storyId : storiesToRemove) {
        // Find the image path in case it's needed for deletion
        QString imagePath;
        for (const StoryData &story : validStories) {
            if (story.id == storyId) {
                imagePath = story.imagePath;
                break;
            }
        }

        // If we didn't find the image path in valid stories, it might be
        // available in the meta file, so try to read it
        if (imagePath.isEmpty()) {
            QString metaPath = "../db/stories/" + storyId + ".meta";
            QFile metaFile(metaPath);
            if (metaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&metaFile);
                while (!in.atEnd()) {
                    QString line = in.readLine();
                    if (line.startsWith("IMAGE:")) {
                        imagePath = line.mid(6).trimmed();
                        break;
                    }
                }
                metaFile.close();
            }
        }

        // Now try to delete files safely
        QString metaPath = "../db/stories/" + storyId + ".meta";
        bool metaDeleted = QFile::remove(metaPath);

        if (!imagePath.isEmpty()) {
            bool imageDeleted = QFile::remove(imagePath);
        } else {
        }
    }
}

int server::getStoryViewerCount(const QString &storyId) const {
    for (const StoryData &story : stories) {
        if (story.id == storyId) {
            return story.viewers.size();
        }
    }
    return 0;
}

QSet<QString> server::getStoryViewers(const QString &storyId) const {
    for (const StoryData &story : stories) {
        if (story.id == storyId) {
            return story.viewers;
        }
    }
    return QSet<QString>();
}

QDateTime server::getStoryViewTime(const QString &storyId,
                                   const QString &viewerId) const {
    for (const StoryData &story : stories) {
        if (story.id == storyId) {
            return story.viewTimes.value(viewerId);
        }
    }
    return QDateTime(); // Return invalid datetime if not found
}

bool server::blockUserForClient(const QString &clientId,
                                const QString &userToBlock) {
    // Make sure client exists
    if (!userMap.contains(clientId)) {
        return false;
    }

    // Check if we need to create a new entry for blocked users
    if (!blockedUsers.contains(clientId)) {
        blockedUsers[clientId] = QVector<QString>();
    }

    // Check if already blocked
    if (blockedUsers[clientId].contains(userToBlock)) {
        return true; // Already blocked, so consider it a success
    }

    // Add to blocked list
    blockedUsers[clientId].append(userToBlock);

    // Save to file
    QString blockedPath = "../db/users/" + clientId + "_blocked.txt";
    QFile file(blockedPath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &user : blockedUsers[clientId]) {
            out << user << "\n";
        }
        file.close();
    } else {
        // Continue anyway as we've updated the in-memory structure
    }

    // Remove from contacts if present
    Client *client = getClient(clientId);
    if (client && client->hasContact(userToBlock)) {
        client->removeContact(userToBlock);
        saveUserContacts(clientId);
    }

    return true;
}

bool server::isUserBlocked(const QString &clientId,
                           const QString &userToCheck) const {
    if (!blockedUsers.contains(clientId)) {
        return false;
    }

    return blockedUsers[clientId].contains(userToCheck);
}

QVector<QString> server::getBlockedUsers(const QString &clientId) const {
    if (!blockedUsers.contains(clientId)) {
        return QVector<QString>();
    }

    return blockedUsers[clientId];
}

// Add the unblockUserForClient method implementation
bool server::unblockUserForClient(const QString &clientId,
                                  const QString &userToUnblock) {
    // Make sure client exists
    if (!userMap.contains(clientId)) {
        return false;
    }

    // Check if we have a blocked users list for this client
    if (!blockedUsers.contains(clientId)) {
        return true; // No blocked users list means nothing to unblock
    }

    // Check if user is blocked
    if (!blockedUsers[clientId].contains(userToUnblock)) {
        return true; // Not blocked, so consider it a success
    }

    // Remove from blocked list
    blockedUsers[clientId].removeAll(userToUnblock);

    // Save to file
    QString blockedPath = "../db/users/" + clientId + "_blocked.txt";
    QFile file(blockedPath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &user : blockedUsers[clientId]) {
            out << user << "\n";
        }
        file.close();
    } else {
        // Continue anyway as we've updated the in-memory structure
    }

    return true;
}

// STL-based method implementations (college requirement)

/*
 * Contributor: Ashraf
 * Function: getRoomMessagesSTL
 * Description: STL-based version of getRoomMessages using std::map.
 * Required for the college project to demonstrate STL container usage.
 * Returns messages for a room as a std::vector instead of QVector.
 */
std::vector<Message> server::getRoomMessagesSTL(const std::string &roomId) const {
    auto it = roomMessagesSTL.find(roomId);
    if (it != roomMessagesSTL.end()) {
        return it->second;
    }
    return std::vector<Message>();
}

/*
 * Contributor: Ashraf
 * Function: sendMessageWithQueue
 * Description: Uses std::queue to handle offline messaging.
 * Messages sent to offline users are stored in a queue for later delivery.
 * This is part of the college requirement to demonstrate STL containers.
 */
bool server::sendMessageWithQueue(const QString &senderId, const QString &recipientId, const QString &message) {
    // Check if recipient is online
    if (!isUserOnline(recipientId)) {
        // Queue message for later delivery
        PendingMessage pending;
        pending.sender = senderId.toStdString();
        pending.recipient = recipientId.toStdString();
        pending.content = message.toStdString();
        pending.timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
        
        pendingMessagesQueue[recipientId.toStdString()].push(pending);
        return true;
    }
    
    // If online, send immediately
    // Get the room for these users
    Client *senderClient = getClient(senderId);
    if (!senderClient) return false;
    
    Room *room = senderClient->getRoomWithUser(recipientId);
    if (!room) {
        // Create a new room if it doesn't exist
        // Create a room name using the two user IDs
        QString roomName = senderId < recipientId ? 
                          senderId + "_" + recipientId : 
                          recipientId + "_" + senderId;
                          
        room = new Room(roomName);
        room->setRoomId(Room::generateRoomId(senderId, recipientId));
        senderClient->addRoom(room);
        addRoomToUser(senderId, room);
        
        // Also add to recipient's rooms
        Client *recipientClient = getClient(recipientId);
        if (recipientClient) {
            recipientClient->addRoom(room);
            addRoomToUser(recipientId, room);
        }
    }
    
    // Add message to room
    Message msg(message, senderId);
    QVector<Message> messages = room->getMessages();
    messages.append(msg);
    room->setMessages(messages);
    
    // Update in server
    updateRoomMessages(room->getRoomId(), messages);
    
    return true;
}

/*
 * Contributor: Ashraf
 * Function: deliverPendingMessages
 * Description: Delivers queued messages to a user when they come online.
 * Processes the std::queue of pending messages for a user and adds them to appropriate rooms.
 * This is part of the college requirement to demonstrate STL containers.
 */
void server::deliverPendingMessages(const QString &userId) {
    std::string userIdStr = userId.toStdString();
    auto it = pendingMessagesQueue.find(userIdStr);
    
    if (it != pendingMessagesQueue.end()) {
        std::queue<PendingMessage>& queue = it->second;
        
        while (!queue.empty()) {
            PendingMessage msg = queue.front();
            queue.pop();
            
            // Convert back to QString and deliver
            QString sender = QString::fromStdString(msg.sender);
            QString content = QString::fromStdString(msg.content);
            QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(msg.timestamp);
            
            // Get the room for these users
            Client *recipientClient = getClient(userId);
            if (recipientClient) {
                Room *room = recipientClient->getRoomWithUser(sender);
                if (!room) {
                    // Create a new room if it doesn't exist
                    // Create a room name using the two user IDs
                    QString roomName = sender < userId ? 
                                      sender + "_" + userId : 
                                      userId + "_" + sender;
                                      
                    room = new Room(roomName);
                    room->setRoomId(Room::generateRoomId(sender, userId));
                    recipientClient->addRoom(room);
                    addRoomToUser(userId, room);
                    
                    // Also add to sender's rooms if they're online
                    Client *senderClient = getClient(sender);
                    if (senderClient) {
                        senderClient->addRoom(room);
                        addRoomToUser(sender, room);
                    }
                }
                
                // Add message to room with the original timestamp
                Message newMsg(content, sender);
                // Since there's no setTimestamp method, we need to create a new message
                // and append it directly to the messages vector
                QVector<Message> messages = room->getMessages();
                // Create a new message using fromString to set the timestamp
                QString messageStr = content + "|" + sender + "|" + timestamp.toString(Qt::ISODate) + "|0";
                Message timestampedMsg = Message::fromString(messageStr);
                messages.append(timestampedMsg);
                room->setMessages(messages);
                
                // Update in server
                updateRoomMessages(room->getRoomId(), messages);
            }
        }
    }
}

/*
 * Contributor: Ashraf
 * Function: addToMessageHistory
 * Description: Adds a message to the global history using std::deque.
 * Using a deque allows efficient addition/removal at both ends.
 * Maintains a maximum history size to prevent unbounded memory growth.
 * This is part of the college requirement to demonstrate STL containers.
 */
void server::addToMessageHistory(const Message &message) {
    // Add to the front for most recent messages
    messageHistory.push_front(message);
    
    // Keep history to a reasonable size (e.g., 1000 messages)
    const size_t MAX_HISTORY_SIZE = 1000;
    
    while (messageHistory.size() > MAX_HISTORY_SIZE) {
        messageHistory.pop_back(); // Remove oldest messages
    }
}

/*
 * Contributor: Ashraf
 * Function: getMessageHistoryPage
 * Description: Retrieves a page of messages from the global history.
 * Supports efficient pagination using std::deque random access.
 * This is part of the college requirement to demonstrate STL containers.
 */
std::vector<Message> server::getMessageHistoryPage(int pageNum, int pageSize) const {
    std::vector<Message> result;
    
    // Calculate start and end indices
    int startIdx = pageNum * pageSize;
    int endIdx = std::min(startIdx + pageSize, static_cast<int>(messageHistory.size()));
    
    // Ensure valid range
    if (startIdx >= 0 && startIdx < static_cast<int>(messageHistory.size())) {
        for (int i = startIdx; i < endIdx; i++) {
            result.push_back(messageHistory[i]);
        }
    }
    
    return result;
}
