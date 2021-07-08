#include "JsonTools.h"

void readFromJson(std::vector <CThostFtdcInstrumentField>& instruments, std::string& filePath) {
    std::ifstream ifs(filePath);
    rapidjson::IStreamWrapper isw(ifs);

    rapidjson::Document docRead;
    docRead.ParseStream(isw);
    if (docRead.HasParseError()) {
        std::cout << "error" << std::endl;
    }
    if (docRead.IsArray()) {
        rapidjson::Value::Array instrumentArr = docRead.GetArray();
        int lengthOfInstrument = instrumentArr.Size();
        for (int i = 0; i < lengthOfInstrument; i++) {
            CThostFtdcInstrumentField field;
            memset(&field, 0, sizeof(CThostFtdcInstrumentField));
            strncpy(field.InstrumentID, instrumentArr[i]["InstrumentID"].GetString(), sizeof(field.InstrumentID));
            strncpy(field.ExchangeID, instrumentArr[i]["ExchangeID"].GetString(), sizeof(field.InstrumentID));
            instruments.push_back(field);
        }
    }
};

void WritToJson(std::vector <CThostFtdcInstrumentField>& instruments, std::string& filePath) {
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter <rapidjson::StringBuffer> writer(s);
    writer.StartArray();
    std::vector<CThostFtdcInstrumentField>::iterator itr;
    for (itr = instruments.begin(); itr != instruments.end(); itr++) {
        writer.StartObject();
        ///合约代码
        writer.Key("InstrumentID");
        writer.String(itr->InstrumentID);
        ///交易所代码
        writer.Key("ExchangeID");
        writer.String(itr->ExchangeID);
        ///合约在交易所的代码
        writer.Key("ExchangeInstID");
        writer.String(itr->ExchangeInstID);
        ///产品代码
        writer.Key("ProductID");
        writer.String(itr->ProductID);
        ///产品类型
        writer.Key("ProductClass");
        writer.String(std::string(1, itr->ProductClass).c_str());
        ///交割年份
        writer.Key("DeliveryYear");
        writer.Int(itr->DeliveryYear);
        ///交割月
        writer.Key("DeliveryMonth");
        writer.Int(itr->DeliveryMonth);
        ///市价单最大下单量
        writer.Key("MaxMarketOrderVolume");
        writer.Int(itr->MaxMarketOrderVolume);
        ///市价单最小下单量
        writer.Key("MinMarketOrderVolume");
        writer.Int(itr->MinMarketOrderVolume);
        ///限价单最大下单量
        writer.Key("MaxLimitOrderVolume");
        writer.Int(itr->MaxLimitOrderVolume);
        ///限价单最小下单量
        writer.Key("MinLimitOrderVolume");
        writer.Int(itr->MinLimitOrderVolume);
        ///合约数量乘数
        writer.Key("VolumeMultiple");
        writer.Int(itr->VolumeMultiple);
        ///最小变动价位
        writer.Key("PriceTick");
        writer.Double(itr->PriceTick);
        ///创建日
        writer.Key("CreateDate");
        writer.String(itr->CreateDate);
        ///上市日
        writer.Key("OpenDate");
        writer.String(itr->OpenDate);
        ///到期日
        writer.Key("ExpireDate");
        writer.String(itr->ExpireDate);
        ///开始交割日
        writer.Key("StartDelivDate");
        writer.String(itr->StartDelivDate);
        ///结束交割日
        writer.Key("EndDelivDate");
        writer.String(itr->EndDelivDate);
        ///合约生命周期状态
        writer.Key("InstLifePhase");
        writer.String(std::string(1, itr->InstLifePhase).c_str());
        ///当前是否交易
        writer.Key("IsTrading");
        writer.Bool(itr->IsTrading);
        ///持仓类型
        writer.Key("PositionType");
        writer.String(std::string(1, itr->PositionType).c_str());
        ///持仓日期类型
        writer.Key("PositionDateType");
        writer.String(std::string(1, itr->PositionDateType).c_str());
        ///多头保证金率
        writer.Key("LongMarginRatio");
        writer.Double(itr->LongMarginRatio);
        ///空头保证金率
        writer.Key("ShortMarginRatio");
        writer.Double(itr->ShortMarginRatio);
        ///是否使用大额单边保证金算法
        writer.Key("MaxMarginSideAlgorithm");
        writer.Double(itr->MaxMarginSideAlgorithm);
        ///基础商品代码
        writer.Key("UnderlyingInstrID");
        writer.String(itr->UnderlyingInstrID);
        ///执行价
        writer.Key("ProductClass");
        writer.String(std::string(1, itr->ProductClass).c_str());
        ///期权类型
        writer.Key("OptionsType");
        writer.String(std::string(1, itr->OptionsType).c_str());
        ///合约基础商品乘数
        writer.Key("UnderlyingMultiple");
        writer.Double(itr->UnderlyingMultiple);
        ///组合类型
        writer.Key("CombinationType");
        writer.String(std::string(1, itr->CombinationType).c_str());

        writer.EndObject();
    }
    writer.EndArray();

    std::ofstream ofs("instrument.json");
    if (ofs.is_open()) {
        ofs << s.GetString() << std::endl;
        ofs.close();
    }
};
