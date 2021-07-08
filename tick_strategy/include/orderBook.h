#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <string> // here we also use string, and it would only be defined onece because it also has macro
#include <vector>
#include <iostream>
#include <string>
#include <map>
#include <tr1/memory>
#include <time.h>
#include "tick.h" // here we use another header defined by us
#include "GenData.h"
#include <math.h>


typedef std::vector<ExtendedTick> ExtendedVec; // define the name of a new type, laterwe can use ExtendedVec directly
typedef std::vector<ExtendedTick>::iterator ExtendedItr; // define the iterator of the vector

//const int TICK_NUM = 66606; // maximum number fo ticks per day, for example the au and ag reaches this number theoretically



typedef std::vector<time_v> Time_value;
typedef std::map<std::string, Time_value> Time_value_map;
// save the order book information 
class OrderBook {
public:
	
	OrderBook(const std::string& path, const std::string& product,
		int multiplier, double spread, int intFactor) : m_path(path), m_product(product),
		m_multiplier(multiplier), m_spread(spread),
		m_intFactor(intFactor) { // initialize the parameters
		
	};// construct order book from a file
	~OrderBook() {}
	void setup(); // function to setup an object
	void addTick(ExtendedTick& newTick);
	void addTickData(NewData_s& newTickData);
	void setTradeTick(std::vector<NewData_s>::iterator lastTick);
	void setTrade(ExtendedItr tick); // calculate the active buy and sell volume of one tick
	
	void output();
	void outputorderbook();
	
public:
	std::vector<ExtendedTick> m_book; // store the tick values
	std::vector<NewData_s> m_vbook; // store the tick values
	std::string m_date; // date of the data file
	std::string m_path; // path of the file
	std::string m_product; // product of the file

	bool m_realStart;
	char m_nightEnd[9];
	char m_afternoon[9];
	int m_multiplier; // multiplier of the contract 合约乘数
	double m_spread; // spread of the contract
	int m_intFactor; // if it's already an integer than m_intFactor is 1; for j it's 10, and for au it's 100;
	int m_intSpread; // when the price is changed to integer, the minimum spread of it
	bool m_endSession; // end of a session
	bool m_finish; // finish subscribe
	double m_cumTurnover; // cumulative turnover at the end of history
	int m_cumQty; // cumulative volume at the end of history
	time_t m_machineTime;
	int m_count;
	std::string time_value_date;
	static const char NIGHT_AUCTION_START[9]; // beginning of night session

	static const char DAY_AUCTION_START[9]; // beginning of day session
};

typedef std::shared_ptr<OrderBook> BookPtr;
// we can go to orderBook.cpp to see its definition


class LinearSol {
public:
	LinearSol() :low(0.), high(0.) {}
	LinearSol(double turnover, double qty, double lowPrice, double  highPrice, int multiplier) {
		if (fabs(lowPrice - highPrice) < 1e-6) {
			low = qty / 2;
			high = qty / 2;
		}
		else {
			low = (turnover - qty * highPrice * multiplier) / (lowPrice - highPrice) / multiplier;
			high = (turnover - qty * lowPrice * multiplier) / (highPrice - lowPrice) / multiplier;
		}
	}
	double low;
	double high;
};

inline bool isWholeNumber(double x) {
	return fabs(round(x) - x) < 1e-6;
}

typedef std::shared_ptr<LinearSol> LinearSolPtr;

#endif // !ORDERBOOK_H

