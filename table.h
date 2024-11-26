#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <algorithm>

class Table
{
private:
    std::string name;
    std::vector<std::string> schema;
    std::string filePath;

    std::string generateUniqueId() const
    {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch())
                             .count();

        std::mt19937 gen(timestamp);
        std::uniform_int_distribution<> dis(0, 35);

        const std::string chars = "0123456789abcdefghijklmnopqrstuvwxyz";
        std::string uniqueId;
        for (int i = 0; i < 12; ++i)
        {
            uniqueId += chars[dis(gen)];
        }
        return uniqueId;
    }

public:
    Table(const std::string &tableName, const std::vector<std::string> &tableSchema,
          const std::string &basePath)
        : name(tableName), schema(tableSchema)
    {
        filePath = basePath + tableName + ".csv";
    }

    const std::string &getName() const { return name; }
    const std::vector<std::string> &getSchema() const { return schema; }

    bool initialize()
    {
        if (schema.empty())
            return false;

        try
        {
            // Ensure the directory exists
            std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

            std::ofstream file(filePath, std::ios::out);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file: " << filePath << std::endl;
                return false;
            }

            // Write schema header
            file << "unique_id";
            for (const auto &field : schema)
            {
                file << "," << field;
            }
            file << std::endl;

            file.close();

            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in initialize(): " << e.what() << std::endl;
            return false;
        }
    }

    bool load()
    {
        std::ifstream file(filePath);
        if (!file.is_open())
            return false;

        std::string header;
        std::getline(file, header);

        // Parse schema from header
        std::stringstream ss(header);
        std::string field;

        // Skip unique_id
        std::getline(ss, field, ',');

        schema.clear();
        while (std::getline(ss, field, ','))
        {
            schema.push_back(field);
        }

        return !schema.empty();
    }

    bool insertRow(const std::vector<std::string> &data)
    {
        if (data.size() != schema.size())
            return false;

        std::ofstream file(filePath, std::ios::app);
        if (!file.is_open())
            return false;

        std::string uniqueId = generateUniqueId();
        std::cout << "Generated unique ID: " << uniqueId << std::endl;
        file << uniqueId;
        for (const auto &field : data)
        {
            file << "," << field;
        }
        file << std::endl;
        return true;
    }
};

#endif