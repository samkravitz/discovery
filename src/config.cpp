/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: config.cpp
 * DATE: December 7th, 2020
 * DESCRIPTION: global configuration data for discovery
 */

#include <fstream>
#include <regex>
#include <vector>
#include <map>
#include "config.h" 
#include "Discovery.h"

namespace config 
{
    std::string rom_name = "";
    std::string backup_path = "";
    std::string bios_name = "gba_bios.bin";
    bool show_help = false;
    bool debug = false;
    std::string config_file = "discovery.config";
}

void config::read_config_file()
{
    std::ifstream file;
    std::map<std::string, std::string> config;
    file.open(config_file, std::ios::binary);
    // read lines in config file
    if(file.is_open())
    {
        std::string line;
        std::regex reg("^\\s*([^\\W][\\w]*)\\s*=\\s*(<?[^\\W\\s][\\w]*\\S*>?)|(#{1,})+2.*$");
        std::smatch group;

        while(std::getline(file, line)) 
        {
            if(std::regex_search(line, group, reg))  
            {
                std::cout<<"matches: "<<line<<"\n";
                if(group.size()) 
                    config.insert({group[1].str(), group[2].str()});
                else
                    log(LogLevel::Error, "Error parsing config file.");
            }
        }
    }
    file.close();
    for(const auto &[key, val] : config)
        std::cout << key << ": " << val << std::endl;
}

