#ifndef INDICATORH // we use macro here to avoid define the class more than one tim
#define INDICATORH

#include <vector>
#include <deque>
#include <array>
#include <string>
#include <map>
#include <cmath>
#include "orderBook.h"

using namespace std;

typedef std::array<double, TICK_NUM * 3> IndVec; // array of signal values, up to 3 days;
typedef vector<IndVec> IndMat; // matrix of signals,
typedef vector<double>::iterator IndItr; // iterator of each signal

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


class Indicator {
public:
    Indicator(const string signalFile) :
        m_signalFile(signalFile) {
        setup();
    }
    void setup();
    string m_signalFile;
    string m_product; // product of the signal
    string m_homePath; // path of home
    double m_adjust; // to adjust ewma
    map<string, double> m_indMap; // newest value of an indicator
    vector<double> m_signalVec; // newest values of all indicators
    vector<double> m_middleVec; // middle values used to calcualte final indicators;
    vector<double> m_wpr; // save the weighted price
    vector<int> m_cqty;
    vector<double> m_coi;// save the openint
    deque<int> m_maxQi; // save maximum position of wpr
    deque<int> m_minQi; // save minimum position of wpr
    deque<double> m_retQue;
    int m_period;// period of the signal
    double m_lambda;
    IndMat m_indMat; // historical values of all indicators
    int m_tickCount; // count the number of ticks;
    void updateWarmup(ExtendedItr tick); // update signal values in warmup period
    void updateIndicator(ExtendedItr tick); //update signal values of each tick in real time
    void outputIndicator(string date);

    inline double wpr(ExtendedItr itr) {
        return ((itr->m_bidQty == 0 || itr->m_askQty == 0) ? itr->m_price : ((itr->m_bid * itr->m_askQty + itr->m_ask * itr->m_bidQty) / (itr->m_bidQty + itr->m_askQty)));
    }

    inline double& ewma(IndItr ema, double x, double ratio) {// calculate ewma using ratio directly, we use inline to increase speed because it's short and being used frequently
        return (*ema = *ema * (1 - ratio) + x * ratio);
    }

    double& ewmaAdjust(IndItr ema, IndItr emaAdjust, double x, double ratio, int n); // using ewma with adjust

    inline double vanish(double& value, double threshold) { // vanish function
        return(value = (fabs(value) > threshold ? 0 : value));
    }

    inline double zeroDiv(double x, double y) { // the "%0/%" in R, if divided by zero then return 0
        return (fabs(y) < 1e-6 ? 0. : x / y);
    }

    // now we can write our first signal, the same as x2 in week.6.r
    // we follow the same way in R so that it's more clear
    inline int sign(double x) {
        if (fabs(x) < 1e-8) return 0;
        else if (x > 0) return 1;
        else return -1;
    }
    void totalImb(ExtendedItr itr, IndItr signal, IndItr ema, double ratio, double thre, int n);
    void tradeImb(ExtendedItr itr, IndItr signal, IndItr numerator, IndItr denominator, double ratio);
    void BSImb(ExtendedItr itr, IndItr signal, IndItr ema, double ratio, int n);
    void nr(IndItr signal, double ret, IndItr nuEMA, IndItr nuAdjust, IndItr deEMA, IndItr deAdjust, double ratio, int n, double thre);
    void rangePos(IndItr signal, IndItr ema, double wpr, double maxPeriod, double minPeriod, double ratio, int n);
    void dbook(ExtendedItr itr, IndItr signal, IndItr ema, double ratio, int n, int adjust);
    void rsi(double ret, IndItr signal, IndItr upTotal, IndItr moveTotal, double ratio, double thre);
    void priceOsci(double wpr, IndItr signal, IndItr shortEMA, IndItr shortAdjust, IndItr longEMA, IndItr longAdjust,
        double maxPeriod, double minPeriod, double shortRatio, double ratio, double thre, int n);
    void kdjK(double wpr, IndItr signal, IndItr ema, double maxPeriod, double minPeriod, double ratio, int n);
    void kdjJ(double kdjK, IndItr signal, IndItr ema, double ratio, int n);
    void fastRollCor(IndItr signal, deque<double>& x, deque<double>& y, int period);

    // void cumVolumeOpenint(IndItr signal,  IndItr ma, IndItr ma2);
    void cumVolumeOpenint(IndItr signal, IndItr ma, IndItr ma2);
    void maDif10(IndItr signal, IndItr shortEMA, IndItr shortAdjust, double shortRatio,
        IndItr longEMA, IndItr longAdjust, double ratio);
    void trendIndex(IndItr signal, double maxPeriod, double minPeriod);
    void totaltradeImb(ExtendedItr itr, IndItr signal, IndItr nuEMA, IndItr nuAdjust, IndItr deEMA,
        IndItr deAdjust, double ratio, int n, double thre);
    inline void rangePeriod(IndItr signal, double maxPeriod, double minPeriod) {
        *signal = maxPeriod - minPeriod;
    }
    void fastRollVar(IndItr signal, IndItr ma, IndItr ma2);
    inline void stdPeriod(IndItr signal, double rollVar) {
        *signal = sqrt(rollVar);
    }
    inline void compositeIndicator(IndItr signal, const string& s1, const string& s2) {
        *signal = m_indMap[s1] * m_indMap[s2];
    }
    inline void absNr(IndItr signal) {
        *signal = fabs(m_indMap["nr." + to_string(m_period)]);
    }
    ssize_t m_nIndicator;
    vector<string> m_signalName;
    map<string, IndicatorPara> m_para;
};

typedef shared_ptr<Indicator> IndicatorPtr;

#endif
