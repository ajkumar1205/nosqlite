// helper.h
#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "user.h"
#include "db.h"
#include "table.h"

class QueryHelper
{
private:
    std::shared_ptr<Database> currentDatabase;

    // Helper function to split string by delimiter
    static std::vector<std::string> split(const std::string &str, char delim)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delim))
        {
            tokens.push_back(trim(token));
        }
        return tokens;
    }

    // Helper function to parse attribute list from string
    static std::vector<std::string> parseAttributeList(const std::string &str)
    {
        // Remove parentheses and split by comma
        std::string cleaned = str.substr(1, str.length() - 2);
        return split(cleaned, ',');
    }

public:
    std::shared_ptr<User> currentUser;
    QueryHelper() : currentUser(std::make_shared<User>()), currentDatabase(nullptr) {}

    // Helper function to trim whitespace
    static std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, last - first + 1);
    }

    // Execute query and return result message
    std::string executeQuery(const std::string &query)
    {
        std::string q = trim(query);
        if (q.empty())
            return "Empty query";

        // Remove semicolon if present
        if (q.back() == ';')
        {
            q = q.substr(0, q.length() - 1);
        }

        // Convert to lowercase for command matching
        std::string lowerQuery = q;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

        // Login command
        if (lowerQuery.substr(0, 5) == "login")
        {
            return handleLogin(q.substr(5));
        }

        // Check if user is logged in
        if (!currentUser)
        {
            return "Not logged in. Please login first.";
        }

        // Show databases command
        if (lowerQuery == "show")
        {
            return handleShow();
        }

        // Create database command
        if (lowerQuery.substr(0, 6) == "create")
        {
            if (lowerQuery.substr(7, 5) == "table" && currentDatabase)
            {
                return handleCreateTable(q.substr(13));
            }
            return handleCreateDatabase(q.substr(7));
        }

        // Open database command
        if (lowerQuery.substr(0, 4) == "open")
        {
            return handleOpenDatabase(q.substr(5));
        }

        // Drop command
        if (lowerQuery.substr(0, 4) == "drop")
        {
            if (currentDatabase)
            {
                return handleDropTable(q.substr(5));
            }
            return handleDropDatabase(q.substr(5));
        }

        // Commands that require an open database
        if (!currentDatabase)
        {
            return "No database opened. Use 'open <database>' first.";
        }

        // Insert command
        if (lowerQuery.substr(0, 11) == "insert into")
        {
            return handleInsert(q.substr(12));
        }

        // Delete command
        if (lowerQuery.substr(0, 11) == "delete from")
        {
            return handleDelete(q.substr(12));
        }

        // Select command
        if (lowerQuery.substr(0, 11) == "select from")
        {
            return handleSelect(q.substr(12));
        }

        return "Unknown command";
    }

private:
    std::string handleLogin(const std::string &params)
    {
        if (!params.empty())
        {
            return "Usage: login";
        }

        std::string username, password;
        std::cout << "Username: ";
        std::getline(std::cin, username);
        std::cout << "Password: ";
        std::getline(std::cin, password);

        if (currentUser->login(username, password))
        {
            return "Successfully logged in as " + username;
        }

        return "Login failed";
    }

    std::string handleShow()
    {
        std::stringstream result;
        result << "Available databases:\n";
        for (const auto &db : currentUser->getDatabases())
        {
            result << "- " << db.first << "\n";
        }
        return result.str();
    }

    std::string handleCreateDatabase(const std::string &dbName)
    {
        if (currentUser->createDatabase(trim(dbName)))
        {
            return "Database '" + dbName + "' created successfully";
        }
        return "Failed to create database";
    }

    std::string handleOpenDatabase(const std::string &dbName)
    {
        std::string name = trim(dbName);
        auto db = currentUser->getDatabase(name);
        if (db)
        {
            currentDatabase = db;
            std::stringstream result;
            result << "Opened database '" << name << "'\n";
            result << "Available tables:\n";
            for (const auto &table : db->getTables())
            {
                result << "- " << table->getName() << "\n";
            }
            return result.str();
        }
        return "Database not found or access denied";
    }

    std::string handleCreateTable(const std::string &params)
    {
        size_t parensStart = params.find('(');
        if (parensStart == std::string::npos)
        {
            return "Invalid syntax. Use: create table name (attr1, attr2, ...)";
        }

        // Extract table name - trim the name before the parenthesis
        std::string tableName = trim(params.substr(0, parensStart));
        std::string attrList = params.substr(parensStart);

        // Parse attribute list, excluding the table name from schema
        auto attributes = parseAttributeList(attrList);

        if (currentDatabase->createTable(tableName, attributes))
        {
            return "Table '" + tableName + "' created successfully";
        }
        return "Failed to create table";
    }

    std::string handleInsert(const std::string &params)
    {
        size_t parensStart = params.find('(');
        if (parensStart == std::string::npos)
        {
            return "Invalid syntax. Use: insert into table_name (value1, value2, ...)";
        }

        std::string tableName = trim(params.substr(0, parensStart));
        std::string valueList = params.substr(parensStart);
        auto values = parseAttributeList(valueList);

        auto table = currentDatabase->getTable(tableName);
        if (!table)
        {
            return "Table not found";
        }

        if (table->insertRow(values))
        {
            return "";
        }
        return "Failed to insert data";
    }

    std::string handleDelete(const std::string &params)
    {
        auto parts = split(params, ' ');
        if (parts.size() != 2 || parts[1].substr(0, 3) != "id:")
        {
            return "Invalid syntax. Use: delete from table_name id:value";
        }

        std::string tableName = parts[0];
        std::string id = parts[1].substr(3);

        auto table = currentDatabase->getTable(tableName);
        if (!table)
        {
            return "Table not found";
        }

        // Implementation for deletion goes here
        return "Record deleted successfully";
    }

    std::string handleSelect(const std::string &params)
    {
        auto parts = split(params, ' ');
        if (parts.empty())
        {
            return "Invalid syntax. Use: select from table_name [limit] [last]";
        }

        std::string tableName = parts[0];
        int limit = -1;
        bool last = false;

        if (parts.size() > 1)
        {
            try
            {
                limit = std::stoi(parts[1]);
                if (parts.size() > 2 && parts[2] == "last")
                {
                    last = true;
                }
            }
            catch (...)
            {
                if (parts[1] == "last")
                {
                    last = true;
                }
            }
        }

        auto table = currentDatabase->getTable(tableName);
        if (!table)
        {
            return "Table not found";
        }

        // Open the table's CSV file
        std::ifstream file(currentDatabase->getName() + "/" + tableName + ".csv");
        if (!file.is_open())
        {
            return "Failed to open table file";
        }

        // Read the header
        std::string headerLine;
        std::getline(file, headerLine);

        // Store all rows
        std::vector<std::string> rows;
        std::string row;
        while (std::getline(file, row))
        {
            rows.push_back(row);
        }

        // Process rows based on limit and last options
        std::stringstream result;
        result << headerLine << "\n";

        if (limit == -1)
        {
            // Return all rows
            for (const auto &r : rows)
            {
                result << r << "\n";
            }
        }
        else
        {
            if (last)
            {
                // Get the last 'limit' rows
                int startIndex = std::max(0, static_cast<int>(rows.size() - limit));
                for (int i = startIndex; i < rows.size(); ++i)
                {
                    result << rows[i] << "\n";
                }
            }
            else
            {
                // Get the first 'limit' rows
                for (int i = 0; i < std::min(limit, static_cast<int>(rows.size())); ++i)
                {
                    result << rows[i] << "\n";
                }
            }
        }

        return result.str();
    }

    std::string handleDropTable(const std::string &tableName)
    {
        // Trim the table name to remove any extra whitespace
        std::string name = trim(tableName);

        // Check if a database is currently open
        if (!currentDatabase)
        {
            return "No database opened. Open a database first.";
        }

        // Check if the table exists
        auto table = currentDatabase->getTable(name);
        if (!table)
        {
            return "Table '" + name + "' not found";
        }

        try
        {
            // Construct the full file path for the table
            std::string filePath = "database/" + currentUser->getName() + "/" +
                                   currentDatabase->getName() + "/" + name + ".csv";

            // Remove the table file
            if (std::filesystem::exists(filePath))
            {
                std::filesystem::remove(filePath);
            }
            else
            {
                return "Table file not found";
            }

            // Remove the table from the database's tables vector
            auto &tables = const_cast<std::vector<std::shared_ptr<Table>> &>(currentDatabase->getTables());
            tables.erase(
                std::remove_if(tables.begin(), tables.end(),
                               [&name](const std::shared_ptr<Table> &t)
                               {
                                   return t->getName() == name;
                               }),
                tables.end());

            return "Table '" + name + "' dropped successfully";
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
            return "Failed to drop table. Error: " + std::string(e.what());
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error dropping table: " << e.what() << std::endl;
            return "Failed to drop table";
        }
    }

    std::string handleDropDatabase(const std::string &dbName)
    {
        // Trim the database name to remove any extra whitespace
        std::string name = trim(dbName);

        // Check if the user has this database
        if (!currentUser->hasDatabaseAccess(name))
        {
            return "Database '" + name + "' not found or access denied";
        }

        try
        {
            // Construct the full database directory path
            std::string dbPath = "database/" + currentUser->getName() + "/" + name;

            // Remove the entire database directory and its contents
            if (std::filesystem::exists(dbPath))
            {
                std::filesystem::remove_all(dbPath);
            }
            else
            {
                return "Database directory not found";
            }

            // Remove the database from the user's databases
            // auto &databases = const_cast<std::unordered_map<std::string, std::shared_ptr<Database>> &>(
            //     currentUser->getDatabases());
            // databases.erase(name);

            // Update user's database list in file
            // if (!currentUser->updateUserDatabases())
            // {
            //     std::cerr << "Failed to update user databases file" << std::endl;
            // }

            // If the current database was the dropped one, reset it
            if (currentDatabase && currentDatabase->getName() == name)
            {
                currentDatabase = nullptr;
            }

            return "Failed to Drop";
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
            return "Failed to drop database. Error: " + std::string(e.what());
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error dropping database: " << e.what() << std::endl;
            return "Failed to drop database";
        }
    }
};

#endif // HELPER_H