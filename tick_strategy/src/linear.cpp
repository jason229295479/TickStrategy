#include "linear.h"
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <fstream>
#include <sstream>
void Linear::getPred()
{
    m_pred = 0.;
    for (auto item : m_coef)
        m_pred += (*m_indMap)[item.first] * item.second; // first value is signal value, second value is coefficent
    m_predAll.push_back(m_pred);
}
void Linear::getAction()
{
    for (auto iStrat = 0; iStrat != m_strat.size(); ++iStrat) {
        if (m_pred > m_strat[iStrat].open) m_action[iStrat] = 1; // buy open
        else if (m_pred < -m_strat[iStrat].open) m_action[iStrat] = 2; // sell open
        else if (m_pred >= -m_strat[iStrat].close && m_pred <= m_strat[iStrat].close) m_action[iStrat] = 5; // buy close and sell close
        else if (m_pred >= -m_strat[iStrat].close) m_action[iStrat] = 3; // buy close
        else if (m_pred <= m_strat[iStrat].close) m_action[iStrat] = 4; // sell close
        else m_action[iStrat] = 0; // nothing
    }for (auto iStrat = 0; iStrat != m_strat.size(); ++iStrat) {
        if (m_pred > m_strat[iStrat].open) m_action[iStrat] = 1; // buy open
        else if (m_pred < -m_strat[iStrat].open) m_action[iStrat] = 2; // sell open
        else if (m_pred >= -m_strat[iStrat].close && m_pred <= m_strat[iStrat].close) m_action[iStrat] = 5; // buy close and sell close
        else if (m_pred >= -m_strat[iStrat].close) m_action[iStrat] = 3; // buy close
        else if (m_pred <= m_strat[iStrat].close) m_action[iStrat] = 4; // sell close
        else m_action[iStrat] = 0; // nothing
    }
}
void Linear::getPosition(ExtendedItr itr)
{
    for (auto iStrat = 0; iStrat != m_strat.size(); ++iStrat) {
        if (itr->m_bid == 0 && m_action[iStrat] == 1)
            m_action[iStrat] = 3; // if lower limit, then sell open becomes sell close
        if (itr->m_ask == 0 && m_action[iStrat] == 2)
            m_action[iStrat] = 4; // if upper limit, then buy open becomes buy close
        switch (m_action[iStrat]) {
        case 1:
            if (itr->m_bid > 0) m_position[iStrat] = m_strat[iStrat].qty; // if not lower limit, buy open
            else if (m_position[iStrat] < 0) m_position[iStrat] = 0; // if lower limit and has short positon, buy close
            break;
        case 2:
            if (itr->m_ask > 0) m_position[iStrat] = -m_strat[iStrat].qty; // if not upper limit, sell open
            else if (m_position[iStrat] > 0) m_position[iStrat] = 0; // if upper limit and has long position, sell close
            break;
        case 3:
            if (m_position[iStrat] < 0 && itr->m_ask>0) m_position[iStrat] = 0; // buy close
            break;
        case 4:
            if (m_position[iStrat] > 0 && itr->m_bid > 0) m_position[iStrat] = 0; // sell close
            break;
        case 5:
            m_position[iStrat] = 0; // other case close position anyway
            break;
        }
    }
    m_totalPos = std::accumulate(m_position.begin(), m_position.end(), 0);
}
void Linear::getPosition(NewData_s& tick)
{
    for (auto iStrat = 0; iStrat != m_strat.size(); ++iStrat) {
        if (tick.BidPrice[0] == 0 && m_action[iStrat] == 1)
            m_action[iStrat] = 3; // if lower limit, then sell open becomes sell close
        if (tick.AskPrice[0] == 0 && m_action[iStrat] == 2)
            m_action[iStrat] = 4; // if upper limit, then buy open becomes buy close
        switch (m_action[iStrat]) {
        case 1:
            if (tick.BidPrice[0] > 0) m_position[iStrat] = m_strat[iStrat].qty; // if not lower limit, buy open
            else if (m_position[iStrat] < 0) m_position[iStrat] = 0; // if lower limit and has short positon, buy close
            break;
        case 2:
            if (tick.AskPrice[0] > 0) m_position[iStrat] = -m_strat[iStrat].qty; // if not upper limit, sell open
            else if (m_position[iStrat] > 0) m_position[iStrat] = 0; // if upper limit and has long position, sell close
            break;
        case 3:
            if (m_position[iStrat] < 0 && tick.AskPrice[0]>0) m_position[iStrat] = 0; // buy close
            break;
        case 4:
            if (m_position[iStrat] > 0 && tick.BidPrice[0] > 0) m_position[iStrat] = 0; // sell close
            break;
        case 5:
            m_position[iStrat] = 0; // other case close position anyway
            break;
        }
    }
    m_totalPos = std::accumulate(m_position.begin(), m_position.end(), 0);

}
void Linear::output(std::string date,std::string posFile, std::vector<int> &position, std::vector<TradeThre> &strat)
{
    /*
    std::ofstream output(m_homePath + "/pred/" + m_product + "/" + date + ".txt");
    output << "pred" << std::endl;
    //output << defaultfloat << setprecision(15);
    output.setf(std::ios::fixed, std::ios::floatfield);
    output.precision(15);
    for (auto pred : m_predAll)
        output << pred << std::endl;
    output.close();
    */
    std::ofstream outputPos(posFile);
    outputPos << "position" << std::endl;
    for (int i = 0; i != strat.size(); ++i)
        outputPos << position[i] << std::endl;
    outputPos.close();
}
void Linear::setup()
{
    m_indMap = NULL;
    // read in coefficient of each signal
    std::ifstream input(m_linearFile);
    std::string head;
    std::getline(input, head);
    std::string item;
    std::string signal;
    double coef;
    // read in signal and coefficients
    std::cout << "\n--------------"<<m_linearFile<<"信息----------------" << std::endl;
    while (true) {
        getline(input, item);
        if (input.fail()) break;
        std::stringstream ss(item);
        ss >> signal >> coef;
        std::cout << signal << " " << coef << std::endl;
        m_coef[signal] = coef;
    }
    input.close();
    m_predAll.reserve(TICK_NUM * 3);
    // read in open, close, and trading qty
    input.open(m_threFile);
    getline(input, head);
    TradeThre newThre;
    std::cout << "\n--------------" << m_threFile << "信息----------------" << std::endl;
    
    while (true) {
        getline(input, item);
        if (input.fail()) break;
        std::stringstream ss(item);
        ss >> newThre.open >> newThre.close >> newThre.qty;
        std::cout << newThre.open << " " << newThre.close << " " << newThre.qty << std::endl;
        m_strat.push_back(newThre);
    }
    input.close();
    m_stratNum = (int)m_strat.size(); // number of strategeis
    m_action = std::vector<int>(m_stratNum, 0); // initialize action
    m_position = std::vector<int>(m_stratNum, 0); // initialize position
    // read in position
    std::ifstream inputPos(m_posFile.c_str());
    getline(inputPos, head);
    int i = 0;
    std::cout << "\n--------------" << m_posFile << "信息----------------" << std::endl;
     
    while (true) {
        getline(inputPos, item);
        if (inputPos.fail()) break;
        std::stringstream ss(item);
        int pos;
        ss >> pos;
        std::cout << pos << std::endl;
        m_position[i++] = pos;
        if (i >= m_stratNum) break;
    }
    //创建pred.csv文件
    std::cout << "\n-----------创建" << m_predFile << "文件-----" << std::endl;
    std::ofstream outFile;

    outFile.open(m_predFile, std::ios::out | std::ios::app); // 打开模式可省略
    if (!outFile.is_open())
    {
        std::cout << m_predFile << "   open  fialed: " << std::endl;
    }
    outFile << "交易日" << ',' << "时间" << ',' << "最后修改时间" << ',' << "交易所代码" << ',' << "pred" << std::endl;
    outFile.close();

    inputPos.close();
    yesterdayPos = accumulate(m_position.begin(), m_position.end(), 0);
}
