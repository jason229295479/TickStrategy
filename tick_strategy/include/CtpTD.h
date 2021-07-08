#pragma once
#ifndef CTP_TD_H
#define CTP_TD_H
#include "ThostFtdcTraderApi.h"
#include "GenData.h"
#include <mutex>
#include <fstream>
#include <time.h>
#include <ctime>
#include <thread>
#include <map>
#include <iostream>
#include <algorithm>
#include <list>
# include <sstream>
#include <condition_variable>
#include <cstring>
#include <string.h>
/*
派生行情类
继承CThostFtdcTraderSpi实现自己的行情回调类CtpTD
*/

const int MAX_INSTRUMENT = 100;
class CtpTD : public CThostFtdcTraderSpi {
public:
	CtpTD();
	~CtpTD();

	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected();

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///        0x1001 网络读失败
	///        0x1002 网络写失败
	///        0x2001 接收心跳超时
	///        0x2002 发送心跳失败
	///        0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason);

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse);

	///客户端认证响应
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, 
		CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, 
		CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	///投资者结算结果确认响应
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, 
		CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, 
		CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID,
		bool bIsLast);
	///请求查询合约响应
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, 
		CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	///请求查询资金账户响应
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount,
		CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	///请求查询投资者持仓响应
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, 
		CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	/////报单录入请求响应
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, 
		CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
	
	//委托报单通知
	virtual void OnRtnOrder(CThostFtdcOrderField* pOrder);

	//成交通知
	virtual void OnRtnTrade(CThostFtdcTradeField* pTrade);
public:
	void GetMainContractInfo(const char* configPath);
	void GetCancelFile(const char* configPath);
	//撤单
	void reqOrderAction(CThostFtdcOrderField* pOrder);
	//报单
	void reqOrderInsert(Orders_s& orderData);
	
public:
	void Init(CtpConfig& ctpconfig);
	void TradetLog(const char* FilePath);
public:
	std::string front_uri_;
	std::string broker_id_;
	std::string account_id_;
	std::string password_;
	std::string app_id_;
	std::string auth_code_;
	std::string tradingDate; //交易日
	std::map<std::string, int> m_sentCancel;
	std::map<std::string, int> m_cancelCount; // count the cancelation number of each instrument
	std::map<std::string, int> m_yesterdayLong; // yesterday long position of each instrument
	std::map<std::string, int> m_yesterdayShort; // yesterday short position of each instrument
	std::map<std::string, int> m_todayLong; // today long position of each instrument
	std::map<std::string, int> m_todayShort; // today short position of each instrument
	int m_instrumentNum=0; // number of instruments
	char* m_instrumentID[MAX_INSTRUMENT]; // to send subscribe;
	std::vector<std::string> orderRefs;
	std::map<std::string, CThostFtdcOrderField*> m_activeOrder; /// current active(not canceled or fully traded) order of each instrument
	std::map<std::string, CThostFtdcOrderField*> m_sentOrder; /// current active(not canceled or fully traded) order of each instrument
	std::map<std::string, int> m_count; // counting of ticks of each instrument
	
	std::map<std::string, int> m_remainQty; // remaining qty of each instrument 合约对应的委托数量
	bool m_finishPos = false; // whether finish reading historical position
	bool m_mytrade=false;
private:
	
	TThostFtdcOrderRefType	m_orderRef;
	CThostFtdcQryInvestorPositionField m_positionReq; // request for position
	CThostFtdcInputOrderActionField m_orderActionReq; // this is used to cancel an order
	CThostFtdcInputOrderField m_orderInsertReq;
	TThostFtdcFrontIDType	m_tradeFrontID;
	TThostFtdcSessionIDType	m_sessionID;
private:
	std::fstream m_log;
	CThostFtdcTraderApi* m_tdapi = nullptr;

	int m_requestID;
	bool m_loginFlag = false;
	int m_maxOrderRef; // the current maximum order reference
	char m_currentTime[87] = {0}; // current time

	std::list<CodeInfo_s> m_listCode;
	//穿透式认证请求
	void reqAuthenticate();
	//登陆
	void reqUserlogin();
	//初始化委托配置
	void setOrderInfo(CThostFtdcRspUserLoginField* pRspUserLogin);

	//查询投资者结算结果请求
	void reqSettlementInfoConfirm();
	//请求查询资金账户
	void reqQueryTradingAccount();
	//请求查询合约，填空可以查询到所有合约。
	void reqQryInstrument();
	
	void reqQueryInvestorPosition(const char* brokerID, const char* InvestorID);
	bool isMyOrder(CThostFtdcOrderField* pOrder);
	bool isTradingOrder(CThostFtdcOrderField* pOrder);
	bool isMyTrader(std::vector<std::string> orderRefs, CThostFtdcTradeField* pTrade);
	//错误信息
	bool isErrorRspInfo(CThostFtdcRspInfoField* pRspInfo);
	void getTime();

	
	
	//互斥量 锁
	std::mutex m_muxLogin;   
	std::mutex m_muxReq;
	std::mutex m_muxAccount;
	std::mutex m_muxIns;
	std::mutex m_muxPos;
	std::mutex m_muxTrade;
	std::mutex m_muxOrder;
	std::condition_variable m_cvLogin;
};


#endif // !CTP_TD_H
