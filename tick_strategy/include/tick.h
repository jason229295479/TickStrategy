
#ifndef TICK_H
#define TICK_H
#include <iostream>
#include <cstring>

class BasicTick {
public:
	BasicTick() {
		m_price = 0.;
		m_qty = 0;
		m_turnover = 0.;
		m_cumOpenint = 0.;
		m_yp = 0.;
		m_bid = 0.;
		m_ask = 0.;
		m_bidQty = 0;
		m_askQty = 0;
	}
	BasicTick(const BasicTick& bt) {
		strcpy(m_date, bt.m_date);
		strcpy(m_time, bt.m_time);
		m_milli = bt.m_milli;
		m_price = bt.m_price;
		m_qty = bt.m_qty;
		m_turnover = bt.m_turnover;
		m_cumOpenint = bt.m_cumOpenint;
		m_yp = bt.m_yp;
		m_bid = bt.m_bid;
		m_ask = bt.m_ask;
		m_bidQty = bt.m_bidQty;
		m_askQty = bt.m_askQty;
	}
	BasicTick(char date[9], char time[9], int milli, double price, int qty, double turnover, double cumOpenint, double yp, double bid, double ask, int bidQty, int askQty) :m_milli(milli),
		m_price(price), m_qty(qty), m_turnover(turnover), m_cumOpenint(cumOpenint), m_yp(yp), m_bid(bid), m_ask(ask), m_bidQty(bidQty),
		m_askQty(askQty) {
		memset(&m_date, 0, sizeof(m_date));
		strcpy(m_date, date);
		memset(&m_time, 0, sizeof(m_time));
		strcpy(m_time, time);
		if (bid == 0) m_bid = ask;
		if (ask == 0) m_ask = bid;
	}
	BasicTick(std::string date, std::string time, int milli, double price, int qty, double turnover, double cumOpenint, double yp, double bid, double ask, int bidQty, int askQty) :m_milli(milli), m_price(price), m_qty(qty), m_turnover(turnover), m_cumOpenint(cumOpenint), m_yp(yp), m_bid(bid), m_ask(ask), m_bidQty(bidQty), m_askQty(askQty) {
		strcpy(m_date, date.c_str()); // set broker ID
		strcpy(m_time, time.c_str());
		if (bid == 0) m_bid = ask;
		if (ask == 0) m_ask = bid;
	}
public:
	char m_date[9];
	char m_time[9];
	int m_milli;
	double m_price;
	int m_qty;
	double m_turnover;
	double m_cumOpenint;
	double m_yp;
	double m_bid;
	double m_ask;
	int m_bidQty;
	int m_askQty;



};

class ExtendedTick : public BasicTick {
public:
	ExtendedTick() {};
	~ExtendedTick() {}
	
	ExtendedTick(const ExtendedTick& et) : BasicTick(et) {
		m_mid = et.m_mid;
		m_buyTrade = et.m_buyTrade;
		m_sellTrade = et.m_sellTrade;
		m_buy2Trade = et.m_buy2Trade;
		m_sell2Trade = et.m_sell2Trade;
	}
	ExtendedTick(char date[9], char time[9], int milli, double price, int qty, double turnover, double cumOpenint, double yp, double bid, double ask, int bidQty, int askQty) :
		BasicTick(date, time, milli, price, qty, turnover, cumOpenint, yp, bid, ask, bidQty, askQty), m_buyTrade(0), m_sellTrade(0), m_buy2Trade(0), m_sell2Trade(0) {}

	ExtendedTick(std::string date, std::string time, int milli, double price, int qty, double turnover, double cumOpenint, double yp, double bid, double ask, int bidQty, int askQty) :
		BasicTick(date, time, milli, price, qty, turnover, cumOpenint, yp, bid, ask, bidQty, askQty), m_buyTrade(0), m_sellTrade(0), m_buy2Trade(0), m_sell2Trade(0) {}

	ExtendedTick(const BasicTick& base) :BasicTick(base),
		m_buyTrade(0), m_sellTrade(0), m_buy2Trade(0), m_sell2Trade(0) {}

public:
	double m_mid;
	double m_buyTrade;// estimated active buy volume at best ask
	double m_sellTrade;// estimated active sell volume at best bid
	double m_buy2Trade;// estimated active buy volume at higher ask
	double m_sell2Trade;//  estimated active sell volume at lower bid
};



#endif // !TICK_H

