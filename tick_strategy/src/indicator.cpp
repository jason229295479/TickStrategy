#include <fstream>
#include <sstream>
#include <cmath>
#include "indicator.h"
#include <algorithm>
using namespace std;

// we follow the same way in R so that it's more clear
void Indicator::setup() {
    // m_signalFile = m_homePath+m_signalFile;
    ifstream input(m_signalFile); // we only need an input file of signal names and parameters
    if (input.fail()) {
        cout << "No signal File" << endl;
        return;
    }
    string head;
    getline(input, head);
    string line;
    IndicatorPara newPara;
    string signalName;
    m_period = 0;

    m_coi.clear();
    std::cout << "\n--------------" << m_signalFile << "信息----------------" << std::endl;
    while (true) {
        getline(input, line);
        if (input.fail()) break;
        stringstream str(line);
        str >> signalName >> newPara.period >> newPara.lambda >> newPara.threshold >> newPara.shortLambda >> newPara.adjust;
        std::cout << signalName << " " << newPara.period << 
                     newPara.lambda  << " " << newPara.threshold <<
                    newPara.shortLambda << " " << newPara.adjust <<std::endl;
        if (m_period == 0) m_period = newPara.period;
        m_signalName.push_back(signalName);
        m_para[signalName] = newPara;
    }
    input.close();
    m_nIndicator = m_signalName.size(); // number of signals
    m_signalVec = vector<double>(m_nIndicator, 0.); // value of signals
    m_middleVec = vector<double>(m_nIndicator * 5, 0.); // value of middle values
    m_indMat.reserve(m_nIndicator); // initialize indicator matrix, this is only used to check the result, not used in real trading
    m_indMat.resize(m_nIndicator);
    m_tickCount = 0; // count the number of ticks
    m_wpr.reserve(TICK_NUM * 3);
    m_cqty.reserve(TICK_NUM * 3);
    m_coi.reserve(TICK_NUM * 3);
    m_adjust = 1;
    m_lambda = 1 - pow(0.5, 1.0 / m_period);
}


void Indicator::updateWarmup(ExtendedItr itr) {
    IndItr signalItr = m_signalVec.begin(); // signal values
    IndItr middleItr = m_middleVec.begin(); // middle parameters for signals
    int signalCount = 0;
    double curWpr = wpr(itr);
    double maxPeriod{ curWpr }; // rolling max wpr
    double minPeriod{ curWpr }; // rolling min wpr
    int qty = itr->m_qty;
    double coi = itr->m_cumOpenint;
    m_wpr.push_back(curWpr); // calculate wpr as middle value
    m_cqty.push_back(itr->m_qty);
    m_coi.push_back(itr->m_cumOpenint);
    double curRet = (m_tickCount == 0) ? log(curWpr) - log(itr->m_yp) : (log(curWpr) - log(m_wpr[m_tickCount - 1])); // ret in R, diff(log(wpr))
    // now we calculate all the signals
    string signalName;
    // calcuate totalImb
    long long iter;
    signalName = "total.imb." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        totalImb(itr, signalItr, middleItr, m_para[signalName].lambda, m_para[signalName].threshold, m_tickCount + 1);
        middleItr++;
        m_indMap[signalName] = *(signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // calculate tradeImb
    signalName = "trade.imb." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        tradeImb(itr, signalItr, middleItr, middleItr + 1, m_para[signalName].lambda);
        m_indMap[signalName] = *(signalItr);
        middleItr += 2;
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    // calculate BSimb
    signalName = "BS.imb." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        BSImb(itr, signalItr, middleItr, m_para[signalName].lambda, m_tickCount + 1);
        m_indMap[signalName] = *(signalItr);
        middleItr++;
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // total.trade.imb
    signalName = "total.trade.imb." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        totaltradeImb(itr, signalItr, middleItr, middleItr + 1, middleItr + 2, middleItr + 3, m_para[signalName].lambda, m_tickCount + 1, m_para[signalName].threshold);
        m_indMap[signalName] = *(signalItr);
        middleItr += 4;
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // calcualte nr
    signalName = "nr." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        m_adjust *= (1 - m_lambda);
        nr(signalItr, curRet, middleItr, middleItr + 1, middleItr + 2, middleItr + 3, m_para[signalName].lambda, m_tickCount + 1, m_para[signalName].threshold);
        m_indMap["nr." + to_string(m_period)] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
        middleItr += 4;
    }
    // calculate dbook
    signalName = "dbook." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        if (m_tickCount > 0) dbook(itr, signalItr, middleItr, m_para[signalName].lambda, m_tickCount + 1, m_para[signalName].adjust);
        m_indMap["dbook." + to_string(m_period)] = *(signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
        middleItr += 1;
    }

    if (m_tickCount < m_period - 1) {
        while ((!m_maxQi.empty()) && curWpr >= m_wpr[m_maxQi.back()])
            m_maxQi.pop_back(); // Remove from rear
        m_maxQi.push_back(m_tickCount);
        maxPeriod = m_wpr[m_maxQi.front()]; // rolling for the first period items, otherwise comment it
        while ((!m_minQi.empty()) && curWpr <= m_wpr[m_minQi.back()])
            m_minQi.pop_back(); // Remove from rear
        m_minQi.push_back(m_tickCount);
        minPeriod = m_wpr[m_minQi.front()]; // rolling for the first period items, otherwise comment it
    }
    else if (m_tickCount == m_period - 1) {
        while ((!m_maxQi.empty()) && curWpr >= m_wpr[m_maxQi.back()])
            m_maxQi.pop_back(); // Remove from rear
        m_maxQi.push_back(m_tickCount);
        maxPeriod = m_wpr[m_maxQi.front()];
        while ((!m_minQi.empty()) && curWpr <= m_wpr[m_minQi.back()])
            m_minQi.pop_back(); // Remove from rear
        m_minQi.push_back(m_tickCount);
        minPeriod = m_wpr[m_minQi.front()];
    }
    else {
        while ((!m_maxQi.empty()) && m_maxQi.front() <= m_tickCount - m_period)
            m_maxQi.pop_front(); // Remove from front of queue
        while ((!m_maxQi.empty()) && curWpr >= m_wpr[m_maxQi.back()])
            m_maxQi.pop_back();
        m_maxQi.push_back(m_tickCount);
        maxPeriod = m_wpr[m_maxQi.front()];
        while ((!m_minQi.empty()) && m_minQi.front() <= m_tickCount - m_period)
            m_minQi.pop_front(); // Remove from front of queue
        while ((!m_minQi.empty()) && curWpr <= m_wpr[m_minQi.back()])
            m_minQi.pop_back();
        m_minQi.push_back(m_tickCount);
        minPeriod = m_wpr[m_minQi.front()];
    }
    // then we can calculate rangePos easily;
    signalName = "range.pos." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        rangePos(signalItr, middleItr, curWpr, maxPeriod, minPeriod, m_para[signalName].lambda, m_tickCount + 1);
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
        middleItr++;
    }
    // calculate rsi
    signalName = "rsi." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        rsi(curRet, signalItr, middleItr, middleItr + 1, m_para[signalName].lambda,
            m_para[signalName].threshold);
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
        middleItr += 2;
    }
    // calculate price.osci
    signalName = "price.osci." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        priceOsci(curWpr, signalItr, middleItr, middleItr + 1, middleItr + 2, middleItr + 3,
            maxPeriod, minPeriod,
            m_para[signalName].shortLambda, m_para[signalName].lambda,
            m_para[signalName].threshold, m_tickCount + 1);
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
        middleItr += 4;
    }
    // calculate kdjK
    signalName = "kdj.k." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        kdjK(curWpr, signalItr, middleItr, maxPeriod, minPeriod, m_para[signalName].shortLambda, m_tickCount + 1);
        middleItr++;
        double kdjkValue = *signalItr;
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // kdj.j
    signalName = "kdj.j." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        double kdjkValue = m_indMap["kdj.k." + to_string(m_period)];
        kdjJ(kdjkValue, signalItr, middleItr, m_para[signalName].shortLambda, m_tickCount + 1);
        middleItr++;
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // ma.dif.10
    signalName = "ma.dif.10." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        maDif10(signalItr, middleItr, middleItr + 1, m_para[signalName].shortLambda, middleItr + 2, middleItr + 3, m_para[signalName].lambda);
        middleItr += 4;
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    /// the following signals are range signals which would not be counted as finals signals
    // so they would be in m_indMap, which would be used by other directional signals
    // but they would not be in m_indMat
    // range
    signalName = "range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        rangePeriod(middleItr, maxPeriod, minPeriod);
        m_indMap[signalName] = (*middleItr++);
    }
   
    // trendIndex
    signalName = "trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        trendIndex(middleItr, maxPeriod, minPeriod);
        *signalItr = *middleItr;
        m_indMap[signalName] = (*middleItr++);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //volume.open.ratio
    signalName = "volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
       
        cumVolumeOpenint(signalItr, middleItr, middleItr + 1);
        middleItr += 2;
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    //std
    signalName = "std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        // fastRollVar
        fastRollVar(middleItr, middleItr + 1, middleItr + 2);
        double rollVarWpr = (*middleItr);
        middleItr += 3;
        stdPeriod(middleItr, rollVarWpr);
        m_indMap[signalName] = (*middleItr++);
    }
  
    //absnr
    signalName = "absnr." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        absNr(middleItr);
        m_indMap[signalName] = *(middleItr++);
    }
    //BS.imb.std
    signalName = "BS.imb.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "BS.imb." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //total.trade.imb.std
    signalName = "total.trade.imb.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "total.trade.imb." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // nr.std
    signalName = "nr.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "nr." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //dbook.std
    signalName = "dbook.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "dbook." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //range.pos.std
    signalName = "range.pos.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "range.pos." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //price.osci.std
    signalName = "price.osci.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "price.osci." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //kdj.k.std
    signalName = "kdj.k.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.k." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // kdj.j.std
    signalName = "kdj.j.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.j." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    // ma.dif.10.std
    signalName = "ma.dif.10.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "ma.dif.10." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    //BS.imb.range
    signalName = "BS.imb.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "BS.imb." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    // total.trade.imb.range
    signalName = "total.trade.imb.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "total.trade.imb." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // nr.range
    signalName = "nr.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "nr." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // dbook.range
    signalName = "dbook.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "dbook." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //range.pos.range
    signalName = "range.pos.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "range.pos." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // price.osci.range
    signalName = "price.osci.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "price.osci." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //kdj.k.range
    signalName = "kdj.k.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.k." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //kdj.j.range
    signalName = "kdj.j.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.j." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //ma.dif.10.range
    signalName = "ma.dif.10.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "ma.dif.10." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // BS.imb.trend.index
    signalName = "BS.imb.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "BS.imb." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // total.trade.imb.trend.index
    signalName = "total.trade.imb.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "total.trade.imb." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // nr.trend.index
    signalName = "nr.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "nr." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // dbook.trend.index
    signalName = "dbook.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "dbook." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }


    // range.pos.trend.index
    signalName = "range.pos.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "range.pos." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // price.osci.trend.index
    signalName = "price.osci.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "price.osci." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // kdj.k.trend.index
    signalName = "kdj.k.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.k." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // kdj.j.trend.index
    signalName = "kdj.j.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.j." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //ma.dif.10.trend.index
    signalName = "ma.dif.10.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "ma.dif.10." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    // BS.imb.volume.open.ratio
    signalName = "BS.imb.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "BS.imb." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // total.trade.imb.volume.open.ratio
    signalName = "total.trade.imb.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "total.trade.imb." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // nr.volume.open.ratio
    signalName = "nr.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "nr." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // dbook.volume.open.ratio
    signalName = "dbook.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "dbook." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // range.pos.volume.open.ratio
    signalName = "range.pos.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "range.pos." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //price.osci.volume.open.ratio
    signalName = "price.osci.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "price.osci." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    //kdj.k.volume.open.ratio
    signalName = "kdj.k.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.k." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //kdj.j.volume.open.ratio
    signalName = "kdj.j.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.j." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //ma.dif.10.volume.open.ratio
    signalName = "ma.dif.10.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "ma.dif.10." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // dbook.abs.nr
    signalName = "dbook.abs.nr." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "dbook." + to_string(m_period), "abs.nr." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    m_tickCount++;
}

void Indicator::updateIndicator(ExtendedItr itr) {
    IndItr signalItr = m_signalVec.begin();
    IndItr middleItr = m_middleVec.begin();
    int signalCount = 0;
    double curWpr = wpr(itr);
    double maxPeriod{ curWpr };
    double minPeriod{ curWpr };
    int qty = itr->m_qty;
    double coi = itr->m_cumOpenint;
    m_wpr.push_back(curWpr); // calculate wpr as middle value
    m_cqty.push_back(itr->m_qty);
    m_coi.push_back(itr->m_cumOpenint);
    double curRet = (m_tickCount == 0) ? log(curWpr) - log(itr->m_yp) : (log(curWpr) - log(m_wpr[m_tickCount - 1])); // ret in R, diff(log(wpr))
    // now we calculate all the signals
    // calcuate totalImb
    string signalName;
    // calcuate totalImb
    long long iter;
    signalName = "total.imb." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        totalImb(itr, signalItr, middleItr, m_para[signalName].lambda, m_para[signalName].threshold, m_tickCount + 1);
        middleItr++;
        m_indMap[signalName] = *(signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // calculate tradeImb
    signalName = "trade.imb." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        tradeImb(itr, signalItr, middleItr, middleItr + 1, m_para[signalName].lambda);
        m_indMap[signalName] = *(signalItr);
        middleItr += 2;
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    signalName = "BS.imb." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        BSImb(itr, signalItr, middleItr, m_para[signalName].lambda, m_tickCount + 1);
        m_indMap[signalName] = *(signalItr);
        middleItr++;
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    signalName = "total.trade.imb." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        totaltradeImb(itr, signalItr, middleItr, middleItr + 1, middleItr + 2, middleItr + 3, m_para[signalName].lambda, m_tickCount + 1, m_para[signalName].threshold);
        m_indMap[signalName] = *(signalItr);
        middleItr += 4;
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // calcualte nr
    signalName = "nr." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        m_adjust *= (1 - m_lambda);
        nr(signalItr, curRet, middleItr, middleItr + 1, middleItr + 2, middleItr + 3, m_para[signalName].lambda, m_tickCount + 1, m_para[signalName].threshold);
        m_indMap["nr." + to_string(m_period)] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
        middleItr += 4;
    }
    // calculate dbook
    signalName = "dbook." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        if (m_tickCount > 0) dbook(itr, signalItr, middleItr, m_para[signalName].lambda, m_tickCount + 1, m_para[signalName].adjust);
        m_indMap["dbook." + to_string(m_period)] = *(signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
        middleItr += 1;
    }
    // calculate range.pos
    // it's quite complicate because it relies on rolling min and max wpr
    // now first calculate rolling min and max
    //maxPeriod=m_wpr[0];
    //minPeriod=m_wpr[0];
    if (m_tickCount < m_period - 1) {
        while ((!m_maxQi.empty()) && curWpr >= m_wpr[m_maxQi.back()])
            m_maxQi.pop_back(); // Remove from rear
        m_maxQi.push_back(m_tickCount);
        maxPeriod = m_wpr[m_maxQi.front()]; // rolling for the first period items, otherwise comment it
        while ((!m_minQi.empty()) && curWpr <= m_wpr[m_minQi.back()])
            m_minQi.pop_back(); // Remove from rear
        m_minQi.push_back(m_tickCount);
        minPeriod = m_wpr[m_minQi.front()]; // rolling for the first period items, otherwise comment it
    }
    else if (m_tickCount == m_period - 1) {
        while ((!m_maxQi.empty()) && curWpr >= m_wpr[m_maxQi.back()])
            m_maxQi.pop_back(); // Remove from rear
        m_maxQi.push_back(m_tickCount);
        maxPeriod = m_wpr[m_maxQi.front()];
        while ((!m_minQi.empty()) && curWpr <= m_wpr[m_minQi.back()])
            m_minQi.pop_back(); // Remove from rear
        m_minQi.push_back(m_tickCount);
        minPeriod = m_wpr[m_minQi.front()];
    }
    else {
        while ((!m_maxQi.empty()) && m_maxQi.front() <= m_tickCount - m_period)
            m_maxQi.pop_front(); // Remove from front of queue
        while ((!m_maxQi.empty()) && curWpr >= m_wpr[m_maxQi.back()])
            m_maxQi.pop_back();
        m_maxQi.push_back(m_tickCount);
        maxPeriod = m_wpr[m_maxQi.front()];
        while ((!m_minQi.empty()) && m_minQi.front() <= m_tickCount - m_period)
            m_minQi.pop_front(); // Remove from front of queue
        while ((!m_minQi.empty()) && curWpr <= m_wpr[m_minQi.back()])
            m_minQi.pop_back();
        m_minQi.push_back(m_tickCount);
        minPeriod = m_wpr[m_minQi.front()];
    }
    // then we can calculate rangePos easily;
    signalName = "range.pos." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        rangePos(signalItr, middleItr, curWpr, maxPeriod, minPeriod, m_para[signalName].lambda, m_tickCount + 1);
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
        middleItr++;
    }
    // calculate rsi
    signalName = "rsi." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        rsi(curRet, signalItr, middleItr, middleItr + 1, m_para[signalName].lambda,
            m_para[signalName].threshold);
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
        middleItr += 2;
    }
    // calculate price.osci
    signalName = "price.osci." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        priceOsci(curWpr, signalItr, middleItr, middleItr + 1, middleItr + 2, middleItr + 3,
            maxPeriod, minPeriod,
            m_para[signalName].shortLambda, m_para[signalName].lambda,
            m_para[signalName].threshold, m_tickCount + 1);
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
        middleItr += 4;
    }
    // calculate kdjK
    signalName = "kdj.k." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        kdjK(curWpr, signalItr, middleItr, maxPeriod, minPeriod, m_para[signalName].shortLambda, m_tickCount + 1);
        middleItr++;
        double kdjkValue = *signalItr;
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // kdj.j
    signalName = "kdj.j." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        double kdjkValue = m_indMap["kdj.k." + to_string(m_period)];
        kdjJ(kdjkValue, signalItr, middleItr, m_para[signalName].shortLambda, m_tickCount + 1);
        middleItr++;
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // ma.dif.10
    signalName = "ma.dif.10." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        maDif10(signalItr, middleItr, middleItr + 1, m_para[signalName].shortLambda, middleItr + 2, middleItr + 3, m_para[signalName].lambda);
        middleItr += 4;
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    /// the following signals are range signals which would not be counted as finals signals
    // so they would be in m_indMap, which would be used by other directional signals
    // but they would not be in m_indMat
    
    // range
    signalName = "range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        rangePeriod(middleItr, maxPeriod, minPeriod);
        m_indMap[signalName] = (*middleItr++);
    }
   
    // trendIndex
    signalName = "trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        trendIndex(middleItr, maxPeriod, minPeriod);
        *signalItr = *middleItr;
        m_indMap[signalName] = (*middleItr++);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    signalName = "volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        cumVolumeOpenint(signalItr, middleItr, middleItr + 1);
        middleItr += 2;
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    
    //std
    signalName = "std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        fastRollVar(middleItr, middleItr + 1, middleItr + 2);
        double rollVarWpr = (*middleItr);
        middleItr += 3;
        stdPeriod(middleItr, rollVarWpr);
        m_indMap[signalName] = (*middleItr++);

    }
   
    //absnr
    signalName = "absnr." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        absNr(middleItr);
        m_indMap[signalName] = *(middleItr++);
    }
    //BS.imb.std
    signalName = "BS.imb.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "BS.imb." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //total.trade.imb.std
    signalName = "total.trade.imb.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "total.trade.imb." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // nr.std
    signalName = "nr.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "nr." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //dbook.std
    signalName = "dbook.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "dbook." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //range.pos.std
    signalName = "range.pos.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "range.pos." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //price.osci.std
    signalName = "price.osci.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "price.osci." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //kdj.k.std
    signalName = "kdj.k.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.k." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // kdj.j.std
    signalName = "kdj.j.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.j." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    // ma.dif.10.std
    signalName = "ma.dif.10.std." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "ma.dif.10." + to_string(m_period), "std." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    //BS.imb.range
    signalName = "BS.imb.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "BS.imb." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    // total.trade.imb.range
    signalName = "total.trade.imb.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "total.trade.imb." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // nr.range
    signalName = "nr.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "nr." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // dbook.range
    signalName = "dbook.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "dbook." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //range.pos.range
    signalName = "range.pos.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "range.pos." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // price.osci.range
    signalName = "price.osci.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "price.osci." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //kdj.k.range
    signalName = "kdj.k.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.k." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //kdj.j.range
    signalName = "kdj.j.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.j." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //ma.dif.10.range
    signalName = "ma.dif.10.range." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "ma.dif.10." + to_string(m_period), "range." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    // BS.imb.trend.index
    signalName = "BS.imb.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "BS.imb." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // total.trade.imb.trend.index
    signalName = "total.trade.imb.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "total.trade.imb." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // nr.trend.index
    signalName = "nr.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "nr." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // dbook.trend.index
    signalName = "dbook.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "dbook." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    // range.pos.trend.index
    signalName = "range.pos.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "range.pos." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // price.osci.trend.index
    signalName = "price.osci.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "price.osci." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // kdj.k.trend.index
    signalName = "kdj.k.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.k." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // kdj.j.trend.index
    signalName = "kdj.j.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.j." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //ma.dif.10.trend.index
    signalName = "ma.dif.10.trend.index." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "ma.dif.10." + to_string(m_period), "trend.index." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    // BS.imb.volume.open.ratio
    signalName = "BS.imb.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "BS.imb." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // total.trade.imb.volume.open.ratio
    signalName = "total.trade.imb.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "total.trade.imb." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // nr.volume.open.ratio
    signalName = "nr.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "nr." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // dbook.volume.open.ratio
    signalName = "dbook.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "dbook." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // range.pos.volume.open.ratio
    signalName = "range.pos.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "range.pos." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //price.osci.volume.open.ratio
    signalName = "price.osci.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "price.osci." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }

    //kdj.k.volume.open.ratio
    signalName = "kdj.k.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.k." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //kdj.j.volume.open.ratio
    signalName = "kdj.j.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "kdj.j." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    //ma.dif.10.volume.open.ratio
    signalName = "ma.dif.10.volume.open.ratio." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "ma.dif.10." + to_string(m_period), "volume.open.ratio." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    // dbook.abs.nr
    signalName = "dbook.abs.nr." + to_string(m_period);
    iter = std::count(m_signalName.begin(), m_signalName.end(), signalName);
    if (iter == 1)
    {
        compositeIndicator(signalItr, "dbook." + to_string(m_period), "abs.nr." + to_string(m_period));
        m_indMap[signalName] = (*signalItr);
        m_indMat[signalCount++][m_tickCount] = *(signalItr++);
    }
    m_tickCount++;
}



void Indicator::outputIndicator(string date) {
    ofstream output("output/outputIndicator/" + m_product + "/" + date);
    output << m_signalName[0];
    for (size_t i = 1; i != m_nIndicator; ++i)
        output << " " << m_signalName[i];
    output << endl;
    output.precision(10);
    for (size_t tick = 0; tick != m_tickCount; ++tick) {
        output << m_indMat[0][tick];
        for (size_t signal = 1; signal != m_nIndicator; ++signal)
            output << " " << m_indMat[signal][tick];
        output << endl;
    }
    output.close();
}


void Indicator::tradeImb(ExtendedItr itr, IndItr signal, IndItr numerator,
    IndItr denominator, double ratio) {
    double nu = ewma(numerator, double(itr->m_buyTrade + itr->m_buy2Trade - itr->m_sellTrade - itr->m_sell2Trade), ratio);
    double de = ewma(denominator, itr->m_qty, ratio);
    *signal = zeroDiv(nu, de);
}

void Indicator::BSImb(ExtendedItr itr, IndItr signal, IndItr ema, double ratio, int n) {
    double aa = zeroDiv(double(itr->m_buyTrade - itr->m_sellTrade), double(itr->m_buyTrade + itr->m_sellTrade));
    ewmaAdjust(ema, signal, aa, ratio, n);
}


void Indicator::totaltradeImb(ExtendedItr itr, IndItr signal, IndItr nuEMA, IndItr nuAdjust, IndItr deEMA, IndItr deAdjust,
    double ratio, int n, double thre) {
    // double nu=ewmaAdjust(nuEMA,nuAdjust, double(itr->m_buyTrade+itr->m_buy2Trade-itr->m_sellTrade-itr->m_sell2Trade), ratio,n);
    //  double de=ewmaAdjust(deEMA,deAdjust, double(itr->m_buyTrade+itr->m_buy2Trade+itr->m_sellTrade+itr->m_sell2Trade), ratio,n);
    // double de=ewmaAdjust(deEMA,deAdjust, itr->m_qty, ratio,n);
    *signal = zeroDiv(ewmaAdjust(nuEMA, nuAdjust, double(itr->m_buyTrade + itr->m_buy2Trade - itr->m_sellTrade - itr->m_sell2Trade), ratio, n), ewmaAdjust(deEMA, deAdjust, itr->m_qty, ratio, n));
    vanish(*signal, thre);
}
// x3 <- list(nr.period ~ vanish.thre(nr(ret, period), 0.15))
void Indicator::nr(IndItr signal, double ret, IndItr nuEMA, IndItr nuAdjust, IndItr deEMA, IndItr deAdjust, double ratio, int n, double thre) {
    // double nu=ewmaAdjust(nuEMA,nuAdjust, ret, ratio,n);
    // double de=ewmaAdjust(deEMA,deAdjust, fabs(ret), ratio,n);
    double nu = ewmaAdjust(nuEMA, nuAdjust, ret, ratio, n);
    double absret = fabs(ret);
    double de = ewmaAdjust(deEMA, deAdjust, absret, ratio, n);

    *signal = zeroDiv(nu, de);
    vanish(*signal, thre);
}

// range.pos.period ~ get.range.pos(wpr, min.period, max.period)
void Indicator::rangePos(IndItr signal, IndItr ema, double wpr, double maxPeriod, double minPeriod, double ratio, int n) {
    ewmaAdjust(ema, signal, zeroDiv(wpr - minPeriod, maxPeriod - minPeriod), ratio, n);
    *signal = *signal - 0.5;
}

// ewma(sign(fill.diff(bid.qty))-sign(fill.diff(ask.qty)))
void Indicator::dbook(ExtendedItr itr, IndItr signal, IndItr ema, double ratio, int n, int adjust) {
    ExtendedItr preItr = prev(itr);
    if (adjust == 1)
    {
        ewmaAdjust(ema, signal, sign(itr->m_bidQty - preItr->m_bidQty) - sign(itr->m_askQty - preItr->m_askQty), ratio, n);
    }
    else
        ewma(signal, sign(itr->m_bidQty - preItr->m_bidQty) - sign(itr->m_askQty - preItr->m_askQty), ratio);
}

void Indicator::rsi(double ret, IndItr signal, IndItr upTotal, IndItr moveTotal, double ratio, double thre) {
    double absMove = fabs(ret);
    double upMove = max(ret, 0.);
    double up = ewma(upTotal, upMove, ratio);
    double move = ewma(moveTotal, absMove, ratio);
    *signal = (up < 1e-8 || move < 1e-8) ? 0. : (up / move - 0.5);
    vanish(*signal, thre);
}


double& Indicator::ewmaAdjust(IndItr ema, IndItr emaAdjust, double x, double ratio, int n) {
    ewma(ema, x, ratio);
    return (*emaAdjust = *ema / (1 - pow(1 - ratio, n)));
}
// using ewma with adjust
void Indicator::totalImb(ExtendedItr itr, IndItr signal, IndItr ema, double ratio, double thre, int n) {
    double aa = zeroDiv(double(itr->m_buyTrade + itr->m_buy2Trade - itr->m_sellTrade - itr->m_sell2Trade), itr->m_qty);
    ewmaAdjust(ema, signal, aa, ratio, n);
    vanish(*signal, thre);
}

void Indicator::priceOsci(double wpr, IndItr signal, IndItr shortEMA, IndItr shortAdjust,
    IndItr longEMA, IndItr longAdjust, double maxPeriod, double minPeriod,
    double shortRatio, double ratio, double thre, int n) {
    double shorts = ewmaAdjust(shortEMA, shortAdjust, wpr, shortRatio, n);
    double longs = ewmaAdjust(longEMA, longAdjust, wpr, ratio, n);
    *signal = zeroDiv(shorts - longs, maxPeriod - minPeriod);
    vanish(*signal, thre);
}

void Indicator::kdjK(double wpr, IndItr signal, IndItr ema, double maxPeriod, double minPeriod, double ratio, int n) {
    ewmaAdjust(ema, signal, (zeroDiv(wpr - minPeriod, maxPeriod - minPeriod) - 0.5) * 2, ratio, n);
}

void Indicator::kdjJ(double kdjK, IndItr signal, IndItr ema, double ratio, int n) {
    ewmaAdjust(ema, signal, kdjK, ratio, n);
}

void Indicator::trendIndex(IndItr signal, double maxPeriod, double minPeriod) {
    *signal = (m_tickCount < m_period ? 0.0 : zeroDiv(fabs(m_wpr[m_tickCount] - m_wpr[m_tickCount - m_period]), maxPeriod - minPeriod));
}

void Indicator::maDif10(IndItr signal, IndItr shortEMA, IndItr shortAdjust, double shortRatio,
    IndItr longEMA, IndItr longAdjust, double ratio) {
    //if (m_tickCount==0) *signal=0.0;
    *signal = zeroDiv(ewmaAdjust(shortEMA, shortAdjust, m_wpr[m_tickCount], shortRatio, m_tickCount + 1) -
        ewmaAdjust(longEMA, longAdjust, m_wpr[m_tickCount], ratio, m_tickCount + 1), m_wpr[m_tickCount]);
}


void Indicator::fastRollVar(IndItr signal, IndItr ma, IndItr ma2) {
    *ma += m_wpr[m_tickCount] / m_period;
    *ma2 += m_wpr[m_tickCount] * m_wpr[m_tickCount] / m_period;
    if (m_tickCount >= m_period) {
        *ma -= (m_wpr[m_tickCount - m_period] / m_period);
        *ma2 -= m_wpr[m_tickCount - m_period] * m_wpr[m_tickCount - m_period] / m_period;
    }
    *signal = (*ma2) - (*ma) * (*ma);
}

/*
void Indicator::cumVolumeOpenint(IndItr signal, IndItr ma, IndItr ma2) {
  *ma+=m_cqty[m_tickCount];
  *ma2+=m_ccumOpenint[m_tickCount]/m_period;
  if (m_tickCount>=m_period) {
    *ma-=m_cqty[m_tickCount-m_period];
    *ma2-=m_ccumOpenint[m_tickCount-m_period]/m_period;
  }
  *signal = zeroDiv(*ma, *ma2);
}
*/
void Indicator::cumVolumeOpenint(IndItr signal, IndItr ma, IndItr ma2) {
    *ma += m_cqty[m_tickCount];
    *ma2 += m_coi[m_tickCount] / m_period;
    if (m_tickCount >= m_period) {
        *ma -= m_cqty[m_tickCount - m_period];
        *ma2 -= m_coi[m_tickCount - m_period] / m_period;
    }
    *signal = zeroDiv(*ma, *ma2);
}

/*
void fastRollCor(IndItr signal, deque<double>& x, deque<double>& y, IndItr xMa, IndItr yMa,
                 IndItr x2Ma, IndItr y2Ma, deque<double>& upDeq, int period) {
  *xMa=*xMa+(x.back()-x.front())/period;
  *yMa=*yMa+(y.back()-y.front())/period;
  *x2Ma=*x2Ma+(x.back()*x.back()-x.front()*x.front())/period;
  *y2Ma=*y2Ma+(y.back()*y.back()-y.front()*y.front())/period;
  double varX=max(*x2Ma-(*xMa)*(*xMa), 0.);
  double varY=max(*y2Ma-(*yMa)*(*yMa), 0.);
  double upper=(x.back()-*xMa)*(y.back()-*yMa);
}
*/
