#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

namespace fs = std::filesystem;

const int SECONDS_UNTIL_FILE_EXPIRATION = 60 * 60 * 24 * 30; // 30 days

enum ruleType {tempFolder, backupFolder};

struct rule
{
    std::string path;
    ruleType type;
    int param;
};

void checkForExpiredFiles(fs::path path, int maxAge)
{
    // Iterate through files/directories within the folder at path
    std::ranges::for_each( fs::directory_iterator{path},
            [](const auto& subPath) {
                // Check modification date
                std::chrono::duration<double> secondsSinceLastEdit = fs::file_time_type::clock::now() - fs::last_write_time(subPath);
                if (secondsSinceLastEdit.count() > SECONDS_UNTIL_FILE_EXPIRATION)
                {
                    std::cout << subPath << " : Expired\n";
                    fs::remove_all(subPath);
                }
            } );
    // todo implement
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
        std::string command{""};
        std::string param{""};
        std::string path{""};
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
        else if (command == "backupFolder")
        {
            newRule.type = backupFolder;
        }
        else
        {
            std::cout << "Not a valid command: \"" << command << "\"\n";
            return 0;
        }
        newRule.path = fs::path(path);
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
            case backupFolder:
                // Unimplemented todo implement
                break;
        }
    }

    return 0;
}
