#pragma once
#ifndef JSON_TOOL_H
#define JSON_TOOL_H

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "ThostFtdcUserApiStruct.h"
#include <string.h>
#include <fstream>
#include <vector>
#include <iostream>

void readFromJson(std::vector <CThostFtdcInstrumentField>& instruments, std::string& filePath);
void WritToJson(std::vector <CThostFtdcInstrumentField>& instruments, std::string& filePath);

#endif // !JSON_TOOL_H
