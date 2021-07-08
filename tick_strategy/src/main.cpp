
#include "JsonTools.h"
#include "ConfigTools.h"
#include "QuoteService.h"

#include <fstream>
#include <string>
#include <thread>
#include <list>
#include <map>
#include <iostream>

int main()
{
    CtpConfig ctpConfig;
    PGSqlConfig pgConfig;
    SysConfig sysConfig;
    //获取pg数据库配置文件
    ReadPGSqlConfigFile("input/sysConfig/pgsql_config.json", pgConfig);
    //登陆数据库pg 获取合约列表
    std::list<CodeInfo_s> listCodes;
    InitPostgresql(pgConfig, listCodes);

    //获取tick策略模型配置文件
    ReadSysConfigFile("input/sysConfig/sys_config.json", sysConfig);
    
    //获取ctp配置文件
    ReadCtpConfigFile("input/sysConfig/ctp_config.json", ctpConfig);
   
    QuoteService* quoteService = new QuoteService();
    quoteService->GetCodeList(listCodes);

    quoteService->InitSysConfig(sysConfig, ctpConfig);
    quoteService->InitTickStrategyConfig(sysConfig);
    quoteService->InitCtpConfig(ctpConfig,sysConfig);
    
    char action;
    while (true) {
        cout << "enter s to end" << endl;
        cin >> action;
        if (action == 's' || action == 'S')
            //getMarket.join();
            break;
    }
//    quoteService->OutPut(sysConfig);
   
    return 0;
}
 
