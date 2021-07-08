#include "orderBook.h"
#include "GenData.h"

#include <fstream>
#include <sstream>
#include <string>

Time_value time_value;
Time_value_map time_value_map;

const char OrderBook::NIGHT_AUCTION_START[9] = "20:59:00"; // night auction starts at 20:59:00
const char OrderBook::DAY_AUCTION_START[9] = "08:59:00"; // day auction starts at 08:59:00
void OrderBook::setup()
{
   // NewData_s newTickData;
    m_realStart = false; // when reading historical data it's false;
    m_vbook.reserve(TICK_NUM * 3); // reserve some space for the vector, otherwise it would declare sapce onece it gets full and it's slow
    // for night tick we have maximum of 3 days so we need TICK NUM multiplied by 3
    
    std::ifstream input(m_path);
    if (input.fail())
    {
        std::cout << "No " << m_path << std::endl;
        input.close();
        return;
    }
       
    std::string head;
    getline(input, head); // the first line is the column name
    //cout << head << endl;
    m_intSpread = (int)round(m_spread * m_intFactor); // set the minimum spread when price is changed to integer
    std::string date;
    std::string time;
    double price;
    int qty;
    double turnover;
    double cumOpenint;
    double yp;
    double bid;
    double ask;
    int bidQty;
    int askQty;
    int milli;
//    double buyTrade;
 //   double buy2Trade;
 //   double sellTrade;
 //   double sell2Trade;
    int cumQty;
    double cumTurnover;
    // get first tick
    while (true) {
        std::string str; // save the information into a string
        getline(input, str); // get a new line
        if (input.fail()) 
            break; // if there is no a new line then exit
        std::stringstream ss(str); // put the newline into a stringstream
        ss >> date >> time >> milli >> price >> qty >> turnover >> cumOpenint >> yp >> bid >> bidQty >> ask >> askQty >> cumQty >> cumTurnover;
        /*
        newTickData.ActionDay = date;
        newTickData.UpdateTime = time;
        newTickData.UpdateMillisec = milli;
        newTickData.LastPrice = price;
        newTickData.NewVolume = qty;
        newTickData.NewTrunover = turnover;
        newTickData.OpenInterest = cumOpenint;
        newTickData.PreClosePrice = yp;
        newTickData.BidPrice[0] = bid;
        newTickData.BidVolume[0] = bidQty;
        newTickData.AskPrice[0] = ask;
        newTickData.AskVolume[0] = askQty;
        */
        //addTickData(newTickData);

        ExtendedTick newTick(date, time, milli, price, qty, turnover, cumOpenint, yp, bid, ask, bidQty, askQty); // set new tick, here we dont't read in buyTrade...etc
         addTick(newTick);
    }
    // we output the size
    m_cumQty = cumQty;
    m_cumTurnover = cumTurnover;
    std::cout << "历史tick路径: "<< m_path<<" ======== final size: " << m_book.size() << std::endl;
    m_realStart = true; // it means loading history ends and now it starts receiving real-time data
    
    input.close();
    m_count = 0; // initialize the count of real-time data  m_logFile="output/buyselltrade.log.txt";
}

void OrderBook::addTick(ExtendedTick& newTick)
{
    time_v tv;
    m_book.push_back(newTick);
    ExtendedItr tick = m_book.end();
    tick--;
    tick->m_mid = (tick->m_bid + tick->m_ask) / 2;
    //double price;
    // for auction it's simply set buy equals sell
    // for other ticks we use the setTrade function
    if (strcmp(newTick.m_time, "20:58:00") > 0 && strcmp(newTick.m_time, "20:59:59") < 0 ||
        strcmp(newTick.m_time, "08:58:00") > 0 && strcmp(newTick.m_time, "08:59:59") < 0) {
        tick->m_buyTrade = tick->m_qty / 2;
        tick->m_sellTrade = tick->m_qty / 2;
    }
    else
    {
        setTrade(tick);
    }
    m_count++;
}

void OrderBook::addTickData(NewData_s& newTickData)
{
    time_v tv;
    m_vbook.push_back(newTickData);
    std::vector<NewData_s>::iterator tick = m_vbook.end();
    tick--; //获取最后一个元素
    tick->m_mid = (tick->BidPrice[0] + tick->AskPrice[0] / 2);
    if (strcmp(tick->UpdateTime.c_str(), "20:58:00") > 0 && strcmp(tick->UpdateTime.c_str(), "20:59:59") < 0 ||
        strcmp(tick->UpdateTime.c_str(), "08:58:00") > 0 && strcmp(tick->UpdateTime.c_str(), "08:59:59") < 0) {
        tick->m_buyTrade = tick->NewVolume / 2;
        tick->m_sellTrade = tick->NewVolume / 2;
    }
    else
    {
        setTradeTick(tick);

        std::vector<NewData_s>::iterator preTick = tick;
        --preTick;

        LinearSol result(tick->NewTrunover, tick->NewVolume, preTick->BidPrice[0], preTick->AskPrice[0], m_multiplier);
        double resultlow = result.low;
        double resulthigh = result.high;
        double intlow;
        double inthigh;
        double aa;
        double price;
        if (newTickData.NewVolume == 0)
        {
            price = 0;
        }
        else
        {
            price = (newTickData.NewTrunover) / m_multiplier / newTickData.NewVolume;
        }
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
        
        if (((tick->m_buyTrade + tick->m_sellTrade + tick->m_sell2Trade + tick->m_buy2Trade) != tick->NewVolume) && isWholeNumber(resultlow) && isWholeNumber(resulthigh) && resulthigh > -1e-5 && resultlow > -1e-5)
        {
            tick->m_buyTrade = resulthigh;
            tick->m_sellTrade = resultlow;
            aa = 1;
        }
        else
            aa = 0;
    }
    m_count++;
}

void OrderBook::setTradeTick(std::vector<NewData_s>::iterator lastTick)
{
  //  std::vector<NewData_s>::iterator end = tick.end();
  //  std::vector<NewData_s>::iterator lastTick = end--; //最后一个
    
    std::vector<NewData_s>::iterator priorTick = lastTick;//前一个
    --priorTick;
    if (lastTick->NewVolume == 0) return;
    LinearSol result(lastTick->NewTrunover, lastTick->NewVolume, priorTick->BidPrice[0], priorTick->AskPrice[0], m_multiplier);
   
    bool done = false;
    if (isWholeNumber(result.low) && isWholeNumber(result.high) && result.low > -1e-5 && result.high > -1e-5) {
        lastTick->m_sellTrade = result.low;
        lastTick->m_buyTrade = result.high;
        done = true;
    }
    else {
        double price = (lastTick->NewTrunover) / m_multiplier / lastTick->NewVolume;
        if (isWholeNumber(price / m_spread)) {// one price
            if (fabs(price - priorTick->BidPrice[0]) < 1e-6) lastTick->m_sellTrade = lastTick->NewVolume;
            else if (price < priorTick->BidPrice[0]) lastTick->m_sell2Trade = lastTick->NewVolume;
            else if (fabs(price - priorTick->AskPrice[0]) < 1e-6) lastTick->m_buyTrade = lastTick->NewVolume;
            else if (price > priorTick->AskPrice[0]) lastTick->m_buy2Trade = lastTick->NewVolume;
            else {
                lastTick->m_sellTrade = lastTick->NewVolume / 2;
                lastTick->m_buyTrade = lastTick->NewVolume / 2;
            }
            done = true;
        }
        else {// more than one price
            double priceInt = price * m_intFactor;
            int upInt = (int)ceil(priceInt);
            int loInt = (int)floor(priceInt);
            if (m_intSpread > 1) {
                loInt -= (loInt % m_intSpread);
                upInt = loInt + m_intSpread;
            }
            double loPrice = double(loInt) / m_intFactor;
            double upPrice = double(upInt) / m_intFactor;
            //cout << price << " " << priceInt << " " << loPrice << " " << upPrice <<endl;
            LinearSol result2(lastTick->NewTrunover, lastTick->NewVolume, loPrice, upPrice, m_multiplier);
            if (result2.low >= 0 && result2.high >= 0 && fabs(result2.low + result2.high - lastTick->NewVolume) < 1e-6) {
                if (price > priorTick->BidPrice[0] && price < priorTick->AskPrice[0]) {
                    lastTick->m_sellTrade = result2.low;
                    lastTick->m_buyTrade = result2.high;
                    done = true;
                }
                else if (price >= priorTick->AskPrice[0]) {
                    lastTick->m_buyTrade = result2.low;
                    lastTick->m_buy2Trade = result2.high;
                    done = true;
                }
                else if (price <= priorTick->BidPrice[0]) {
                    lastTick->m_sell2Trade = result2.low;
                    lastTick->m_sellTrade = result2.high;
                    done = true;
                }
            }
        }
    }
    if (!done) {
        if (fabs(lastTick->AskPrice[0] - lastTick->BidPrice[0] - m_spread) < 1e-6) { // minimum spread
            if (lastTick->m_mid > priorTick->m_mid) {
                LinearSol result2(lastTick->NewTrunover, lastTick->NewVolume, lastTick->BidPrice[0], lastTick->AskPrice[0], m_multiplier);
                lastTick->m_buyTrade = result2.low;
                lastTick->m_buy2Trade = result2.high;
            }
            else if (lastTick->m_mid < priorTick->m_mid) {
                LinearSol result2(lastTick->NewTrunover, lastTick->NewVolume, lastTick->BidPrice[0], lastTick->AskPrice[0], m_multiplier);
                lastTick->m_sell2Trade = result2.low;
                lastTick->m_sellTrade = result2.high;
            }
            else { // mid price doesn't change, and also minimum spread
                if (lastTick->LastPrice >= lastTick->AskPrice[0]) {
                    LinearSol result2(lastTick->NewTrunover, lastTick->NewVolume, lastTick->AskPrice[0], lastTick->AskPrice[0] + m_spread, m_multiplier);
                    lastTick->m_buyTrade = result2.low;
                    lastTick->m_buy2Trade = result2.high;
                }
                else if (lastTick->LastPrice <= lastTick->BidPrice[0]) {
                    LinearSol result2(lastTick->NewTrunover, lastTick->NewVolume, lastTick->BidPrice[0] - m_spread, lastTick->AskPrice[0], m_multiplier);
                    lastTick->m_sell2Trade = result2.low;
                    lastTick->m_sellTrade = result2.high;
                }
            }
        }
        else { // spread>minimum
            if (lastTick->AskPrice[0] > priorTick->AskPrice[0]) {
                LinearSol result2(lastTick->NewTrunover, lastTick->NewVolume, priorTick->AskPrice[0], lastTick->AskPrice[0], m_multiplier);
                lastTick->m_buyTrade = result2.low;
                lastTick->m_buy2Trade = result2.high;
            }
            else if (lastTick->BidPrice[0] < priorTick->BidPrice[0]) {
                LinearSol result2(lastTick->NewTrunover, lastTick->NewVolume, lastTick->BidPrice[0], priorTick->BidPrice[0], m_multiplier);
                lastTick->m_sell2Trade = result2.low;
                lastTick->m_sellTrade = result2.high;
            }
        }
    }

}

void OrderBook::setTrade(ExtendedItr tick)
{
    ExtendedItr preTick = tick;
    --preTick;
    if (tick->m_qty == 0) return;
    LinearSol result(tick->m_turnover, tick->m_qty, preTick->m_bid, preTick->m_ask, m_multiplier);
    bool done = false;
    if (isWholeNumber(result.low) && isWholeNumber(result.high) && result.low > -1e-5 && result.high > -1e-5) {
        tick->m_sellTrade = result.low;
        tick->m_buyTrade = result.high;
        done = true;
    }
    else {
        double price = (tick->m_turnover) / m_multiplier / tick->m_qty;
        if (isWholeNumber(price / m_spread)) {// one price
            if (fabs(price - preTick->m_bid) < 1e-6) tick->m_sellTrade = tick->m_qty;
            else if (price < preTick->m_bid) tick->m_sell2Trade = tick->m_qty;
            else if (fabs(price - preTick->m_ask) < 1e-6) tick->m_buyTrade = tick->m_qty;
            else if (price > preTick->m_ask) tick->m_buy2Trade = tick->m_qty;
            else {
                tick->m_sellTrade = tick->m_qty / 2;
                tick->m_buyTrade = tick->m_qty / 2;
            }
            done = true;
        }
        else {// more than one price
            double priceInt = price * m_intFactor;
            int upInt =(int) ceil(priceInt);
            int loInt =(int) floor(priceInt);
            if (m_intSpread > 1) {
                loInt -= (loInt % m_intSpread);
                upInt = loInt + m_intSpread;
            }
            double loPrice = double(loInt) / m_intFactor;
            double upPrice = double(upInt) / m_intFactor;
            //cout << price << " " << priceInt << " " << loPrice << " " << upPrice <<endl;
            LinearSol result2(tick->m_turnover, tick->m_qty, loPrice, upPrice, m_multiplier);
            if (result2.low >= 0 && result2.high >= 0 && fabs(result2.low + result2.high - tick->m_qty) < 1e-6) {
                if (price > preTick->m_bid && price < preTick->m_ask) {
                    tick->m_sellTrade = result2.low;
                    tick->m_buyTrade = result2.high;
                    done = true;
                }
                else if (price >= preTick->m_ask) {
                    tick->m_buyTrade = result2.low;
                    tick->m_buy2Trade = result2.high;
                    done = true;
                }
                else if (price <= preTick->m_bid) {
                    tick->m_sell2Trade = result2.low;
                    tick->m_sellTrade = result2.high;
                    done = true;
                }
            }
        }
    }
    if (!done) {
        if (fabs(tick->m_ask - tick->m_bid - m_spread) < 1e-6) { // minimum spread
            if (tick->m_mid > preTick->m_mid) {
                LinearSol result2(tick->m_turnover, tick->m_qty, tick->m_bid, tick->m_ask, m_multiplier);
                tick->m_buyTrade = result2.low;
                tick->m_buy2Trade = result2.high;
            }
            else if (tick->m_mid < preTick->m_mid) {
                LinearSol result2(tick->m_turnover, tick->m_qty, tick->m_bid, tick->m_ask, m_multiplier);
                tick->m_sell2Trade = result2.low;
                tick->m_sellTrade = result2.high;
            }
            else { // mid price doesn't change, and also minimum spread
                if (tick->m_price >= tick->m_ask) {
                    LinearSol result2(tick->m_turnover, tick->m_qty, tick->m_ask, tick->m_ask + m_spread, m_multiplier);
                    tick->m_buyTrade = result2.low;
                    tick->m_buy2Trade = result2.high;
                }
                else if (tick->m_price <= tick->m_bid) {
                    LinearSol result2(tick->m_turnover, tick->m_qty, tick->m_bid - m_spread, tick->m_ask, m_multiplier);
                    tick->m_sell2Trade = result2.low;
                    tick->m_sellTrade = result2.high;
                }
            }
        }
        else { // spread>minimum
            if (tick->m_ask > preTick->m_ask) {
                LinearSol result2(tick->m_turnover, tick->m_qty, preTick->m_ask, tick->m_ask, m_multiplier);
                tick->m_buyTrade = result2.low;
                tick->m_buy2Trade = result2.high;
            }
            else if (tick->m_bid < preTick->m_bid) {
                LinearSol result2(tick->m_turnover, tick->m_qty, tick->m_bid, preTick->m_bid, m_multiplier);
                tick->m_sell2Trade = result2.low;
                tick->m_sellTrade = result2.high;
            }
        }
    }
}

void OrderBook::output()
{
    std::string outputFile = "output/" + m_product + "/" + m_date + ".txt";
    std::ofstream output(outputFile);

    output << "date time milli price turnover qty bid bid.qty ask ask.qty buy.trade buy2.trade sell.trade sell2.trade" << std::endl; // output head
    ExtendedItr itr;
    for (itr = m_book.begin(); itr != m_book.end(); ++itr)
        output << itr->m_date << " " << itr->m_time << " " << itr->m_milli << " " << itr->m_price
        << " " << long(itr->m_turnover) << " " << itr->m_qty << " "
        << itr->m_bid << " " << itr->m_bidQty << " " << itr->m_ask << " " << itr->m_askQty
        << " " << itr->m_buyTrade << " " << itr->m_buy2Trade << " " << itr->m_sellTrade << " "
        << itr->m_sell2Trade << std::endl; // output each tick with
    output.close();

}

void OrderBook::outputorderbook()
{
    time_t now=time(0);
    tm *gmtm;
    gmtm = gmtime(&now);
    ExtendedItr itr;
    for(auto item:time_value_map)
    {
        std::string date;
        if((1+gmtm->tm_mon)<10)
        {
            date = std::to_string(1900+gmtm->tm_year)+"0" + std::to_string(1+gmtm->tm_mon)+std::to_string(gmtm->tm_mday);
        }
        else
            date = std::to_string(1900+gmtm->tm_year) + std::to_string(1+gmtm->tm_mon)+std::to_string(gmtm->tm_mday);
        std::string outputFile="outputorderbook/" + item.first + "/" + date + ".txt";
        std::ofstream output(outputFile,std::ofstream::out | std::ofstream::app);
        output << "date time ask ask.qty bid bid.qty turnover qty buytrade selltrade buy2trade sell2trade price resulthigh resultlow inthigh intlow aa" << std::endl; // output heads
        for (auto i : item.second)
        {
            output << i.p_date << " " << i.p_time << " " << i.ask << " " << i.askqty << " " << i.bid << " " << i.bidqty << " " << i.turnover << " " << i.qty  << " " << i.buytrade <<
        " " << i.selltrade << " " << i.buy2trade << " " << i.sell2trade << " " << i.price << " " << i.resulthigh << " " << i.resultlow << " " << i.inthigh << " " << i.intlow  << " " << i.aa
                << std::endl;
        }
        output.close();
    }
}

