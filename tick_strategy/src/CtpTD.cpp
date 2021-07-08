#include "CtpTD.h"

CtpTD::CtpTD()
{
	m_requestID = 1;
    getTime();

}

CtpTD::~CtpTD()
{
    if (m_tdapi) {
        m_tdapi->RegisterSpi(nullptr);
        m_tdapi->Release();
        m_tdapi = nullptr;
    }
}

void CtpTD::OnFrontConnected()
{
    std::cout << "=====建立网络连接成功=====" << std::endl;
    std::cout << "OnFrontConnected..." << std::endl;
    reqAuthenticate();
     
}

void CtpTD::OnFrontDisconnected(int nReason)
{
    std::cout << "OnFrontDisconnected, nReason:" << nReason << std::endl;
}

void CtpTD::OnHeartBeatWarning(int nTimeLapse)
{
    std::cerr << "=====Network Heart Breaks=====" << std::endl;
    std::cerr << "Time from last " << nTimeLapse << std::endl;
}






void CtpTD::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    if (!isErrorRspInfo(pRspInfo)) {
        m_loginFlag = false;
        std::cout << "=====Account Log Off Successfully=====" << std::endl;
        std::cout << "Broker " << pUserLogout->BrokerID << std::endl;
        std::cout << "Account " << pUserLogout->UserID << std::endl;
        getTime();
        m_log << m_currentTime << " log off " << " broker " << pUserLogout->BrokerID
            << " account " << pUserLogout->UserID << std::endl;
    }
}

void CtpTD::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    std::cout << "error!" << std::endl;
    isErrorRspInfo(pRspInfo);
}





void CtpTD::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    std::unique_lock<std::mutex> lk(m_muxPos);
    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "请求查询投资者持仓响应成功" << std::endl;
        if (pInvestorPosition) {
            int yp = pInvestorPosition->YdPosition;
            int tp = pInvestorPosition->Position;
            char dire = pInvestorPosition->PosiDirection;
            char* id = pInvestorPosition->InstrumentID;
            strcpy((char*)id, (char*)pInvestorPosition->InstrumentID);
            std::string instrument(pInvestorPosition->InstrumentID);
            std::cout << id << " yp " << yp << " tp " << tp << " dire " << dire << std::endl;
            //if (m_yesterdayLong.count(pInvestorPosition->InstrumentID)>0) {
            if (m_yesterdayLong.count(instrument) > 0) {
                if (yp > 0) {
                    if (dire == '2')
                        m_yesterdayLong[instrument] = tp;
                    else if (dire == '3')
                        m_yesterdayShort[instrument] = tp;
                }
            }
        }
        // dire: 1-net; 2-long; 3-short;
        if (bIsLast) {
            m_finishPos = true;
            getTime();
            std::cout << "完成持仓查询" << std::endl;
            for (int i = 0; i != m_instrumentNum; ++i) {
                //char* contract=m_instrumentID[i];
                std::string contract(m_instrumentID[i]);
                //strcpy_s(m_orderRef[contract],"\0");
                m_count[contract] = 0;
                if (m_yesterdayLong[contract] > 0) {
                    std::cout << contract << " yLong " << m_yesterdayLong[contract] << std::endl;
                    m_log << m_currentTime << " " << contract << " yLong " << m_yesterdayLong[contract] << std::endl;
                }
                if (m_yesterdayShort[contract] > 0) {
                    std::cout << contract << " yShort " << m_yesterdayShort[contract] << std::endl;
                    m_log << m_currentTime << " " << contract << " yShort " << m_yesterdayShort[contract] << std::endl;
                }
                if (m_todayLong[contract] > 0) {
                    std::cout << contract << " tLong " << m_todayLong[contract] << std::endl;
                    m_log << m_currentTime << " " << contract << " tLong " << m_todayLong[contract] << std::endl;
                }
                if (m_todayShort[contract] > 0) {
                    std::cout << contract << " tShort " << m_todayShort[contract] << std::endl;
                    m_log << m_currentTime << " " << contract << "tShort " << m_todayShort[contract] << std::endl;
                }
            }
        }
    }
    

}

void CtpTD::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    //std::cout << "=====SPI Order Insert Successfully=====" << std::endl;
    if (!isErrorRspInfo(pRspInfo)) {
        std::cout << "=====Order Insert Successfully=====" << std::endl;
        std::cout << "Instrument ID: " << pInputOrder->InstrumentID << std::endl;
        std::cout << "Price: " << pInputOrder->LimitPrice << std::endl;
        std::cout << "Volume: " << pInputOrder->VolumeTotalOriginal << std::endl;
        std::cout << "Direction: " << pInputOrder->Direction << std::endl;
    }
    else {
        std::cout << "Error Order " << pInputOrder->InstrumentID << " " << pInputOrder->LimitPrice << " " << pInputOrder->OrderRef << " "
            << nRequestID << " " << bIsLast << std::endl;
        std::string instrument(pInputOrder->InstrumentID);
        if (m_sentOrder[instrument] != NULL &&
            strcmp(m_sentOrder[instrument]->OrderRef, pInputOrder->OrderRef) == 0) {
            std::cout << "set sent order to NULL" << std::endl;
            m_sentOrder[instrument] = NULL;
        }
    }
}


void CtpTD::OnRtnOrder(CThostFtdcOrderField* pOrder)
{
    if (pOrder == nullptr)
    {
        std::cout << "pOrder is null  " << std::endl;
        return;
    }
    int orderRef = atoi(pOrder->OrderRef);
    std::string contract(pOrder->InstrumentID);
    std::unique_lock<std::mutex> lkOrder(m_muxOrder);
    if (isMyOrder(pOrder)) {
        if (isTradingOrder(pOrder)) { // if it's an open trade, it may already been here; // if it's an close trade
          // it may not be successfuly so we need the active reference to confirm
          //unique_lock<mutex> lock(m_mtxTrade);
            if (m_sentCancel.count(pOrder->OrderRef) == 0) {
                m_activeOrder[contract] = pOrder;
                m_sentOrder[contract] = NULL;
                getTime();
                m_log << m_currentTime << " " << pOrder->InstrumentID << " " << pOrder->OrderRef << " active " << std::endl;
            }
            //lock.unlock();
        }
        else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled) {
           
            m_cancelCount[pOrder->InstrumentID]++;
            getTime();
            std::string c_s = pOrder->OrderRef;
           
            std::vector<std::string>::iterator itor = std::find(orderRefs.begin(), orderRefs.end(), pOrder->OrderRef);
            if (itor != orderRefs.end())
            {
                orderRefs.erase(itor);
            }
            m_log << m_currentTime << " " << pOrder->InstrumentID << " " << pOrder->OrderRef << " canceled " << std::endl;
            if (m_activeOrder[contract] != NULL)
                if (strcmp(pOrder->OrderRef, m_activeOrder[contract]->OrderRef) == 0) { // if the order is in the active order map
                    m_activeOrder[contract] = NULL;
                    m_remainQty[contract] = 0;
                }
            if (m_sentOrder[contract] != NULL && m_activeOrder[contract] == NULL)
                if (strcmp(pOrder->OrderRef, m_sentOrder[contract]->OrderRef) == 0) { // if the order is in the sent order map
                    m_sentOrder[contract] = NULL;
                    m_remainQty[contract] = 0;
                }
            m_sentCancel.erase(pOrder->OrderRef);
           
        }
    }
    lkOrder.unlock();
}


void CtpTD::OnRtnTrade(CThostFtdcTradeField* pTrade)
{
    if (pTrade == nullptr)
    {
        std::cout << "pTrade is null  " << std::endl;
        return;
    }
    if (pTrade->OffsetFlag == '4') { // close yesterday
        if (pTrade->Direction == '0') // buy
            m_yesterdayShort[pTrade->InstrumentID] -= pTrade->Volume; // buy close yesterday
        else // sell
            m_yesterdayLong[pTrade->InstrumentID] -= pTrade->Volume; // sell close yesterday
    }
    if (!m_finishPos) return;
    if (!isMyTrader(orderRefs, pTrade)) return;
    int hour = (pTrade->TradeTime[0] - '0') * 10 + (pTrade->TradeTime[1] - '0');
    int minute = (pTrade->TradeTime[3] - '0') * 10 + (pTrade->TradeTime[4] - '0');
    int second = (pTrade->TradeTime[6] - '0') * 10 + (pTrade->TradeTime[7] - '0');
    int timeValue = hour * 3600 + minute * 60 + second;
    
    std::string contract(pTrade->InstrumentID);
    if (pTrade->OffsetFlag == '0') { // open
        if (pTrade->Direction == '0') // buy
            m_todayLong[contract] += pTrade->Volume; // buy open today
        else // sell
            m_todayShort[contract] += pTrade->Volume; // sell open today
    }
    else if (pTrade->OffsetFlag == '4') { // close yesterday
        if (pTrade->Direction == '0') // buy
        {
            //m_yesterdayShort[contract]-=pTrade->Volume; // buy close yesterday
            m_todayShort[contract] -= pTrade->Volume;
        }
        else // sell
        {
            //m_yesterdayLong[contract]-=pTrade->Volume; // sell close yesterday
            m_todayLong[contract] -= pTrade->Volume;
        }
    }
    else if (pTrade->OffsetFlag == '3') { // close today, only for shanghai
        if (pTrade->Direction == '0') // buy
            m_todayShort[contract] -= pTrade->Volume; // buy close today
        else // sell
            m_todayLong[contract] -= pTrade->Volume; // sell close today
    }
    else if (pTrade->OffsetFlag == '1') { // close
        if (pTrade->Direction == '0')
            //  int remainVolume=pTrade->Volume; // remain volume to close today
              //m_yesterdayShort[contract]=0; // yesterday short is zero now
            m_todayShort[contract] -= pTrade->Volume; // today short minus the remainVolume

        else if (pTrade->Direction == '1')  // sell
          // int remainVolume=pTrade->Volume; // remain volume to sell today
            //m_yesterdayLong[contract]=0; // yesterday long is zero
            m_todayLong[contract] -= pTrade->Volume; // today long minus the remainVolume
    }
    getTime();
    
    m_remainQty[contract] -= pTrade->Volume;
   
    if (m_remainQty[contract] <= 0) {
        m_activeOrder[contract] = NULL;
        m_sentOrder[contract] = NULL;
        m_sentCancel.erase(pTrade->OrderRef);
        m_remainQty[contract] = 0;
        m_log << pTrade->InstrumentID << " set remain qty to " << m_remainQty[contract] << std::endl;

        std::vector<std::string>::iterator itor = std::find(orderRefs.begin(), orderRefs.end(), pTrade->OrderRef);
        if (itor != orderRefs.end())
        {
            orderRefs.erase(itor);
        }

    }
    
    m_log << m_currentTime << " trade " << pTrade->InstrumentID << " " << pTrade->OrderRef << " " << pTrade->Volume << " " << pTrade->Price << " " << pTrade->Direction << " " << pTrade->OffsetFlag << std::endl;
    std::cout << pTrade->TradeTime << " " << pTrade->InstrumentID << " "
        << pTrade->Volume << "@" << pTrade->Price << " " << pTrade->Direction << " " << pTrade->OffsetFlag << std::endl;
}

void CtpTD::Init(CtpConfig& ctpconfig)
{
    account_id_ = ctpconfig.account_id;
    password_ = ctpconfig.password;
    broker_id_ = ctpconfig.broker_id;
    front_uri_ = ctpconfig.td_url;
    app_id_ = ctpconfig.app_id;
    auth_code_ = ctpconfig.auth_code;
    m_tdapi = CThostFtdcTraderApi::CreateFtdcTraderApi();
   
    m_tdapi->RegisterSpi(this);
    m_tdapi->SubscribePublicTopic(THOST_TERT_RESTART);
    m_tdapi->SubscribePrivateTopic(THOST_TERT_RESTART);
    m_tdapi->RegisterFront((char*)ctpconfig.td_url.c_str());
    std::unique_lock<std::mutex> lkLogin(m_muxLogin);
    m_tdapi->Init();
    if (m_cvLogin.wait_for(lkLogin, std::chrono::seconds(10)) == std::cv_status::timeout)
    {
        std::cout << "交易登陆超时"  << std::endl;
    }

    static const char* TdVersion = m_tdapi->GetApiVersion();
    const char* td_day = m_tdapi->GetTradingDay();
    std::cout << "TD Version:" << TdVersion << std::endl;
    std::cout << "TD TradingDay:" << td_day << std::endl;

  //  m_tdapi->Join();
}


void CtpTD::reqAuthenticate()
{
    CThostFtdcReqAuthenticateField a = { 0 };
    strncpy(a.BrokerID, broker_id_.c_str(), sizeof(TThostFtdcBrokerIDType));
    strncpy(a.UserID, account_id_.c_str(), sizeof(TThostFtdcUserIDType));
    strncpy(a.AuthCode, auth_code_.c_str(), sizeof(TThostFtdcAuthCodeType));
    strncpy(a.AppID, app_id_.c_str(), sizeof(TThostFtdcAppIDType));

    int ret = m_tdapi->ReqAuthenticate(&a, m_requestID++);

}
void CtpTD::OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{

    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "TD  穿透认证成功 Success:" << std::endl;
        reqUserlogin();
    }
    else {
        std::cout << "OnRspAuthenticate error:"
            << pRspInfo->ErrorID
            << pRspInfo->ErrorMsg
            << std::endl;
    }
}

void CtpTD::reqUserlogin()
{
    
    CThostFtdcReqUserLoginField Loginfield;
    memset(&Loginfield, 0, sizeof(CThostFtdcReqUserLoginField));
    strcpy(Loginfield.BrokerID, broker_id_.c_str());
    strcpy(Loginfield.UserID, account_id_.c_str());
    strcpy(Loginfield.Password, password_.c_str());
    int rt = m_tdapi->ReqUserLogin(&Loginfield, m_requestID++);
    if (!rt)
        std::cout << ">>>>>>发送登录请求成功" << std::endl;
    else
        std::cerr << "--->>>发送登录请求失败" << std::endl;
}
void CtpTD::setOrderInfo(CThostFtdcRspUserLoginField* pRspUserLogin)
{
    std::cout << "Trade Date " << pRspUserLogin->TradingDay << std::endl;
    std::cout << "Login Time " << pRspUserLogin->LoginTime << std::endl;
    std::cout << "Broker " << pRspUserLogin->BrokerID << std::endl;
    std::cout << "Account " << pRspUserLogin->UserID << std::endl;
    m_tradeFrontID = pRspUserLogin->FrontID;
    m_sessionID = pRspUserLogin->SessionID;
    memset(&m_orderActionReq, 0, sizeof(m_orderActionReq));
    strcpy(m_orderActionReq.BrokerID, broker_id_.c_str());
    strcpy(m_orderActionReq.InvestorID, account_id_.c_str());
    m_orderActionReq.FrontID = m_tradeFrontID;
    m_orderActionReq.SessionID = m_sessionID;
    m_orderActionReq.ActionFlag = THOST_FTDC_AF_Delete;
    m_maxOrderRef = atoi(pRspUserLogin->MaxOrderRef);
    m_maxOrderRef = 2000;
    memset(&m_positionReq, 0, sizeof(m_positionReq));
    strcpy(m_positionReq.BrokerID, broker_id_.c_str());
    strcpy(m_positionReq.InvestorID, account_id_.c_str());
    memset(&m_orderInsertReq, 0, sizeof(m_orderInsertReq));
    strcpy(m_orderInsertReq.BrokerID, broker_id_.c_str());
    strcpy(m_orderInsertReq.InvestorID, account_id_.c_str());
    m_orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    m_orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    m_orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
    m_orderInsertReq.MinVolume = 1;
    m_orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
    m_orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    m_orderInsertReq.IsAutoSuspend = 0;
    m_orderInsertReq.UserForceClose = 0;
    std::cout << "===================Query all=============" << std::endl; //OK
}
void CtpTD::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    std::unique_lock<std::mutex> lk(m_muxLogin);
    m_cvLogin.notify_one();
    lk.unlock();
    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "登陆请求响应成功" << std::endl;
        tradingDate = m_tdapi->GetTradingDay();
        std::cout << "TradingDay:" << tradingDate.c_str() << std::endl;
        std::cout << "Login Success:" << std::endl;
        std::cout << "FrontID:" << pRspUserLogin->FrontID << std::endl;
        std::cout << "SessionID:" << pRspUserLogin->SessionID << std::endl;
        std::cout << "MaxOrderRef:" << pRspUserLogin->MaxOrderRef << std::endl;
        std::cout << "SHFETime:" << pRspUserLogin->SHFETime << std::endl;
        std::cout << "DCETime:" << pRspUserLogin->DCETime << std::endl;
        std::cout << "CZCETime:" << pRspUserLogin->CZCETime << std::endl;
        std::cout << "FFEXTime:" << pRspUserLogin->FFEXTime << std::endl;
        std::cout << "INETime:" << pRspUserLogin->INETime << std::endl;
        std::cout << "SystemName:" << pRspUserLogin->SystemName << std::endl;

        //初始化委托单配置
        setOrderInfo(pRspUserLogin);

        //向期货商第一次发送交易指令前，需要先查询投资者结算结果， 投资者确认以后才能交易。
        reqSettlementInfoConfirm();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    else {
        std::cout << "OnRspUserLogin error:"
            << pRspInfo->ErrorID
            << pRspInfo->ErrorMsg
            << std::endl;
    }
}

void CtpTD::reqSettlementInfoConfirm()
{
    CThostFtdcSettlementInfoConfirmField pSettlementInfoConfirm;
    memset(&pSettlementInfoConfirm, 0, sizeof(CThostFtdcSettlementInfoConfirmField));
    strcpy(pSettlementInfoConfirm.BrokerID, broker_id_.c_str());
    strcpy(pSettlementInfoConfirm.InvestorID, account_id_.c_str());
    std::unique_lock<std::mutex> lk(m_muxIns);
    int rtn = m_tdapi->ReqSettlementInfoConfirm(&pSettlementInfoConfirm, m_requestID++);
    if (rtn != 0)
    {
        std::cout << "投资者结算结果确认请求失败" << std::endl;
    }
}
void CtpTD::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "投资者结算确认响应成功" << std::endl;
        std::cout << "BrokerID:" << pSettlementInfoConfirm->BrokerID << std::endl;
        std::cout << "InvestorID:" << pSettlementInfoConfirm->InvestorID << std::endl;
        std::cout << "ConfirmDate:" << pSettlementInfoConfirm->ConfirmDate << std::endl;
        std::cout << "ConfirmTime:" << pSettlementInfoConfirm->ConfirmTime << std::endl;
        //查询资金账户
        //reqQueryTradingAccount();
        //查询所有可用合约信息
        //reqQryInstrument();
        //查询持仓
        reqQueryInvestorPosition(pSettlementInfoConfirm->BrokerID, pSettlementInfoConfirm->InvestorID);
    }
    else {
        std::cout << "投资者结算确认响应失败:" << pSettlementInfoConfirm << std::endl;
    }

}

void CtpTD::reqQueryTradingAccount()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    CThostFtdcQryTradingAccountField tradingAccountReq;
    memset(&tradingAccountReq, 0, sizeof(tradingAccountReq));
    strcpy(tradingAccountReq.BrokerID, broker_id_.c_str());
    strcpy(tradingAccountReq.InvestorID, account_id_.c_str());
    std::cout << "broker id: " << tradingAccountReq.BrokerID << std::endl;
    std::cout << "investor id: " << tradingAccountReq.InvestorID << std::endl;
 
    int rt = m_tdapi->ReqQryTradingAccount(&tradingAccountReq, m_requestID++);
    if (!rt)
        std::cout << ">>>>>>SPI Query Trading Account Successfully" << std::endl;
    else
        std::cerr << "--->>>SPI Query Trading Account Failure" << std::endl;
}
void CtpTD::OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    if (!isErrorRspInfo(pRspInfo)) {
        std::unique_lock<std::mutex> lkAcc(m_muxAccount);
        std::cout << "=====查询资金账户响应成功=====" << std::endl; //OK
        lkAcc.unlock();
        //查持仓
        reqQueryInvestorPosition(pTradingAccount->BrokerID, pTradingAccount->AccountID);
    }
}

void CtpTD::reqQryInstrument()
{
    CThostFtdcQryInstrumentField  InsField;
    memset(&InsField, 0, sizeof(CThostFtdcQryInstrumentField));

    std::unique_lock<std::mutex> lk(m_muxIns);
    m_listCode.clear();
    int rt = m_tdapi->ReqQryInstrument(&InsField, ++m_requestID);
    if (!rt)
        std::cout << ">>>>>>SPI Query Trading ReqQryInstrument Successfully" << std::endl;
    else
        std::cerr << "--->>>SPI Query Trading ReqQryInstrument Failure" << std::endl;
}
void CtpTD::OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    std::unique_lock<std::mutex> lk(m_muxIns);
    if (!isErrorRspInfo(pRspInfo)) {
        //std::cout << "====SPI Query Instrument Successfully=====" << bIsLast << std::endl; //OK
        //std::cout << "Instrument ID: " << pInstrument->InstrumentID << std::endl; // english, good
        //std::string info{ pInstrument->ProductID };
        // request to check investor's trading account
       
    }
    if (bIsLast)
    {
        std::cout << "=====查询合约响应成功=====" << std::endl; //OK
        reqQueryTradingAccount();
    }
        
    
}




void CtpTD::reqQueryInvestorPosition(const char* brokerID,const char * InvestorID)
{
    strcpy(m_positionReq.InstrumentID, brokerID);
    strcpy(m_positionReq.InvestorID, InvestorID);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    int rt = m_tdapi->ReqQryInvestorPosition(&m_positionReq, m_requestID++);
    if (!rt)
        std::cout << ">>>>>>Query Investor Position Successfully" << std::endl;
    else {
        std::cerr << "--->>>Query Investor Position Failure" << std::endl;
    }
}

bool CtpTD::isMyOrder(CThostFtdcOrderField* pOrder)
{

    return ((pOrder->FrontID == m_tradeFrontID) &&
        (pOrder->SessionID == m_sessionID) &&
        (strcmp(pOrder->OrderRef, m_orderRef) == 0));
}

bool CtpTD::isTradingOrder(CThostFtdcOrderField* pOrder)
{
    return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
        (pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
        (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

bool CtpTD::isMyTrader(std::vector<std::string> orderRefs, CThostFtdcTradeField* pTrade)
{
    m_mytrade = false;
    for (int i = 0; i < orderRefs.size(); i++) {
        // const char* c_s=orderRefs[i].c_str();
        std::string c_s = pTrade->OrderRef;
        if (c_s == orderRefs[i]) {
            m_mytrade = true;
            break;
        }
        else
            m_mytrade = false;
    }
    return m_mytrade;
}

bool CtpTD::isErrorRspInfo(CThostFtdcRspInfoField* pRspInfo)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    if (bResult) {
       
        getTime();
        std::cerr << "Return Error--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
        
        m_log << m_currentTime << " Return Error " << "ErrorID= " << pRspInfo->ErrorID << " ErrorMsg " << pRspInfo->ErrorMsg << std::endl;
    }
    return bResult;
}

void CtpTD::getTime()
{
    time_t now = time(0);
    tm* ltm = localtime(&now);

    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", ltm);
    sprintf(m_currentTime, "%s:%ld", buffer, ltm->tm_sec* 1000000);
}

void CtpTD::TradetLog(const char* FilePath)
{
    m_log.open(FilePath, std::ofstream::out | std::ofstream::app);
}



void CtpTD::GetMainContractInfo(const char* configPath)
{
   
    std::ifstream inputContract(configPath);
    std::string head;
    getline(inputContract, head);
    std::string product;
    std::string contract;
    std::string line;
    int i = 0;
    while (true) {
        getline(inputContract, line);
        if (inputContract.fail()) break;
        std::stringstream ss(line);
        ss >> product >> contract;
        m_instrumentID[i] = new char[contract.size() + 1];
        strcpy(m_instrumentID[i], contract.c_str());
        m_yesterdayLong[contract] = 0;
        m_yesterdayShort[contract] = 0;
        m_todayLong[contract] = 0;
        m_todayShort[contract] = 0;
        m_activeOrder[contract] = NULL; // active order of each instrument
        m_sentOrder[contract] = NULL; // sent order of each instrument
        m_cancelCount[contract] = 0;
        i++;
    }
    m_instrumentNum = i;
    inputContract.close();
}

void CtpTD::GetCancelFile(const char* configPath)
{
    std::string head;
    std::string line;
    std::ifstream inputCancel(configPath);
    getline(inputCancel, head);
    while (true) {
        getline(inputCancel, line);
        if (inputCancel.fail()) break;
        std::stringstream ss(line);
        std::string contract;
        int count;
        ss >> contract >> count;
        m_cancelCount[contract] = count;
    }
    inputCancel.close();

   
    
}

void CtpTD::reqOrderAction(CThostFtdcOrderField* pOrder)
{
    strcpy(m_orderActionReq.OrderRef, pOrder->OrderRef);
    strcpy(m_orderActionReq.InstrumentID, pOrder->InstrumentID);
    strcpy(m_orderActionReq.ExchangeID, pOrder->ExchangeID);
    int rt = m_tdapi->ReqOrderAction(&m_orderActionReq, m_requestID++);
    getTime();
    m_log << m_currentTime << " cancel " << pOrder->OrderRef << std::endl;
    std::cout << m_currentTime << " cancel " << pOrder->OrderRef << std::endl;
}

void CtpTD::reqOrderInsert(Orders_s& orderData)
{
    if (m_activeOrder[orderData.strCode] != NULL) 
        return;
    CThostFtdcInputOrderField orderField;
    memset(&orderField, 0, sizeof(orderField));
    //经纪公司代码
    strcpy(orderField.BrokerID, broker_id_.c_str());
    //投资者代码
    strcpy(orderField.InvestorID, account_id_.c_str());
    std::string strMarket;
    //小写转大写
    transform(orderData.strMarket.begin(), orderData.strMarket.end(), back_inserter(strMarket), ::toupper);
    strcpy(orderField.ExchangeID, strMarket.c_str());
    //合约代码
    strcpy(orderField.InstrumentID, orderData.strCode.c_str());
    //报单引用
    snprintf(m_orderRef, sizeof(m_orderRef), "%d", m_maxOrderRef++); // order reference is character array
    strcpy(orderField.OrderRef, m_orderRef);
    //报单价格条件
    if ((orderData.strMarket == "shfe") || (orderData.strMarket == "ine"))
    {
        orderField.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    }
    else
    {
        if (orderData.ePriceType == MarketPrice)
        {
            if (orderData.strMarket == "cffex")
            {
                orderField.OrderPriceType = THOST_FTDC_OPT_FiveLevelPrice;
            }
            else
            {
                orderField.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
            }
        }
        else
        {
            orderField.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
        }
    }
    //买卖方向: 
    //卖
    if (orderData.eDirection == DIREC_Sell)
    {
        orderField.Direction = THOST_FTDC_D_Sell;
    }
    //买
    else if (orderData.eDirection == DIREC_Buy)
    {
        orderField.Direction = THOST_FTDC_D_Buy;
    }
    //组合开平标志: 开平仓
    if (orderData.eOffSetFlag == OFFSET_Open)
    {
        orderField.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    }
    else
    { //平
        if ((orderData.strMarket == "shfe") || (orderData.strMarket == "ine"))
        {
            //shfe、ine 区分平今平昨
            if (orderData.eOffSetFlag == OFFSET_CloseToday)
            {
                orderField.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
            }
            else if (orderData.eOffSetFlag == OFFSET_CloseYestoday || orderData.eOffSetFlag == OFFSET_Close)
            {
                orderField.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
            }
        }
        else
        {
            orderField.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
        }
    }
    //组合投机套保标志
    if (orderData.eHedgeFlage == HedgeFlag_Spe)
    {
        orderField.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    }
    else if (orderData.eHedgeFlage == HedgeFlag_Arbitrage)
    {
        orderField.CombHedgeFlag[0] = THOST_FTDC_HF_Arbitrage;
    }
    else if (orderData.eHedgeFlage == HedgeFlag_Hedge)
    {
        orderField.CombHedgeFlag[0] = THOST_FTDC_HF_Hedge;
    }
    else if (orderData.eHedgeFlage == HedgeFlag_MarketMaker)
    {
        orderField.CombHedgeFlag[0] = THOST_FTDC_HF_MarketMaker;
    }
    //价格
    orderField.LimitPrice = orderData.dbPrice;
    //数量：
    orderField.VolumeTotalOriginal = orderData.intVolume;
    //有效期类型: 当日有效
    orderField.TimeCondition = THOST_FTDC_TC_GFD;
    //成交量类型: 任何数量
    orderField.VolumeCondition = THOST_FTDC_VC_AV;
    //触发条件: 立即
    orderField.ContingentCondition = THOST_FTDC_CC_Immediately;
    //强平原因: 非强平
    orderField.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;	//强平原因:非强平
    //自动挂起标志: 否
    orderField.IsAutoSuspend = 0;  //自动挂起标志:否
    orderField.MinVolume = 1;	//最小成交量:1
    ///用户强评标志: 否
    orderField.UserForceClose = 0;
   
    m_remainQty[orderData.strCode] = orderData.intVolume;
    m_log << m_currentTime << " set remain qty of " << orderData.strCode << " to " << m_remainQty[orderData.strCode]<< std::endl;
    int rt = m_tdapi->ReqOrderInsert(&orderField, ++m_requestID);
    if (!rt)
    {
        std::cout << ">>>>>>发送报单录入请求成功" << std::endl;
        std::cout << "组合投机套报标记：" << orderField.CombHedgeFlag << " 组合开平标志：" << orderField.CombOffsetFlag <<
            " 触发条件：" << orderField.ContingentCondition << " 买卖方向 " << orderField.Direction <<
            " 交易所代码：" << orderField.ExchangeID << "合约代码：" << orderField.InstrumentID <<
            " 价格：" << orderField.LimitPrice << " 报单价格条件：" << orderField.OrderPriceType <<
            " 报单引用：" << orderField.OrderRef << " 有效期类型：" << orderField.TimeCondition <<
            " 成交量类型：" << orderField.VolumeCondition << " 数量：" << orderField.VolumeTotalOriginal << std::endl;
    }
       
    else
        std::cerr << "--->>>发送报单录入请求失败" << std::endl;
    getTime();
    
    CThostFtdcOrderField* pOrder = new CThostFtdcOrderField;
    strcpy(pOrder->InstrumentID, orderField.InstrumentID);
    strcpy(pOrder->OrderRef, orderField.OrderRef);
    strcpy(pOrder->ExchangeID, orderField.ExchangeID);
    pOrder->VolumeTotalOriginal = orderField.VolumeTotalOriginal;
    pOrder->LimitPrice = orderField.LimitPrice;
    pOrder->Direction = orderField.Direction;
    pOrder->CombOffsetFlag[0] = orderField.CombOffsetFlag[0];
    //cout << instrumentID << " send order  " << endl;
    
    
    std::unique_lock<std::mutex> lock(m_muxOrder);
    m_sentOrder[orderField.InstrumentID] = pOrder; // we have sent this order, including open and close
    m_remainQty[orderField.InstrumentID] = orderField.VolumeTotalOriginal;
    char* c = const_cast<char*>(m_orderRef);
    std::string mid_orderRefs(c);
    orderRefs.push_back(mid_orderRefs);
    lock.unlock();
    m_log << m_currentTime << " send " << m_orderRef << " " << orderField.InstrumentID << " " << orderField.VolumeTotalOriginal << " " <<
        orderField.VolumeTotalOriginal<< " " << orderField.Direction << " " << orderField.CombOffsetFlag[0] << " " <<
        orderField.TimeCondition << std::endl;
    m_log << m_currentTime << " get remain qty of " << orderField.InstrumentID << " to " << m_remainQty[orderField.InstrumentID] << " " <<std::endl;
    std::cout << m_currentTime << " send " << m_orderRef << " " << orderField.InstrumentID << " " << orderField.VolumeTotalOriginal << " " <<
        orderField.VolumeTotalOriginal << " " << orderField.Direction << " " << orderField.CombOffsetFlag[0] << " " <<
        orderField.TimeCondition << std::endl;
}
