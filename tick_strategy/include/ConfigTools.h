
#ifndef CONFIG_TOOLS_H
#define CONFIG_TOOLS_H


#include "GenData.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/document.h"
#include "pqxx/pqxx"
#include <sstream>
#include <fstream>
#include <string>
#include <list>
#include <map>
#include <iostream>

void ReadCtpConfigFile(const char* configPath, CtpConfig& ctpConfig);
void ReadPGSqlConfigFile(const char* configPath, PGSqlConfig& pgConfig);
void ReadSysConfigFile(const char* configPath, SysConfig& sysConfig);
void InitPostgresql(PGSqlConfig& pgconfig, std::list<CodeInfo_s>& listCodes);



#endif // !CONFIG_TOOLS_H
