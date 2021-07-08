#include "QuoteService.h"
#include "CtpMD.h"
#include "ThostFtdcUserApiStruct.h"
#include <ctime>
#include <chrono>

QuoteService::QuoteService()
{

	for (auto& th : m_thDepthMarketData)
	{
		th.join();
	}
}

QuoteService::~QuoteService()
{
}

void QuoteService::InitCtpConfig(CtpConfig & ctpConfig, SysConfig& sysConfig)
{
     

    std::ofstream outFilePre;
    tp_csv = sysConfig.pred_file + "/tp.csv";
    outFilePre.open(tp_csv, std::ios::out | std::ios::trunc); // 打开模式可省略
    if (!outFilePre.is_open())
    {
        std::cout << pred_csv << "   open  fialed: " << std::endl;
    }

    outFilePre << "date" << ',' << "time" << ',' << "ask" << ',' << "ask.qty"<<','
        << "bid" << ',' << "bid.qty" << ',' << "turnover" << ',' << "qty" << ','
        << "pre" << ',' << "buytrade" << ',' << "selltrade" << ',' << "buy2trade" << ','
        << "sell2trade" << ',' << "price" << ',' << "resulthigh" << ',' << "resultlow" << ','
        << "inthigh" << ',' << "resulthigh2" << ',' << "resultlow2" << std::endl;
    outFilePre.close(); 
    

    md = new CtpMD();
    md->Init(ctpConfig);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    td = new CtpTD();
    td->GetMainContractInfo(sysConfig.mainContract_file.c_str());
    td->GetCancelFile(sysConfig.cancel_file.c_str());
    td->TradetLog(sysConfig.tradeLog_file.c_str());
    MarketLog(sysConfig.mdLog_file.c_str());
    td->Init(ctpConfig);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    md->CallBackDepthMarketData(std::bind(&QuoteService::DealDepthMarketData, this, std::placeholders::_1));
    this->StartTask();
   // std::this_thread::sleep_for(std::chrono::seconds(5));
    md->SubMarketData(m_instrument);

    
    /*
    //连接Postgresql 数据库获取合约列表
    std::vector<CodeInfo_s> vinstruments;
    vinstruments.assign(listCodes.begin(), listCodes.end());
    //全订阅
    int nCount = (int)vinstruments.size();
    char** ppInstrumentID = new char* [nCount];
    for (int i = 0; i < nCount; i++) {
        ppInstrumentID[i] = (char*)vinstruments.at(i).strCode.c_str();
    }
    mdapi->SubscribeMarketData(ppInstrumentID, nCount);
    */
   // m_instrument.push_back("rb2110");

   

}

void QuoteService::InitSysConfig(SysConfig& sysConfig, CtpConfig& ctpConfig)
{
    std::string head;
    std::string line;
    std::string product;
    std::string contract;
    bool closeIntraday;
    std::string exchange;

    std::cout << "\n==============初始化报单有效期类型============" << std::endl;
    m_orderType = THOST_FTDC_TC_GFD;
    if (ctpConfig.orderType == "ioc")
        m_orderType = THOST_FTDC_TC_IOC;
    std::cout << "m_orderType:立即完成，否则撤销" << std::endl;

    std::cout << "\n==============获取配置文件中的主力合约============" << std::endl;
    std::cout << "文件路径："<< sysConfig.mainContract_file << std::endl;
    std::ifstream inputContract(sysConfig.mainContract_file);
    std::getline(inputContract, head);
    int n = 0;
    // get contract information
    while (true) {
        getline(inputContract, line);
        if (inputContract.fail()) break;
        std::stringstream ss(line);
        ss >> product >> contract >> exchange >> closeIntraday;
        m_contractProduct[contract] = product;
        m_closeIntraday[contract] = closeIntraday;
        m_exchange[contract] = exchange;
        std::cout << m_contractProduct[contract] << " " << contract << " " << m_exchange[contract] << " " << m_closeIntraday[contract] << std::endl;
        m_lastCancel[contract] = 0;
        m_instrument.push_back(contract);
        m_contractIndex[contract] = n;
        m_turnover.push_back(m_iniTurnover[contract]);
        m_qty.push_back(m_iniQty[contract]);
        ++n;
    }
    inputContract.close();
    std::cout << "\n==============获取配置文件中的主力合约的参数信息============" << std::endl;
    std::cout << "文件路径：" << sysConfig.productInfo_file << std::endl;
    std::ifstream inputInfo(sysConfig.productInfo_file);
    getline(inputInfo, head);
    while (true) {
        getline(inputInfo, line);
        if (inputInfo.fail()) break;
        
        std::stringstream ss(line);
        std::string product;
        ss >> product;
        ss >> m_multiplier[product] >> m_spread[product] >> m_intFactor[product] >> m_nightEnd[product] >> m_afternoon[product] >> m_clearTime[product];
        std::cout << product << " " << m_multiplier[product] <<" "
                  << m_spread[product] << " " << m_intFactor[product] << " " << m_nightEnd[product] << " "
                  << m_afternoon[product] << " " << m_clearTime[product] <<std::endl;
    }
    inputInfo.close();

    std::cout << "\n==============获取配置文件中的品种对应的交易所============" << std::endl;
    std::cout << "文件路径：" << sysConfig.productExchange_file << std::endl;
    std::ifstream inputExchange(sysConfig.productExchange_file);
    std::string exchangeLine;
    getline(inputExchange, exchangeLine);
    while (true) {
        getline(inputExchange, exchangeLine);
        if (inputExchange.fail()) break;
        std::stringstream ss(exchangeLine);
        std::string product;
        std::string exchange;
        ss >> product >> exchange;
        strcpy(m_exchangeID[product], exchange.c_str());
    //    std::cout << product << " " << m_exchangeID[product] << std::endl;
    }
    inputExchange.close();
    
  
}

void QuoteService::InitTickStrategyConfig(SysConfig& sysConfig)
{
    std::cout << "\n==============获取对应合约历史tick数据============" << std::endl;
    std::cout << "文件路径：" << sysConfig.tickData_file << std::endl;
    for (auto contract : m_instrument)
    {
        std::string product = m_contractProduct[contract];
        std::string dateDirPath = sysConfig.tickData_file + "/" + product ;
        getFileName(dateDirPath.c_str(),dateFile);
        pre_value_date = dateFile;
        std::string dataFilePath = dateDirPath + "/" + dateFile;
        BookPtr newBook(new OrderBook(dataFilePath, product, m_multiplier[product], 
            m_spread[product], m_intFactor[product]));
        newBook->setup();
        listModelFiles(sysConfig.tickModelConfig_file.c_str());   
        std::map<std::string, std::map<std::string, modeConfigFile>>::iterator itr_mapProduct;   
        std::map<std::string, modeConfigFile>::iterator itr_mapModel;
        int iInstrument = 0;

        itr_mapProduct = mapProduct.find(product);
        if (itr_mapProduct != mapProduct.end())
        {
            for (itr_mapModel = itr_mapProduct->second.begin();
                itr_mapModel != itr_mapProduct->second.end();
                itr_mapModel++)
            {
                std::cout << "\ntickModelConfig_file model路径：" << sysConfig.tickModelConfig_file + '/' + itr_mapProduct->first + '/' + itr_mapModel->first + '/' + itr_mapModel->second.model << std::endl;
                std::cout << "tickModelConfig_file signal路径：" << sysConfig.tickModelConfig_file + '/' + itr_mapProduct->first + '/' + itr_mapModel->first + '/' + itr_mapModel->second.signal << std::endl;
                std::cout << "tickModelConfig_file position路径：" << sysConfig.tickModelConfig_file + '/' + itr_mapProduct->first + '/' + itr_mapModel->first + '/' + itr_mapModel->second.position << std::endl;
                std::cout << "tickModelConfig_file threshold路径：" << sysConfig.tickModelConfig_file + '/' + itr_mapProduct->first + '/' + itr_mapModel->first + '/' + itr_mapModel->second.threshold << std::endl;
                std::string signalFile = sysConfig.tickModelConfig_file + '/' + itr_mapProduct->first + '/' + itr_mapModel->first + '/' + itr_mapModel->second.signal;
                std::string modelFile = sysConfig.tickModelConfig_file + '/' + itr_mapProduct->first + '/' + itr_mapModel->first + '/' + itr_mapModel->second.model;
                std::string positionFile = sysConfig.tickModelConfig_file + '/' + itr_mapProduct->first + '/' + itr_mapModel->first + '/' + itr_mapModel->second.position;
                std::string thresholdFile = sysConfig.tickModelConfig_file + '/' + itr_mapProduct->first + '/' + itr_mapModel->first + '/' + itr_mapModel->second.threshold;
                //自动创建输出模型目录，子目录，文件
                std::string pred_product_file = sysConfig.pred_file + '/' + itr_mapProduct->first;
                if (access(pred_product_file.c_str(), 0) == -1)	//如果文件夹不存在
                {
                    std::cout << pred_product_file << "该文件夹不存在，准备创建" << std::endl;
                    mkdir(pred_product_file.c_str(),0777);				//则创建
                }
                else
                {
                    std::cout << pred_product_file << "该文件夹已经存在" << std::endl;
                }
                std::string pred_model_fiel = pred_product_file + '/' + itr_mapModel->first;
                if (access(pred_model_fiel.c_str(), 0) == -1)	//如果文件夹不存在
                {
                    std::cout << pred_model_fiel << "该文件夹不存在，准备创建" << std::endl;
                    mkdir(pred_model_fiel.c_str(),0777);				//则创建
                }
                else
                {
                    std::cout << pred_model_fiel << "该文件夹已经存在" << std::endl;
                }
                //创建每个模型文件下的pred.csv文件
                char DateTime[32];
                memset(DateTime, 0, sizeof(DateTime));
                GetDate(DateTime);;
                std::string predFile = pred_model_fiel + "/" + DateTime + "_" + itr_mapModel->first + "_pred.csv";
                
                IndicatorPtr newIndicator(new Indicator(signalFile)); // read in signal
       
                LinearPtr newLinear(new Linear(modelFile, thresholdFile , positionFile, predFile));
           

                newLinear->m_indMap = &newIndicator->m_indMap; // transfer the indicator map to linear model
          
                //typedef long    clock_t;
                ExtendedItr itr;
               
                for (itr = newBook->m_book.begin(); itr != newBook->m_book.end(); ++itr) {
                    newIndicator->updateWarmup(itr);
                    newLinear->getPred();
                }
               
                // newIndicator->outputIndicator(date);
                --itr;
                if (strcmp(itr->m_time, m_nightEnd[product].c_str()) >= 0
                    || strcmp(itr->m_time, "03:00:00") < 0)
                { // at the end of night
                    m_turnover[iInstrument] = newBook->m_cumTurnover; // we keep the newest turnover
                    m_qty[iInstrument] = newBook->m_cumQty; // we keep the newest volume
                    std::cout << "\n latest turnover " << std::to_string(m_turnover[iInstrument]) << " lastest volume " << m_qty[iInstrument] << std::endl;
                }
                map_signalPtr[contract].push_back(newIndicator);
                map_linearPtr[contract].push_back(newLinear);
            }
            m_bookPtr.push_back(newBook);
//            map_bookPtr[contract].push_back(newBook);
        }
        else
        {
            std::cout << contract << "no find tickModel" << std::endl;
            continue;
        }
    }

}

void QuoteService::StartTask()
{
	m_thDepthMarketData.emplace_back(std::thread(std::bind(&QuoteService::PushQue, this)));
}

void QuoteService::PushQue()
{
    
    while (1)
    {
        time_pre_value tp;
        RtData_s rtData;
        //NewData_s newData;
        Orders_s orderData;
        m_QueueRtdata.wait_dequeue(rtData);
        //判断当前tick是否为空
        if (rtData.strCode == " ")
            return;
        int index = m_contractIndex[rtData.strCode];
        contract = rtData.strCode;
        std::string product = m_contractProduct[contract];
        //更改tick 数据 
       // NewTick(newData,rtData);

        // get the new Tick
        ExtendedTick newTick(rtData.strActionDay,
            rtData.UpdateTime,
            rtData.iUpdateMillisec,
            rtData.dbLastPrice,
            (double)rtData.dbVolume-m_qty[index],
            (double)rtData.dbTurnover-m_turnover[index],
            rtData.dbOpenInterest,
            rtData.dbPreClosePrice,
            rtData.vecBidPrice[0],
            rtData.vecAskPrice[0], 
            (int)rtData.vecBidVolume[0],
            (int)rtData.vecAskVolume[0]);

        //非交易时间过滤
        
        if (strcmp(rtData.UpdateTime.c_str(), "20:55:00") < 0 && strcmp(rtData.UpdateTime.c_str(), "15:05:00") > 0
            || strcmp(rtData.UpdateTime.c_str(), "08:55:00") < 0 && strcmp(rtData.UpdateTime.c_str(), "03:00:00") > 0)
        {
            std::cout << "非交易时间" << std::endl;
            continue;
        }
        
  
        //m_bookPtr[index]->addTickData(newData);
        m_turnover[index] = (double)rtData.dbTurnover; // update cumulative turnover
        m_qty[index] = (double)rtData.dbVolume; // update cumulative volume
        if (fabs(rtData.dbUpperLimitPrice - rtData.vecBidPrice[0]) < 1e-6)
            newTick.m_ask = 0; // upper limit
        else if (fabs(rtData.dbLowerLimitPrice - rtData.vecAskPrice[0]) < 1e-6)
            newTick.m_bid = 0; // lower limit

        if (m_bookPtr[index]->m_count == 0) {
            ExtendedItr itr = m_bookPtr[index]->m_book.end();
            --itr;
            if (strcmp(itr->m_time, newTick.m_time) == 0 && itr->m_milli == newTick.m_milli) {
                //cout << "duplicate first tick" << endl;
                continue;
            }
            if (strcmp(newTick.m_time, "20:55:00") < 0 && strcmp(newTick.m_time, "15:05:00") > 0 ||
                strcmp(newTick.m_time, "08:55:00") < 0 && strcmp(newTick.m_time, "03:00:00") > 0) {
                //cout << "bad time tick" << endl;
                continue;
            }
        }
        m_bookPtr[index]->addTick(newTick); // push in the newTick
       
        itr_mapSignalPtr = map_signalPtr.find(contract);
        ExtendedItr itr = m_bookPtr[index]->m_book.end();
        --itr;
        //筛选
        
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
        if (itr_mapSignalPtr != map_signalPtr.end())
        {
            for (ssize_t i = 0; i < itr_mapSignalPtr->second.size(); i++)
            {
                itr_mapSignalPtr->second[i]->updateIndicator(itr); // calculate signal;
                //itr_mapSignalPtr->second[i]->outputIndicator(dateFile); // calculate signal;
            }
        }
        itr_maplinearPtr = map_linearPtr.find(contract);
        //筛选
        if (itr_maplinearPtr != map_linearPtr.end())
        {
            for (ssize_t i = 0; i < itr_maplinearPtr->second.size(); i++)
            {
                itr_maplinearPtr->second[i]->getPred(); // calculate prediction
                itr_maplinearPtr->second[i]->getAction(); // calculate buy/sell/open/close action
                itr_maplinearPtr->second[i]->getPosition(itr); // calculate theoretical position
                WriteTickToCsv(itr_maplinearPtr->second[i]->m_predFile, newTick.m_date, newTick.m_time, newTick.m_milli, contract, itr_maplinearPtr->second[i]->m_pred);
            }
        }
        std::chrono::milliseconds ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
        std::cout << "时间差：" << ms1.count() - ms.count() << std::endl;
        if (strcmp(newTick.m_time, m_clearTime[product].c_str()) > 0) {

            itr_maplinearPtr = map_linearPtr.find(contract);
            //筛选
            if (itr_maplinearPtr != map_linearPtr.end())
            {
                for (ssize_t i = 0; i < itr_maplinearPtr->second.size(); i++)
                {
                    for (auto iStrat = 0; iStrat != itr_maplinearPtr->second[i]->m_strat.size(); ++iStrat)
                    {
                        itr_maplinearPtr->second[i]->m_position[iStrat] = 0;
                        itr_maplinearPtr->second[i]->m_totalPos = 0;

                    }
                }
                m_orderType = THOST_FTDC_TC_IOC; // close position with active order
            }
        }
        else if (std::to_string(m_orderType) != "ioc")
            m_orderType = THOST_FTDC_TC_GFD;
        
       //计算当前品种所有模型的理论仓位
        itr_maplinearPtr = map_linearPtr.find(contract);
        //筛选
        int simPos=0;
        int yesPos=0;
        if (itr_maplinearPtr != map_linearPtr.end())
        {
            for (ssize_t i = 0; i < itr_maplinearPtr->second.size(); i++)
            {     
                simPos += itr_maplinearPtr->second[i]->m_totalPos;
                yesPos += itr_maplinearPtr->second[i]->yesterdayPos;     
            }
        }

        if (!td->m_finishPos)
            continue;
        int yLong = td->m_yesterdayLong[contract]; // yesterday long position
        int yShort = td->m_yesterdayShort[contract]; // yesterday short position

        if (yesPos > 0)
        {
            tLong = yesPos + td->m_todayLong[rtData.strCode];
            tShort = td->m_todayShort[rtData.strCode];
        }
        else
        {
            tLong = td->m_todayLong[rtData.strCode];
            tShort = -yesPos + td->m_todayShort[rtData.strCode];
        }
        int curPos = tLong - tShort;
        CThostFtdcOrderField* pOrder = td->m_activeOrder[contract];

        LinearSol result(newTick.m_turnover, newTick.m_qty, newTick.m_bid, newTick.m_ask, m_bookPtr[index]->m_multiplier);
        double resultlow = result.low;
        double resulthigh = result.high;
        double intlow;
        double inthigh;
        if (isWholeNumber(result.low)) {
            intlow = 1;
        }
        else
            intlow = 0;
        if (isWholeNumber(result.high)) {
            inthigh = 1;
        }
        else
            inthigh = 0;

        double price;
        double resultlow2;
        double resulthigh2;
      
        if (newTick.m_qty == 0)
        {
            price = 0;
            resultlow2 = 0;
            resulthigh2 = 0;
        }
        else
        {
            price = (newTick.m_turnover) / m_bookPtr[index]->m_multiplier / newTick.m_qty;

            int m_intFactor = 1;
            double priceInt = price * m_intFactor;
            int upInt = (int)ceil(priceInt);
            int loInt = (int)floor(priceInt);

            double loPrice = double(loInt) / m_intFactor;
            double upPrice = double(upInt) / m_intFactor;
            //cout << price << " " << priceInt << " " << loPrice << " " << upPrice <<endl;
           
            LinearSol result2(newTick.m_turnover, newTick.m_qty, loPrice, upPrice, m_bookPtr[index]->m_multiplier);
            resulthigh2 = result2.high;
            resultlow2 = result2.low;
        }
        
        itr_maplinearPtr = map_linearPtr.find(contract);
        //筛选
      
        if (itr_maplinearPtr != map_linearPtr.end())
        {
            for (ssize_t i = 0; i < itr_maplinearPtr->second.size(); i++)
            {
               
                    std::cout << newTick.m_date << " " << newTick.m_time << " " << contract
                        << " " << newTick.m_bidQty << " " << newTick.m_bid << " " << newTick.m_ask
                        << " " << newTick.m_askQty << " pred " << itr_maplinearPtr->second[i]->m_pred
                        << " position " << simPos
                        << " real pos " << curPos
                        << " with order " << (pOrder != NULL)
                        << " sent order " << (td->m_sentOrder[contract] != NULL)
                        << " cancel# " << td->m_cancelCount[contract]
                        << " remainQty " << td->m_remainQty[contract]
                        <<std::endl;
                    m_marketLog << newTick.m_date << " " << newTick.m_time << " " << contract
                        << " " << newTick.m_bidQty << " " << newTick.m_bid << " " << newTick.m_ask
                        << " " << newTick.m_askQty << " pred " << itr_maplinearPtr->second[i]->m_pred
                        << " position " << simPos
                        << " real pos " << curPos
                        << " with order " << (pOrder != NULL)
                        << " sent order " << (td->m_sentOrder[contract] != NULL)
                        << " cancel# " << td->m_cancelCount[contract]
                        << " remainQty " << td->m_remainQty[contract];
                   // WriteTickToCsv(pred_csv, newTick.m_date, newTick.m_time,newTick.m_milli, contract,itr_maplinearPtr->second[i]->m_pred);

                    tp.p_date = newTick.m_date;
                    tp.p_time = newTick.m_time;
                    tp.prediction_value = itr_maplinearPtr->second[i]->m_pred;
                    tp.ask = newTick.m_ask;
                    tp.askqty = newTick.m_askQty;
                    tp.bid = newTick.m_bid;
                    tp.bidqty = newTick.m_bidQty;
                    tp.turnover = newTick.m_turnover;
                    tp.qty = newTick.m_qty;
                    tp.buytrade = itr->m_buyTrade;
                    tp.selltrade = itr->m_sellTrade;
                    tp.buy2trade = itr->m_buy2Trade;
                    tp.sell2trade = itr->m_sell2Trade;
                    tp.price = price;
                    tp.resulthigh = resulthigh;
                    tp.resultlow = resultlow;
                    tp.inthigh = inthigh;
                    tp.intlow = intlow;
                    tp.resulthigh2 = resulthigh2;
                    tp.resultlow2 = resultlow2;

                    WriteTickTpToCsv(tp_csv, tp);
                    //Pre_value_map[product].push_back(tp);/////////////////pre_valuetp.p_date = newTick.m_date;
                    //m_signalPtr[index]->outputIndicator(pre_value_date);
                   
            }
        }
       
        
//        WriteTickToCsv("output/pred.csv", newData, m_linearPtr[index]->m_pred);
        if (pOrder != NULL) {
            std::cout << " " << pOrder->OrderRef;
            m_marketLog << " " << pOrder->OrderRef;
        }
        std::cout << std::endl;
        m_marketLog << std::endl;
        if (pOrder == NULL)
            pOrder = td->m_sentOrder[contract];
        if (strcmp(newTick.m_time, m_clearTime[product].c_str()) > 0 && pOrder != NULL)
        {
            td->reqOrderAction(pOrder);

            td->m_sentCancel[pOrder->OrderRef] = 1;
            m_lastCancel[contract] = m_bookPtr[index]->m_count;
        }

        if (pOrder != NULL)
        {
            if (pOrder->LimitPrice + 1e-6 < newTick.m_bid && pOrder->Direction == THOST_FTDC_D_Buy)
            {
                if (td->m_sentOrder[contract] != NULL)
                    if (strcmp(pOrder->OrderRef, td->m_sentOrder[contract]->OrderRef) == 0)
                        td->m_sentOrder[contract] = NULL;
                td->reqOrderAction(pOrder);
               
                td->m_sentCancel[pOrder->OrderRef] = 1;
                m_lastCancel[contract] = m_bookPtr[index]->m_count;
               
                continue; // or cancle command may not arrivte at the exchange, so we don't need to do anything now
            }
            else if (pOrder->LimitPrice - 1e-6 > newTick.m_ask && pOrder->Direction == THOST_FTDC_D_Sell)
            {
                //if (pOrder->OrderRef==NULL) return;
                if (td->m_sentOrder[contract] != NULL)
                    if (strcmp(pOrder->OrderRef, td->m_sentOrder[contract]->OrderRef) == 0)
                        td->m_sentOrder[contract] = NULL;
                td->reqOrderAction(pOrder);
                // unique_lock<mutex> lock(pMdSpi->m_mtxTrade);
                td->m_sentCancel[pOrder->OrderRef] = 1;
                m_lastCancel[contract] = m_bookPtr[index]->m_count;
                // lock.unlock();
                continue;
            }
        }


        if (m_bookPtr[index]->m_count - m_lastCancel[contract] >= 10)
        { // have sent a cancel order 10 ticks ago
        //    unique_lock<mutex> lock(pMdSpi->m_mtxTrade);
            if (td->m_activeOrder[contract] != NULL)
            { // there is an active order now
                if (td->m_sentCancel.count(td->m_activeOrder[contract]->OrderRef) > 0)
                {
                    td->m_activeOrder[contract] = NULL; // if we wait 10 ticks and still not received canceLED confirmation
                }
            }
            // lock.unlock();
        }
        if (simPos == curPos || td->m_activeOrder[contract] != NULL || td->m_sentOrder[contract] != NULL)
            continue;
        double spread = m_spread[product]; // get product's sprad

        //模型持仓更改
        itr_maplinearPtr = map_linearPtr.find(contract);
        if (itr_maplinearPtr != map_linearPtr.end())
        {
            for (ssize_t i = 0; i < itr_maplinearPtr->second.size(); i++)
            {
                std::string posFile = itr_maplinearPtr->second[i]->m_posFile;
                std::vector<int> position = itr_maplinearPtr->second[i]->m_position;
                std::vector<TradeThre> strat = itr_maplinearPtr->second[i]->m_strat;
                itr_maplinearPtr->second[i]->output(rtData.strActionDay, posFile, position, strat);
            }
        }

        orderData.strCode = rtData.strCode;
        orderData.strMarket = rtData.strMarket;
        orderData.ePriceType = LimitPrice;
        orderData.eHedgeFlage = HedgeFlag_Spe;

        if (m_orderType == THOST_FTDC_TC_GFD)
            orderData.eTimeConditionType = FTDC_TC_GFD;
        else
            orderData.eTimeConditionType = FTDC_TC_IOC;

        if (simPos > curPos)
        { // if we need to buy
            if (yShort > 0 && curPos < 0)
            {
                // if there is yesterday's position
                orderData.dbPrice = newTick.m_bid + spread;
                orderData.eDirection = DIREC_Buy;
                orderData.eOffSetFlag = OFFSET_Close;
                if (m_orderType == THOST_FTDC_TC_IOC)
                    orderData.eTimeConditionType = FTDC_TC_IOC;
                else
                    orderData.eTimeConditionType = FTDC_TC_GFD;
                if (simPos < 0)
                {
                    int qty = (yShort <= simPos - curPos ? yShort : simPos - curPos);
                    orderData.intVolume = qty;
                    td->reqOrderInsert(orderData);
                }
                else
                {
                    int qty = (yShort <= -curPos ? yShort : -curPos);
                    orderData.intVolume = qty;
                    td->reqOrderInsert(orderData);
                }
            }
            else if (m_closeIntraday[contract] && tShort > 0)
            {// if no yesterday position, and close today is better
                int qty = (tShort <= simPos - curPos ? tShort : simPos - curPos);
                orderData.intVolume = qty;
                orderData.dbPrice = newTick.m_bid + spread;
                orderData.eDirection = DIREC_Buy;

                if (m_orderType == THOST_FTDC_TC_GFD)
                    orderData.eTimeConditionType = FTDC_TC_GFD;
                else
                    orderData.eTimeConditionType = FTDC_TC_IOC;

                if (m_exchange[contract] == "SHFE") // shanghai exchange
                {
                    orderData.eOffSetFlag = OFFSET_CloseYestoday;
                    td->reqOrderInsert(orderData);
                }
                else
                {
                    orderData.eOffSetFlag = OFFSET_CloseToday;
                    td->reqOrderInsert(orderData);
                }

            }
            else
            {
                orderData.intVolume = simPos - curPos;
                orderData.dbPrice = newTick.m_bid + spread;
                orderData.eDirection = DIREC_Buy;
                orderData.eOffSetFlag = OFFSET_Open;

                // td->reqOrderInsert((char*)rtData.strCode.c_str(), exchangeID, newTick.m_bid + spread, simPos - curPos,
                 //    THOST_FTDC_D_Buy, THOST_FTDC_OF_Open, m_orderType);
                td->reqOrderInsert(orderData);
            }
        }
        else if (yLong > 0 && curPos > 0)
        {
            orderData.dbPrice = newTick.m_bid - spread;
            orderData.eDirection = DIREC_Sell;
            orderData.eOffSetFlag = OFFSET_Close;
            if (m_orderType == THOST_FTDC_TC_GFD)
                orderData.eTimeConditionType = FTDC_TC_GFD;
            else
                orderData.eTimeConditionType = FTDC_TC_IOC;
            if (simPos > 0)
            {
                int qty = (yLong <= curPos - simPos ? yLong : curPos - simPos);
                orderData.intVolume = qty;           
                td->reqOrderInsert(orderData);
                continue;
            }
            else
            {
                int qty = (yLong <= curPos ? yLong : curPos);
                orderData.intVolume = qty;
                orderData.eTimeConditionType = FTDC_TC_GFD;
                td->reqOrderInsert(orderData);
                continue;
            }
        }
        else if (m_closeIntraday[contract] && tLong > 0) 
        {
            orderData.dbPrice = newTick.m_bid - spread;
            orderData.eDirection = DIREC_Sell;
            if (m_orderType == THOST_FTDC_TC_GFD)
                orderData.eTimeConditionType = FTDC_TC_GFD;
            else
                orderData.eTimeConditionType = FTDC_TC_IOC;
            if (m_exchange[contract] == "SHFE")
            {// shanghai exchange
                int qty = (yLong <= curPos ? yLong : curPos);
                orderData.intVolume = qty;     
                orderData.eOffSetFlag = OFFSET_CloseYestoday;
                td->reqOrderInsert(orderData);
            }
            else
            {
                int qty = (yLong <= curPos - simPos ? yLong : curPos - simPos);
                orderData.intVolume = qty;
                orderData.eOffSetFlag = OFFSET_Close;  
                td->reqOrderInsert(orderData);
               
            }
        }
        else // if there is no yesterday position, and close today is more expensive, or no today's long position, we need to sell open
        {
            orderData.intVolume = curPos - simPos;
            orderData.dbPrice = newTick.m_bid - spread;
            orderData.eDirection = DIREC_Sell;
            orderData.eOffSetFlag = OFFSET_Open;
            if (m_orderType == THOST_FTDC_TC_GFD)
                orderData.eTimeConditionType = FTDC_TC_GFD;
            else
                orderData.eTimeConditionType = FTDC_TC_IOC;
                td->reqOrderInsert(orderData);
        }
    }
    
    
}

void QuoteService::DealDepthMarketData(RtData_s& rtData)
{
	auto ID = m_mapMarket.find(rtData.strCode);
	if (ID != m_mapMarket.end())
	{
		rtData.strMarket = ID->second;
	}
	//入队
	m_QueueRtdata.enqueue(rtData);
}

void QuoteService::GetCodeList(std::list<CodeInfo_s> &listCodes)
{
	for (auto& Code : listCodes)
	{
		m_mapMarket[Code.strCode] = Code.strMarket;
	}
}

void QuoteService::WriteTickToCsv(std::string fileName,
    std::string date, 
    std::string time,
    int iUpdateMillisec,
    std::string contract,
    double& pred)
{
    std::ofstream outFile;
    outFile.open(fileName, std::ios::out | std::ios::app); // 打开模式可省略
    if (!outFile.is_open())
    {
        std::cout << " pred.csv  open1  fialed: " << std::endl;
    }
    outFile << date << " ," << time << " ," << std::to_string(iUpdateMillisec) << " ," << contract<<", "<< pred << std::endl;
        
    outFile.close();
}

void QuoteService::WriteTickTpToCsv(std::string fileName, time_pre_value& tp)
{
   
    std::ofstream outFile;
    outFile.open(fileName, std::ios::out | std::ios::app); // 打开模式可省略
    if (!outFile.is_open())
    {
        std::cout << " pred.csv  open1  fialed: " << std::endl;
    }

    outFile << tp.p_date << ',' << tp.p_time << ',' << tp.ask << ',' << tp.askqty << ','
        << tp.bid << ',' << tp.bidqty << ',' << tp.turnover << ',' << tp.qty << ','<< tp.prediction_value <<','
        << tp.buytrade << ',' << tp.selltrade << ',' << tp.buy2trade << ',' << tp.sell2trade << ','
        << tp.price << ',' << tp.resulthigh << ',' << tp.resultlow << ','
        << tp.inthigh << ',' << tp.resulthigh2 << ',' << tp.resultlow2 << std::endl;

    outFile.close();
}



void QuoteService::OutPut(SysConfig& sysConfig)
{
	m_marketLog.close();
	int index = 0;

	time_t now = time(0);
	tm* gmtm;
	gmtm = gmtime(&now);

	std::string file;
	m_bookPtr[0]->outputorderbook();
	

	for (auto item : Pre_value_map)
	{
        itr_mapSignalPtr = map_signalPtr.find(contract);
        if (itr_mapSignalPtr != map_signalPtr.end())
        {
            for (ssize_t i = 0; i < itr_mapSignalPtr->second.size(); i++)
            {
                itr_mapSignalPtr->second[i]->outputIndicator(pre_value_date); // calculate signal;
            }
        }
		// m_signalPtr[index]->outputMiddle(pre_value_date);
		   // string date = pre_value_date.substr(0,5) + std::to_string(gmtm->tm_mday);
		std::string date;
		if ((1 + gmtm->tm_mon) < 10)
		{
			date = std::to_string(1900 + gmtm->tm_year) + "0" + std::to_string(1 + gmtm->tm_mon) + std::to_string(gmtm->tm_mday);
		}
		else
			date = std::to_string(1900 + gmtm->tm_year) + std::to_string(1 + gmtm->tm_mon) + std::to_string(gmtm->tm_mday);
		std::string outputFile = sysConfig.pred_file +"/" + item.first + "/" + date + ".txt";
		std::ofstream output(outputFile, std::ofstream::out | std::ofstream::app);

		if (gmtm->tm_hour >= 15 && gmtm->tm_hour < 21) {
			output << "date time ask ask.qty bid bid.qty turnover qty pre buytrade selltrade buy2trade sell2trade price resulthigh resultlow inthigh intlow resulthigh2 resultlow2" << std::endl; // output heads
		}
		output.precision(15);
		for (auto i : item.second)
		{
			output << i.p_date << " " << i.p_time << " " << i.ask << " " << i.askqty << " " << i.bid << " " << i.bidqty << " " << i.turnover << " " << i.qty << " " << i.prediction_value << " " << i.buytrade <<
				" " << i.selltrade << " " << i.buy2trade << " " << i.sell2trade << " " << i.price << " " << i.resulthigh << " " << i.resultlow << " " << i.inthigh << " " << i.intlow << " " << i.resulthigh2 << " " << i.resultlow2
				<< std::endl;
		}
		index = index + 1;
		output.close();
	}
  
}

void QuoteService::MarketLog(const char* FilePath)
{
    // market log
  //  std::string logFile = "output/log.txt";

    m_marketLog.open(FilePath, std::ofstream::out | std::ofstream::app);
}

void QuoteService:: GetDate(char * psDate)
{

    time_t nSeconds;
    struct tm * pTM;
    
    time(&nSeconds); // 同 nSeconds = time(NULL);
    pTM = localtime(&nSeconds);
    
    /* 系统日期,格式:YYYMMDD */
    sprintf(psDate,"%04d-%02d-%02d", 
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday);
    
}

void QuoteService::listModelFiles(const char* dir_name)
{
    char dirNew[200];
    if( NULL == dir_name )
    {  
        cout<<" dir_name is null ! "<<endl;
        return;
    }

    struct stat s;  
    lstat( dir_name , &s );  
    if( ! S_ISDIR( s.st_mode ) )  
    {
        return;
    }
      
    struct dirent * filename;
    DIR * dir;
    dir = opendir( dir_name );  
    if( NULL == dir )  
    {  
        return;  
    }  

    int iName=0;
    while( ( filename = readdir(dir) ) != NULL )  
    {  
        if( strcmp( filename->d_name , "." ) == 0 ||
            strcmp( filename->d_name , "..") == 0)
            continue;

        cout << filename->d_name << "\t<dir>\n" << endl;
        strcpy(dirNew,dir_name);
        strcat(dirNew,"/");
        strcat(dirNew,filename->d_name);
        std::string dirName = filename->d_name;
        listFiles(dirNew,dirName);
    }
    closedir(dir);
}

void QuoteService::listFiles(char* dir_name, std::string& dirName)
{
    char dirNew[200];
    strcpy(dirNew, dir_name);
    strcat(dirNew, "/*.*"); //在目录后面加上"\\*.*"进行第一次搜索.
    modeConfigFile config;
    if( NULL == dir_name )
    {  
        cout<<" dir_name is null ! "<<endl;
        return;
    }

    struct stat s;  
    lstat( dir_name , &s );  
    if( ! S_ISDIR( s.st_mode ) )  
    {
        return;
    }
      
    struct dirent * filename;
    DIR * dir;
    dir = opendir( dir_name );  
    if( NULL == dir )  
    {  
        return;  
    }  

    int iName=0;
    while( ( filename = readdir(dir) ) != NULL )  
    {  
        if( strcmp( filename->d_name , "." ) == 0 ||
            strcmp( filename->d_name , "..") == 0)
            continue;

        cout << filename->d_name << "\t<dir>\n" << endl;
        std::string dirName_path= filename->d_name;
        mapModel[dirName_path] = config;
        mapProduct[dirName] = mapModel;
    }
    mapModel.clear();
    closedir(dir);

}

void QuoteService::getFileName(const char* dirPath , std::string &dataName)
{
    if( NULL == dirPath )
    {  
        cout<<" dir_name is null ! "<<endl;
        return;
    }

    struct stat s;  
    lstat( dirPath , &s );  
    if( ! S_ISDIR( s.st_mode ) )  
    {
      
        return;
    }
      
    struct dirent * filename;
    DIR * dir;
    dir = opendir( dirPath );  
    if( NULL == dir )  
    {  
        
        return;  
    }  

    int iName=0;
    while( ( filename = readdir(dir) ) != NULL )  
    {  
        if( strcmp( filename->d_name , "." ) == 0 ||
            strcmp( filename->d_name , "..") == 0)
            continue;
        if( filename->d_type == DT_REG)
        {
            dataName = filename->d_name;
            
            break;
        }
    }
    closedir(dir);
}


