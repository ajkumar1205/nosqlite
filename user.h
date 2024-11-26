#ifndef USER_H
#define USER_H

#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include "db.h"

class User
{
private:
    std::string name;
    std::string password;
    std::unordered_map<std::string, std::shared_ptr<Database>> databases;

    // Static method to initialize system with admin user
    static void initializeAdminIfNeeded()
    {
        std::cout << "Initializing admin user and default database..." << std::endl;
        // Create account directory if it doesn't exist
        if (!std::filesystem::exists("account"))
        {
            std::cout << "Creating account directory..." << std::endl;
            std::filesystem::create_directory("account");
        }

        // Check if users.csv exists
        if (!std::filesystem::exists("account/users.csv"))
        {

            std::cout << "Creating admin user and default database..." << std::endl;
            // Create admin user
            std::ofstream usersFile("account/users.csv");
            usersFile << "admin" << std::endl;
            usersFile.close();

            // Create admin directory and file
            std::filesystem::create_directory("account/admin");
            std::ofstream adminFile("account/admin/admin.csv");
            adminFile << "admin" << std::endl;   // password
            adminFile << "default" << std::endl; // default database
            adminFile.close();
            std::cout << "Admin user created successfully." << std::endl;

            // Create default database directory
            std::cout << "Creating default database..." << std::endl;
            std::filesystem::create_directories("database/admin/default");
        }
    }

public:
    User()
    {
        // Initialize admin user and default database if needed
        initializeAdminIfNeeded();
    }

    static bool isFirstRun()
    {
        return !std::filesystem::exists("account/users.csv");
    }

    bool login(const std::string &inputName, const std::string &inputPassword)
    {
        // Initialize system if needed
        initializeAdminIfNeeded();

        // Verify user credentials from file
        if (!verifyCredentials(inputName, inputPassword))
            return false;

        std::cout << "User logged in: " << inputName << std::endl;

        name = inputName;
        password = inputPassword;
        loadDatabases();
        return true;
    }

    // Static method to create a new user
    static bool createUser(const std::string &userName, const std::string &userPassword)
    {
        // Initialize system if needed
        initializeAdminIfNeeded();

        // Check if user already exists
        std::ifstream usersFile("account/users.csv");
        std::string existingUser;
        while (std::getline(usersFile, existingUser))
        {
            if (existingUser == userName)
            {
                return false; // User already exists
            }
        }
        usersFile.close();

        // Create user directory
        std::filesystem::create_directory("account/" + userName);

        // Create user file
        std::ofstream userFile("account/" + userName + "/" + userName + ".csv");
        userFile << userPassword << std::endl;
        userFile << std::endl; // Empty line for databases
        userFile.close();

        // Add user to users.csv
        std::ofstream usersFileAppend("account/users.csv", std::ios::app);
        usersFileAppend << userName << std::endl;
        usersFileAppend.close();

        return true;
    }

    const std::string &getName() const { return name; }

    bool hasDatabaseAccess(const std::string &dbName) const
    {
        return databases.find(dbName) != databases.end();
    }

    std::shared_ptr<Database> getDatabase(const std::string &dbName)
    {
        auto it = databases.find(dbName);
        return (it != databases.end()) ? it->second : nullptr;
    }

    std::unordered_map<std::string, std::shared_ptr<Database>> getDatabases() const
    {
        return databases;
    }

    bool createDatabase(const std::string &dbName)
    {
        if (hasDatabaseAccess(dbName))
            return false;

        auto db = std::make_shared<Database>(dbName, name);
        databases[dbName] = db;

        // Update user's database list in file
        return updateUserDatabases();
    }

    // Method to get all users (admin only)
    static std::vector<std::string> getAllUsers()
    {
        std::vector<std::string> users;
        std::ifstream usersFile("account/users.csv");
        std::string user;
        while (std::getline(usersFile, user))
        {
            users.push_back(user);
        }
        return users;
    }

private:
    bool verifyCredentials(const std::string &inputName, const std::string &inputPassword)
    {
        std::ifstream userFile("account/" + inputName + "/" + inputName + ".csv");
        if (!userFile.is_open())
            return false;

        std::cout << "Verifying credentials for user: " << inputName << std::endl;

        std::string storedPassword;
        std::getline(userFile, storedPassword);
        return storedPassword == inputPassword;
    }

    void loadDatabases()
    {
        std::cout << "Loading user databases for: " << name << std::endl;

        std::string userFilePath = "account/" + name + "/" + name + ".csv";
        std::ifstream userFile(userFilePath);
        if (!userFile.is_open())
        {
            std::cout << "Failed to open user file: " << userFilePath << std::endl;
            return;
        }

        std::string password, dbLine;

        std::getline(userFile, password); // Skip password line
        std::cout << "Read password line: " << password << std::endl;

        std::getline(userFile, dbLine);
        std::cout << "Read database line: " << dbLine << std::endl;

        std::stringstream ss(dbLine);
        std::string dbName;

        while (std::getline(ss, dbName, ','))
        {
            dbName.erase(0, dbName.find_first_not_of(" "));
            dbName.erase(dbName.find_last_not_of(" ") + 1);

            if (!dbName.empty())
            {
                std::cout << "Adding database: " << dbName << std::endl;
                databases[dbName] = std::make_shared<Database>(dbName, name);
            }
        }

        // If admin user and no databases, create default database
        if (name == "admin" && databases.empty())
        {
            std::cout << "Creating default database for admin" << std::endl;
            try
            {
                createDatabase("default");
            }
            catch (const std::exception &e)
            {
                std::cout << "Error creating default database: " << e.what() << std::endl;
            }
        }
    }

    bool updateUserDatabases()
    {
        std::string userFilePath = "account/" + name + "/" + name + ".csv";
        std::ofstream userFile(userFilePath);
        if (!userFile.is_open())
            return false;

        userFile << password << std::endl;

        bool first = true;
        for (const auto &[dbName, db] : databases)
        {
            if (!first)
                userFile << ",";
            userFile << dbName;
            first = false;
        }
        userFile << std::endl;

        return true;
    }
};

#endif