#ifndef CMD_MAP_H
#define CMD_MAP_H

#include <map>
#include <string>

std::map<std::string, unsigned char> cmd_map = {
    { "INS_LDX_IM",0xA2 },
    { "INS_LDA_ABSX",0xBD }
};

std::map<std::string, unsigned char>::iterator cmd_it;

#endif