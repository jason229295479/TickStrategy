#include "CtpMD.h"
#include <cstring>
#include <string.h>
/*
派生行情类
继承CThostFtdcMdSpi实现自己的行情回调类CtpMD
*/
CtpMD::CtpMD()
{
	m_requestID = 1;
}

CtpMD::~CtpMD()
{
    if (m_mdapi) {
        m_mdapi->RegisterSpi(nullptr);
        m_mdapi->Release();
        m_mdapi = nullptr;
    }
}

void CtpMD::Init(CtpConfig& ctpconfig)
{
    m_mdapi  = CThostFtdcMdApi::CreateFtdcMdApi();
    m_mdapi->RegisterSpi(this);
    account_id_ = ctpconfig.account_id;
    broker_id_ = ctpconfig.broker_id;
    front_uri_ = ctpconfig.md_url;
    m_mdapi->RegisterFront((char*)ctpconfig.md_url.c_str());
    std::unique_lock<std::mutex> lkLogin(m_muxLogin);
    m_mdapi->Init();
    if (m_cvLogin.wait_for(lkLogin, std::chrono::seconds(10)) == std::cv_status::timeout)
    {
        std::cout << "登陆超时" << std::endl;
    }


    static const char* MdVersion = m_mdapi->GetApiVersion();
    const char* md_day = m_mdapi->GetTradingDay();
    std::cout << "MD Version:" << MdVersion<<std::endl;
    std::cout << "MD TradingDay:" << md_day<<std::endl;

  //  m_mdapi->Join();
}

void CtpMD::OnFrontConnected()
{
    std::cout << "MD OnFrontConnected..." << std::endl;
    if (login())
    {
        std::cout << "MD Login Success" << std::endl;
    }
    else
    {
        std::cout << "MD Login failed" << std::endl;
    }
}

void CtpMD::OnFrontDisconnected(int nReason)
{
    std::cout << " MD OnFrontDisconnected, nReason:" << nReason << std::endl;
}

void CtpMD::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    if (nullptr == pRspInfo || 0 == pRspInfo->ErrorID) {
        {
            std::cout << "MD OnRspUserLogin ok" << std::endl;

            //char* g_pInstrumentID[] = { (char*)"SR109", (char*)"rb2110", (char*)"cs2105", (char*)"CF109" }; // 行情合约代码列表，中、上、大、郑交易所各选一种
           // int instrumentNum = 4;

           // m_mdapi->SubscribeMarketData(g_pInstrumentID, instrumentNum);
            
        }

    }
    else {
        std::cout << "MD OnRspUserLogin error:"
            << pRspInfo->ErrorID
            << pRspInfo->ErrorMsg
            << std::endl;
    }
    std::unique_lock<std::mutex> lk(m_muxLogin);
    m_cvLogin.notify_one();
    lk.unlock();
}

void CtpMD::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    CThostFtdcUserLogoutField logoutField;
    memset(&logoutField, 0, sizeof(CThostFtdcUserLogoutField));
    strcpy(logoutField.BrokerID, broker_id_.c_str());
    strcpy(logoutField.UserID, account_id_.c_str());
    int iret = m_mdapi->ReqUserLogout(&logoutField, ++m_requestID);
    if (iret != 0)
    {
        std::cout << "MD ReqUserLogout error:"
            << pRspInfo->ErrorID
            << pRspInfo->ErrorMsg
            << std::endl;  
    }
    else
    {
        if (m_mdapi != nullptr)
        {
            m_mdapi->RegisterSpi(nullptr);
            m_mdapi->Release();
            m_mdapi = nullptr;
            std::cout << "MD Logout Success" << std::endl;
        }
    } 
    std::unique_lock<std::mutex> lk(m_muxLogin);
    m_cvLogin.notify_one();
    lk.unlock();
}

void CtpMD::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    std::cout << "MD OnRspError:"
        << pRspInfo->ErrorID
        << pRspInfo->ErrorMsg
        << std::endl;
}

void CtpMD::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
   if (nullptr == pRspInfo || 0 == pRspInfo->ErrorID) {
        //std::cout << "OnRspSubMarketData ok" << std::endl;
    }
    else {
        std::cout << "MD OnRspSubMarketData error:"
            << pRspInfo->ErrorID
            << pRspInfo->ErrorMsg
            << std::endl;
    }
}

void CtpMD::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
{
   /*
    std::cout << "OnRtnDepthMarketData--->"
        << pDepthMarketData->InstrumentID << ","
        << pDepthMarketData->ExchangeID << ","
        << pDepthMarketData->LastPrice << ","
        << pDepthMarketData->OpenPrice << ","
        << pDepthMarketData->HighestPrice << ","
        << pDepthMarketData->LowestPrice << ","
        << pDepthMarketData->Volume << ","
        << pDepthMarketData->Turnover << ","
        << pDepthMarketData->OpenInterest << ","
        << pDepthMarketData->ClosePrice << ","
        << pDepthMarketData->ActionDay << ","
        << pDepthMarketData->UpdateTime << ","
        << pDepthMarketData->UpdateMillisec << ","
        << pDepthMarketData->AskPrice1 << ","
        << pDepthMarketData->AskVolume1 << ","
        << pDepthMarketData->BidPrice1 << ","
        << pDepthMarketData->BidVolume1 << ","
        << std::endl;
     */   
    RtData_s rtData;
    convertDepthMarketData(*pDepthMarketData, rtData);
    m_funcDepthMarketData(rtData);
     
}

void CtpMD::CallBackDepthMarketData(std::function<void(RtData_s&)> func)
{
    m_funcDepthMarketData = func;
}



void CtpMD::SubMarketData(std::vector<std::string>& m_instrument)
{
    int i = 0;
    for (auto contract : m_instrument) {
        m_instrumentID[i] = new char[contract.size() + 1];
        strcpy(m_instrumentID[i], contract.c_str());
        i++;
    }
    int m_instrumentNum = i;

    int rt = m_mdapi->SubscribeMarketData(m_instrumentID, m_instrumentNum);
    if (!rt)
        std::cout << ">>>>>>request for market data success" << std::endl;
    else
        std::cerr << "--->>>request for market data fails" << std::endl;
    for (int i = 0; i != m_instrumentNum; ++i)
        delete[](m_instrumentID[i]);

}

bool CtpMD::login()
{
    CThostFtdcReqUserLoginField login_field = {};
    strncpy(login_field.TradingDay, "", sizeof(TThostFtdcDateType));
    strncpy(login_field.UserID, account_id_.c_str(), sizeof(TThostFtdcUserIDType));
    strncpy(login_field.BrokerID, broker_id_.c_str(), sizeof(TThostFtdcBrokerIDType));
    strncpy(login_field.Password, password_.c_str(), sizeof(sizeof(TThostFtdcPasswordType)));
    int rtn = m_mdapi->ReqUserLogin(&login_field, m_requestID++);
    if (rtn != 0)
    {
        std::cout << "MD failed to request login..." << std::endl;
        return false;
    }
    else
    {
        return true;
    }
}
