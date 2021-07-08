#ifndef LINEAR_H
#define LINEAR_H

#include "indicator.h"
#include "GenData.h"

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <iostream>



class Linear {
public:
    Linear(std::string linearFile, std::string threFile, std::string posFile, std::string predFile) : m_linearFile(linearFile),
        m_threFile(threFile), m_posFile(posFile), m_predFile(predFile) {
        setup();
    };
    ~Linear() {}
    void getPred(); // get the prediction based on linear combination
    void getAction(); // trading action of each strategy
    void getPosition(ExtendedItr itr); // position of each strategy
    void getPosition(NewData_s &tick); // position of each strategy
    void output(std::string date, std::string posFile, std::vector<int>& position, std::vector<TradeThre>& strat); //output the prediction value
    void setup();
    std::map<std::string, double> m_coef; // coefficent of each signal
    std::vector<TradeThre> m_strat; // trading threshold and qty
    std::vector<int> m_action; // trading action of each strategy
    std::vector<int> m_position; // position of each strategy
    int m_totalPos; // total position;
    std::string m_linearFile; // file of linear moel information
    std::string m_threFile; // file of threshold inforamtion
    std::string m_posFile; // position file
    std::string m_homePath; //home address
    std::string m_predFile;
    std::string m_product;
    std::map<std::string, double>* m_indMap; // newest value of each signal
    double m_pred; // newest prediction value;
    int yesterdayPos;
    int m_stratNum; // total number of strategies
    std::vector<double> m_predAll; // all of the prediction value


};

typedef std::shared_ptr<Linear> LinearPtr;


#endif // !LINEAR_H

