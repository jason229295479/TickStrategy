#pragma once
#ifndef CTP_MD_H
#define CTP_MD_H

#include "ThostFtdcMdApi.h"
#include "GenData.h"
#include "blockingconcurrentqueue.h"
#include "concurrentqueue.h"
#include <functional>
#include <iostream>
#include <vector>
#include <condition_variable>

#include <mutex>
#include <map>
class CtpMD :public CThostFtdcMdSpi
{
public:
	CtpMD();
	~CtpMD();
	void Init(CtpConfig& ctpconfig);
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
	virtual void OnHeartBeatWarning(int nTimeLapse) {};


	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);


	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///订阅行情应答
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, 
		int nRequestID, bool bIsLast);

	///取消订阅行情应答
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo,
		int nRequestID, bool bIsLast) {};

	///深度行情通知
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData);
public:
	//行情回调函数
	void CallBackDepthMarketData(std::function<void(RtData_s&)> func);
	
	inline void convertDepthMarketData(CThostFtdcDepthMarketDataField& DepthMarketData, RtData_s& rtData)
	{
		rtData.strDate = DepthMarketData.TradingDay;
		rtData.strCode = DepthMarketData.InstrumentID;
		rtData.strMarket = DepthMarketData.ExchangeID;
		rtData.dbLastPrice = checkDouble(DepthMarketData.LastPrice);
		rtData.dbPreSettlePrice = checkDouble(DepthMarketData.PreSettlementPrice);
		rtData.dbPreClosePrice = checkDouble(DepthMarketData.PreClosePrice);
		rtData.dbPreOpenInterest = checkDouble(DepthMarketData.PreOpenInterest);
		rtData.dbOpenPrice = checkDouble(DepthMarketData.OpenPrice);
		rtData.dbHighestPrice = checkDouble(DepthMarketData.HighestPrice);
		rtData.dbLowestPrice = checkDouble(DepthMarketData.LowestPrice);
		rtData.dbVolume = checkDouble(DepthMarketData.Volume);
		rtData.dbTurnover = checkDouble(DepthMarketData.Turnover);
		rtData.dbOpenInterest = checkDouble(DepthMarketData.OpenInterest);
		rtData.dbClosePrice = checkDouble(DepthMarketData.ClosePrice);
		rtData.dbSettlePrice = checkDouble(DepthMarketData.SettlementPrice);
		rtData.dbUpperLimitPrice = checkDouble(DepthMarketData.UpperLimitPrice);
		rtData.dbLowerLimitPrice = checkDouble(DepthMarketData.LowerLimitPrice);
		rtData.dbPreDelta = checkDouble(DepthMarketData.PreDelta);
		rtData.dbCurDelta = checkDouble(DepthMarketData.CurrDelta);
		rtData.UpdateTime = DepthMarketData.UpdateTime;
		rtData.iUpdateMillisec = DepthMarketData.UpdateMillisec;
		rtData.dbAveragePrice = checkDouble(DepthMarketData.AveragePrice);
		rtData.strActionDay = DepthMarketData.ActionDay;
		rtData.vecBidPrice.resize(5);
		rtData.vecAskPrice.resize(5);
		rtData.vecAskVolume.resize(5);
		rtData.vecBidVolume.resize(5);

		rtData.vecBidPrice[0] = checkDouble(DepthMarketData.BidPrice1);
		rtData.vecBidPrice[1] = checkDouble(DepthMarketData.BidPrice2);//0; //checkDouble(DepthMarketData.BidPrice2);
		rtData.vecBidPrice[2] = checkDouble(DepthMarketData.BidPrice3);//0; //checkDouble(DepthMarketData.BidPrice3);
		rtData.vecBidPrice[3] = checkDouble(DepthMarketData.BidPrice4); //0; //checkDouble(DepthMarketData.BidPrice4);
		rtData.vecBidPrice[4] = checkDouble(DepthMarketData.BidPrice5); //0; //checkDouble(DepthMarketData.BidPrice5);

		rtData.vecAskPrice[0] = checkDouble(DepthMarketData.AskPrice1);
		rtData.vecAskPrice[1] = checkDouble(DepthMarketData.AskPrice2); //0; //checkDouble(DepthMarketData.AskPrice2);
		rtData.vecAskPrice[2] = checkDouble(DepthMarketData.AskPrice3); //0; //checkDouble(DepthMarketData.AskPrice3);
		rtData.vecAskPrice[3] = checkDouble(DepthMarketData.AskPrice4); //0; //checkDouble(DepthMarketData.AskPrice4);
		rtData.vecAskPrice[4] = checkDouble(DepthMarketData.AskPrice5);// 0; //checkDouble(DepthMarketData.AskPrice5);

		rtData.vecAskVolume[0] = DepthMarketData.AskVolume1;
		rtData.vecAskVolume[1] = DepthMarketData.AskVolume2; //0; //DepthMarketData.AskVolume2;
		rtData.vecAskVolume[2] = DepthMarketData.AskVolume3;// 0; //DepthMarketData.AskVolume3;
		rtData.vecAskVolume[3] = DepthMarketData.AskVolume4;// 0; //DepthMarketData.AskVolume4;
		rtData.vecAskVolume[4] = DepthMarketData.AskVolume5;// 0; //DepthMarketData.AskVolume5;

		rtData.vecBidVolume[0] = DepthMarketData.BidVolume1;
		rtData.vecBidVolume[1] = DepthMarketData.BidVolume2; //DepthMarketData.BidVolume2;
		rtData.vecBidVolume[2] = DepthMarketData.BidVolume3; //DepthMarketData.BidVolume3;
		rtData.vecBidVolume[3] = DepthMarketData.BidVolume4; //DepthMarketData.BidVolume4;
		rtData.vecBidVolume[4] = DepthMarketData.BidVolume5; //DepthMarketData.BidVolume5;
	}
	inline double checkDouble(double dbValue)
	{
		if ((dbValue >= -1E-15 && dbValue <= 1E-15) || dbValue > 10E+15)
		{
			return 0;
		}
		return dbValue;
	}
	void SubMarketData(std::vector<std::string> &m_instrument);
private:
	
	CThostFtdcMdApi* m_mdapi=NULL;
	int m_requestID=0;

	bool login();
	std::mutex m_muxLogin;
	std::condition_variable m_cvLogin;
public:
	std::string front_uri_;
	std::string broker_id_;
	std::string account_id_;
	std::string password_;
	std::string app_id_;
	std::string auth_code_;
	std::function<void(RtData_s&)> m_funcDepthMarketData;
	std::map<std::string, std::string> m_mapMarket;
	char* m_instrumentID[100] = {0}; // to send subscribe;

};




#endif // !CTP_MD_H
