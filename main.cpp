#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

namespace fs = std::filesystem;

// Globals
const int SECONDS_UNTIL_FILE_EXPIRATION = 60 * 60 * 24 * 30; // 30 days

// Types for managing rules
enum ruleType {tempFolder, archiveFolder};
struct rule
{
    std::string path;
    ruleType type;
    int param;
};

// Types for managing archive inventory
struct inventoryEntry
{
    std::string path;
    std::string date;
};

void checkForExpiredFiles(fs::path path, int maxAge)
{
    // Iterate through files/directories within the folder at path
    std::ranges::for_each( fs::directory_iterator{path},
            [](const auto& subPath) {
                // Get seconds since last edit
                std::chrono::duration<double> secondsSinceLastEdit = fs::file_time_type::clock::now() - fs::last_write_time(subPath);
                // Delete if last modified a while ago
                if (secondsSinceLastEdit.count() > SECONDS_UNTIL_FILE_EXPIRATION)
                {
                    std::cout << subPath << " : Expired\n";
                    fs::remove_all(subPath);
                }
            } );
}

void zipModifiedSubfolders(fs::path archiveRoot)
{
    // read inventory file
    std::vector<inventoryEntry> oldInventoryState;
    std::ifstream inventoryFile{archiveRoot.string() + "/.steward"};
    if (inventoryFile.is_open())
    {
        // Read each line of the file
        std::string line;
        while (std::getline(inventoryFile, line))
        {
            // Parse the line
            std::string date;
            std::string path;
            enum {DATE, PATH} readingState = DATE;
            for (char &c : line)
            {
                switch(readingState)
                {
                    case DATE:
                        if (c != ' ')
                            date += c;
                        else
                            readingState = PATH;
                        break;
                    case PATH:
                        path += c;
                        break;
                }
            }

            // Create another inventory entry, and add to list
            inventoryEntry newEntry{};
            newEntry.path = path;
            newEntry.date = date;
            oldInventoryState.push_back(newEntry);
            std::cout << oldInventoryState.size() << '\n';
        }
    }

    // Scan archive and make package list
    std::vector<inventoryEntry> newInventoryState;
    std::ranges::for_each( fs::directory_iterator{archiveRoot},
            [&newInventoryState](const auto& subPath) {
                // Get last modified date
                fs::file_time_type modTime = fs::last_write_time(subPath);
                // Convert date to string (uuuuuugh)
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(modTime - std::chrono::file_clock::now() + std::chrono::system_clock::now());
                std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
                std::string dateString = std::to_string(tt);
                // Create new inventory entry, and add to list
                inventoryEntry newEntry;
                newEntry.path = subPath.path().string();
                newInventoryState.push_back(newEntry);
            } );
    std::cout << newInventoryState.size() << '\n';

    // Merge inventory data with package list

    // Zip packages with recent modifications

    // Generate new inventory file
}

int main(int argc, char** argv)
{
    // Try to get the rules file
    std::ifstream rulesFile{std::string{argv[1]}};

    // Read list of rules
    std::vector<std::string> rulesAsText;
    std::string ruleText;
    if (rulesFile.is_open())
        while (std::getline(rulesFile, ruleText))
            rulesAsText.push_back(ruleText);
    
    // Parse rules
    std::vector<rule> rules;
    for (int i = 0; i < rulesAsText.size(); i++)
    {
        // Read the rule character by character
        std::string command;
        std::string param;
        std::string path;
        enum {COMMAND, PARAM, PATH} readingState = COMMAND;
        for (char &c : rulesAsText.at(i))
        {
            switch (readingState)
            {
                case COMMAND:
                    if (c != ' ')
                    {
                        command += c;
                    }
                    else
                    {
                        // Only tempFolder command takes a parameter
                        if (command == "tempFolder")
                            readingState = PARAM;
                        else
                            readingState = PATH;
                    }
                    break;
                case PARAM:
                    if (c != ' ')
                        param += c;
                    else
                        readingState = PATH;
                    break;
                case PATH:
                    path += c;
                    break;
            }
        }

        // Create a new rule
        rule newRule{};
        if (command == "tempFolder")
        {
            newRule.type = tempFolder;
            newRule.param = std::stoi(param);
        }
        else if (command == "archiveFolder")
        {
            newRule.type = archiveFolder;
        }
        else
        {
            std::cout << "Not a valid command: \"" << command << "\"\n";
            return 0;
        }
        newRule.path = path;
        rules.push_back(newRule);
    }

    // Implement rules
    for (int i = 0; i < rules.size(); i++)
    {
        switch (rules.at(i).type)
        {
            case tempFolder:
                checkForExpiredFiles(rules.at(i).path, rules.at(i).param);
                break;
            case archiveFolder:
                zipModifiedSubfolders(rules.at(i).path);
                break;
        }
    }

    return 0;
}
