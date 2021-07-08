#pragma once
#ifndef GEN_DATA_H
#define GEN_DATA_H
#include <string>
#include <vector>

const int TICK_NUM = 66606; // maximum number fo ticks per day, for example the au and ag reaches this number theoretically
struct SysConfig
{
	
	std::string tickData_file;
	std::string tickModelConfig_file;
	std::string mainContract_file;
	std::string productInfo_file;
	std::string productExchange_file;
	std::string cancel_file;
	std::string outputIndicator_file;
	std::string pred_file;
	std::string tradeLog_file;
	std::string mdLog_file;
	
};
struct CtpConfig
{
	std::string account_id;
	std::string password;
	std::string broker_id;
	std::string td_url;
	std::string md_url;
	std::string app_id;
	std::string auth_code;
	std::string orderType;
};

struct ContractInfo
{	//品种
	std::string Product;
	//合约代码
	std::string Contract;
	//日内
	std::string Intraday;
	//交易所
	std::string ExchangeID;
	//合约数量乘数
	int VolumeMultiple;
	///最小变动价位
	double PriceTick;
	//自定义倍数
	int Factor;
	//夜晚收盘时间
	std::string NightEnd;
	//下午收盘时间
	std::string Afternoon;
	//收盘时间
	std::string ClearTime;
	

	ContractInfo()
		: VolumeMultiple(0)
		, PriceTick(0)
		, Factor(0)
	{}
	bool operator == (const ContractInfo& src) const
	{
		if (Product == src.Product)
		{
			return true;
		}
		return false;
	}
	bool operator != (const ContractInfo& src) const
	{
		if (*this == src)
		{
			return false;
		}
		return true;
	}
};



struct PGSqlConfig
{
	//数据库 参数
	std::string dbname;
	std::string dbhostaddr;
	std::string dbuser;
	std::string dbpassword;
	std::string dbport;
};
enum CodeType_e
{

	FUT_INDEX = 0, //期货指数
	FUTURE = 1, //普通合约
	FUT_MAIN = 2, //主力
	A_STOCK = 3, //A股
	B_STOCK = 4, //B股
	STOCK_INDEX = 5, //股票指数

};
//合约信息
struct CodeInfo_s
{
	std::string strCode; //合约
	std::string strMarket; //市场
	std::string strName; //合约名称
	std::string strShortName; //简称
	std::string strLastDay; //退市日期
	std::string strListDay; //上市日期
	std::string strRoot; //品种
	std::string strRootName; //品种名称
	std::string strType;//类型
	double dbMult; //合约乘数
	double dbUnit; //最小变动单位
	CodeType_e eType; //类型
	CodeInfo_s()
		: dbMult(0)
		, dbUnit(0)
	{}
};
struct RtData_s
{
	///交易日
	std::string strDate; //0
	///交易所代码
	std::string strCode; //1
	///交易所
	std::string strMarket; //2
	///最新价
	double dbLastPrice; //3
	///上次结算价
	double dbPreSettlePrice;//4
	///昨收盘
	double dbPreClosePrice; //5
	///昨持仓量
	double dbPreOpenInterest; //6
	///今开盘
	double dbOpenPrice;
	///最高价
	double dbHighestPrice;
	///最低价
	double dbLowestPrice;
	///数量
	double dbVolume; //10
	///数量
	double dbVolumeTick;
	///成交金额
	double dbTurnover;
	///持仓量
	double dbOpenInterest; //13
	///今收盘
	double dbClosePrice;
	///本次结算价
	double dbSettlePrice;
	///涨停板价
	double dbUpperLimitPrice;
	///跌停板价
	double dbLowerLimitPrice;
	///昨虚实度
	double dbPreDelta;
	///今虚实度
	double dbCurDelta;
	///最后修改时间
	std::string UpdateTime;
	//最后修改毫秒
	int iUpdateMillisec;
	///申买价
	std::vector<double> vecBidPrice;
	///申买量
	std::vector<double> vecBidVolume;
	//申卖价
	std::vector<double> vecAskPrice;
	///申卖量
	std::vector<double> vecAskVolume; //25
	///当日均价
	double dbAveragePrice;
	///业务日期
	std::string strActionDay;


	RtData_s()
		: dbLastPrice(0)
		, dbPreSettlePrice(0)
		, dbPreClosePrice(0)
		, dbPreOpenInterest(0)
		, dbOpenPrice(0)
		, dbHighestPrice(0)
		, dbLowestPrice(0)
		, dbVolume(0)
		, dbVolumeTick(0)
		, dbTurnover(0)
		, dbOpenInterest(0)
		, dbClosePrice(0)
		, dbSettlePrice(0)
		, dbUpperLimitPrice(0)
		, dbLowerLimitPrice(0)
		, dbPreDelta(0)
		, dbCurDelta(0)
		, iUpdateMillisec(0)
		, dbAveragePrice(0)
	{
		vecBidPrice.resize(5);
		vecBidVolume.resize(5);
		vecAskPrice.resize(5);
		vecAskVolume.resize(5);
	}

	bool operator == (const RtData_s& rtData) const
	{
		if (dbSettlePrice == rtData.dbSettlePrice && dbVolume == rtData.dbVolume
			&& dbOpenInterest == rtData.dbOpenInterest && dbLastPrice == rtData.dbLastPrice && dbTurnover == rtData.dbTurnover &&
			vecAskPrice[0] == rtData.vecAskPrice[0] && vecAskVolume[0] == rtData.vecAskVolume[0] &&
			vecBidPrice[0] == rtData.vecBidPrice[0] && vecBidVolume[0] == rtData.vecBidVolume[0])
		{
			return true;
		}
		return false;
	}
};
/*
//声明且初始化指标结构体
struct IndicatorPara {
	int period;
	double lambda;
	double threshold;
	double shortLambda;
	bool adjust;

	IndicatorPara()
		:period(0)
		, lambda(0)
		, threshold(0)
		, shortLambda(0)
		, adjust(false)
	{}
};
*/


struct time_v
{
	std::string p_date;
	std::string p_time;
	double ask;
	double askqty;
	double bid;
	double bidqty;
	double turnover;
	double qty;
	double buytrade;
	double selltrade;
	double buy2trade;
	double sell2trade;
	double price;
	double resulthigh;
	double resultlow;
	double inthigh;
	double intlow;
	double aa;
	time_v()
		:ask(0)
		,askqty(0)
		,bid(0)
		,bidqty(0)
		,turnover(0)
		,qty(0)
		,buytrade(0)
		,selltrade(0)
		,buy2trade(0)
		,sell2trade(0)
		,price(0)
		,resulthigh(0)
		,resultlow(0)
		,inthigh(0)
		,intlow(0)
		,aa(0)
	{}
};


struct time_pre_value
{
	double prediction_value;
	std::string p_date;
	std::string p_time;
	double ask;
	double askqty;
	double bid;
	double bidqty;
	double turnover;
	double qty;
	double buytrade;
	double selltrade;
	double buy2trade;
	double sell2trade;
	double price;
	double resulthigh;
	double resultlow;
	double inthigh;
	double intlow;
	double resultlow2;
	double resulthigh2;
	time_pre_value()
		:prediction_value(0)
		,ask(0)
		,askqty(0)
		,bid(0)
		,bidqty(0)
		,turnover(0)
		,qty(0)
		,buytrade(0)
		,selltrade(0)
		,buy2trade(0)
		,sell2trade(0)
		,price(0)
		,resulthigh(0)
		,resultlow(0)
		,inthigh(0)
		,intlow(0)
		,resultlow2(0)
		,resulthigh2(0)
	{}

};


struct TradeThre {
	double open;
	double close;
	int qty;
	TradeThre()
		:open(0)
		,close(0)
		,qty(0)
	{}
};

//买卖方向
enum Direction_e
{
	DIREC_NA = 0,
	DIREC_Buy=1,  //买
	DIREC_Sell=2 //卖
};
//开平
enum OffSetFlag_e
{
	OFFSET_NA = 0,
	OFFSET_Open = 1, //开
	OFFSET_Close = 2, //平
	OFFSET_CloseYestoday = 3, //平昨
	OFFSET_CloseToday = 4, //平今
	OFFSET_ForceOff = 5, //强减
	OFFSET_LOCALForceClose = 6 //本地强平
};

enum HedgeFlag_e
{
	HedgeFlag_Spe = 0, //投机
	HedgeFlag_Arbitrage = 2,//套利
	HedgeFlag_Hedge = 3,//套保
	HedgeFlag_MarketMaker = 4 //做市商
};

enum OrderPriceType_e
{
	MarketPrice = 0, //市价
	LimitPrice = 1, //限价
	LastPrice = 2 //最新价
};

enum OrderSubmitStatus_e
{
	//已经提交
	Order_InsertSubmitted = 0,
	//撤单已经提交
	Order_CancelSubmitted = 1,
	//修改已经已经
	Order_ModifySubmitted = 2,
	//已经接受
	Order_Accepted = 3,
	//报单被拒绝
	Order_InsertRejuected = 4,
	//撤单被拒绝
	Order_CancelRejuected = 5,
	//改单被拒绝
	Order_ModifyRejuected = 6
};

enum OrderStatus_e
{
	//全部成交
	Order_ALLTraded = 0,
	//部分成交还在队列中
	Order_PartTradedQueueing = 1,
	///部分成交不在队列中
	Order_PartTradedNotQueueing = 2,
	//未成交还在队列中
	Order_NoTradeQueueing = 3,
	//未成交不在队列中
	Order_NoTradeNotQueueing = 4,
	//撤单
	Order_Canceled = 5,
	//成交
	Order_Unknown = 6,
	//尚未触发
	Order_NotTouched = 7,
	//已触发
	Order_Touched = 8,
	//报单被拒绝
	Order_Reject = 9,
	//部分成交
	Order_PartTrade = 10
};
enum OrderForceType_e
{
	Force_Not = 0, //非强平
	Force_LackDeposit = 1 //资金不足
};
//有效期类型
enum TimeConditionType_e
{
	///立即完成，否则撤销
	FTDC_TC_IOC = 1,
	///本节有效
	FTDC_TC_GFS = 2,
	///当日有效
	FTDC_TC_GFD = 3,
	///指定日期前有效
	FTDC_TC_GTD = 4,
	///撤销前有效
	FTDC_TC_GTC = 5,
	///集合竞价有效
	FTDC_TC_GFA = 6
};
//委托单
struct Orders_s
{
	
	//合约
	std::string strCode;
	//交易所
	std::string strMarket;
	//报单编号（第三方返回）
	std::string strOrderThirdID;
	//报单引用
	std::string strOrderRef;
	//买卖
	Direction_e eDirection;
	//开平
	OffSetFlag_e eOffSetFlag;
	//投保
	HedgeFlag_e eHedgeFlage;
	//价格类型
	OrderPriceType_e ePriceType;
	

	
	int intVolume; //委托数量

	//委托价格
	double dbPrice;
	//成交数量
	double dbTradeVolume;
	//交易日
	std::string strTradingDay;
	//报单日期
	std::string strInsertData;
	//委托时间
	std::string strInsertTime;
	//更新时间
	std::string strUpdateTime;
	//撤单时间
	std::string strCancelTime;
	//报单提交状态
	OrderSubmitStatus_e eSubmitStatus;
	//报单状态
	OrderStatus_e eStatus;
	//报单详情
	std::string strNotice;
	//报单来源
	std::string strOrderSource;
	//结算编号
	int64_t iSettlementID;
	//强平原因
	OrderForceType_e eForceClose;
	TimeConditionType_e eTimeConditionType;


		Orders_s()
		: dbPrice(0)
		, intVolume(0)
		, dbTradeVolume(0)
		, iSettlementID(0)
	{}
};



//修改tick数据

struct NewData_s
{
	std::string ActionDay;	//业务日期
	///最后修改时间
	std::string UpdateTime;
	//最后修改毫秒
	int UpdateMillisec;
	std::string StrCode;
	//最新价
	double LastPrice;
	//tick 当前成交量
	int NewVolume;
	//当前成交金额
	double NewTrunover;
	//持仓量
	double OpenInterest;
	//昨天收盘价
	double PreClosePrice;
	//申买价
	std::vector<double> BidPrice;
	//申买量
	std::vector<double> BidVolume;
	//申卖价
	std::vector<double> AskPrice;
	//申卖量
	std::vector<double> AskVolume; 

	double m_mid;
	double m_buyTrade;// estimated active buy volume at best ask 主动买1
	double m_sellTrade;// estimated active sell volume at best bid 主动卖1
	double m_buy2Trade;// estimated active buy volume at higher ask 主动买2
	double m_sell2Trade;//  estimated active sell volume at lower bid 主动卖2


	NewData_s()
		: UpdateMillisec(0)
		, LastPrice(0)
		, NewVolume(0)
		, NewTrunover(0)
		, OpenInterest(0)
		, PreClosePrice(0)
		, m_mid(0)
		, m_buyTrade(0)
		, m_sellTrade(0)
		, m_buy2Trade(0)
		, m_sell2Trade(0)
	{
		BidPrice.resize(5);
		BidVolume.resize(5);
		AskPrice.resize(5);
		AskVolume.resize(5);
	}
};


struct modeConfigFile
{
	std::string model = "model.txt";
	std::string position = "position.txt";
	std::string signal = "signal.txt";
	std::string threshold = "threshold.txt";
};
#endif // !GEN_DATA_H



