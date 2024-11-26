#include <iostream>
#include <string>
#include "helper.h"

void printHelp()
{
    std::cout << "\nAvailable commands:\n"
              << "  login                        - Login to the system\n"
              << "  show                          - Show available databases\n"
              << "  create <database_name>        - Create a new database\n"
              << "  open <database_name>          - Open an existing database\n"
              << "  create table <name> (attrs)   - Create a new table\n"
              << "  insert into <table> (values)  - Insert data into table\n"
              << "  select from <table> [limit] [last] - Query data\n"
              << "  delete from <table> id:<value> - Delete record\n"
              << "  drop <database/table_name>    - Drop database or table\n"
              << "  exit                          - Exit the program\n"
              << "  help                          - Show this help message\n";
}

int main()
{
    std::cout << "Simple Database Management System\n"
              << "Type 'help' for available commands\n";

    QueryHelper queryHelper;

    std::string input;

    // If first run, create admin account
    if (User::isFirstRun())
    {
        std::cout << "First run detected. Admin account created.\n"
                  << "Username: admin\n"
                  << "Password: admin\n";
    }

    while (true)
    {
        std::cout << "\n> ";
        std::getline(std::cin, input);

        // Trim input
        input = QueryHelper::trim(input);

        if (input.empty())
        {
            continue;
        }

        // Handle exit command
        if (input == "exit")
        {
            std::cout << "Goodbye!\n";
            break;
        }

        // Handle login command
        if (input == "login")
        {
            std::string username, password;
            std::cout << "Username: ";
            std::getline(std::cin, username);
            std::cout << "Password: ";
            std::getline(std::cin, password);

            // Create a new User instance for login attempt
            User loginUser;
            if (loginUser.login(username, password))
            {
                // Update the queryHelper's current user on successful login
                queryHelper.currentUser = std::make_shared<User>(loginUser);
                std::cout << "Successfully logged in as " << username << std::endl;
            }
            else
            {
                std::cout << "Login failed" << std::endl;
            }
            continue;
        }

        // Handle help command
        if (input == "help")
        {
            printHelp();
            continue;
        }

        // Execute query and print result
        try
        {
            std::string result = queryHelper.executeQuery(input);
            std::cout << result << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }

    return 0;
}