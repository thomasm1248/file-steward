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
const int SECONDS_IN_A_DAY = 60 * 60 * 24;

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
    bool modified;
};

void checkForExpiredFiles(fs::path path, int maxAge)
{
    // Iterate through files/directories within the folder at path
    std::ranges::for_each( fs::directory_iterator{path},
            [&maxAge](const auto& subPath) {
                // Get seconds since last edit
                std::chrono::duration<double> secondsSinceLastEdit = fs::file_time_type::clock::now() - fs::last_write_time(subPath);
                // Delete if last modified a while ago
                if (secondsSinceLastEdit.count() > SECONDS_IN_A_DAY * maxAge)
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
    std::string inventoryFilePathString = archiveRoot.string() + "/.steward";
    std::ifstream inventoryFile{inventoryFilePathString};
    if (inventoryFile.is_open())
    {
        // Read each line of the file
        std::string line;
        while (std::getline(inventoryFile, line))
        {
            // Ignore blank lines
            if (line == "") continue;

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
        }
        inventoryFile.close();
    }
    fs::remove(fs::path{inventoryFilePathString});

    // Scan archive and make package list
    // todo consider ignoring inventory file
    std::vector<inventoryEntry> newInventoryState;
    for (auto& subPath : fs::directory_iterator{archiveRoot})
    {
        using namespace std::chrono;
        // Get last modified date
        auto modTime = fs::last_write_time(subPath);
        // Convert date to string (uuuuuugh)
        auto sctp = time_point_cast<system_clock::duration>(modTime - file_clock::now() + system_clock::now());
        std::time_t tt = system_clock::to_time_t(sctp);
        std::string dateString = std::to_string(tt);
        // Create new inventory entry, and add to list
        inventoryEntry newEntry;
        newEntry.path = subPath.path().string();
        newEntry.date = dateString;
        newInventoryState.push_back(newEntry);
    }

    // Merge inventory data with package list
    for (auto &entry : newInventoryState)
    {
        // Find corresponding entry from inventory file
        bool entryFound = false;
        inventoryEntry previousEntry;
        for (auto oldEntry : oldInventoryState)
        {
            if (oldEntry.path == entry.path)
            {
                previousEntry = oldEntry;
                entryFound = true;
                break;
            }
        }

        // Decide whether or not current package has been modified
        if (entryFound)
        {
            // The current package is not new, so it only needs to be zipped
            // if the modification date has been changed
            if (previousEntry.date != entry.date)
                entry.modified = true;
            else
                entry.modified = false;
        }
        else
        {
            // The current package is new, so it needs to be zipped
            entry.modified = true;
        }
        //std::cout << (entry.modified ? "modified " : "unchanged") << " : " << entry.path << '\n';
    }

    // Zip packages with recent modifications
    for (auto &entry : newInventoryState)
    {
        if (entry.modified)
        {
            std::cout << "Zipping " << entry.path << "\n";
            std::string strComm = "zip -r \"" + entry.path + ".zip\" \"" + entry.path + "\"";
            const char *command = strComm.c_str();
            system(command);
            std::cout << "Done.\n";
        }
    }

    // Generate new inventory file
    std::ofstream inventoryOutputFile{inventoryFilePathString};
    if (inventoryOutputFile.is_open())
    {
        for (auto &entry : newInventoryState)
        {
            inventoryOutputFile << entry.date << ' ' << entry.path << '\n';
        }
        inventoryOutputFile.close();
    }
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
