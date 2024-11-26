#ifndef DB_H
#define DB_H

#include <string>
#include <vector>
#include <memory>
#include "table.h"
#include <filesystem>
#include <fstream>
#include <iostream>

class Database
{
private:
    std::string name;
    std::string owner;
    std::vector<std::shared_ptr<Table>> tables;
    std::string basePath;

public:
    Database(const std::string &dbName, const std::string &ownerName)
        : name(dbName), owner(ownerName)
    {
        basePath = "database/" + owner + "/" + name + "/";
        loadExistingTables();
    }

    const std::string &getName() const { return name; }
    const std::string &getOwner() const { return owner; }
    const std::vector<std::shared_ptr<Table>> &getTables() const { return tables; }

    bool createTable(const std::string &tableName, const std::vector<std::string> &schema)
    {
        try
        {
            std::filesystem::create_directories(basePath);

            auto newTable = std::make_shared<Table>(tableName, schema, basePath);

            if (newTable->initialize())
            {
                tables.push_back(newTable);
                return true;
            }
            return false;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error creating table: " << e.what() << std::endl;
            return false;
        }
    }

    std::shared_ptr<Table> getTable(const std::string &tableName)
    {
        auto it = std::find_if(tables.begin(), tables.end(),
                               [&tableName](const auto &table)
                               {
                                   return table->getName() == tableName;
                               });
        return (it != tables.end()) ? *it : nullptr;
    }

private:
    void loadExistingTables()
    {
        if (!std::filesystem::exists(basePath))
            return;

        for (const auto &entry : std::filesystem::directory_iterator(basePath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".csv")
            {
                std::string tableName = entry.path().stem().string();
                auto table = std::make_shared<Table>(tableName, std::vector<std::string>{}, basePath);
                if (table->load())
                {
                    tables.push_back(table);
                }
            }
        }
    }
};

#endif