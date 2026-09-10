/***************************************************************************
 * 信号灯干线接口，TESS NG实现，用于管理信号灯干线方案
 ***************************************************************************/

#pragma once

#include <QObject>
#include "tessinterfaces_global.h"


/*
干线结构体，用于存储干线相关信息
*/
struct Mainline
{
    int SignalPlanId;    //信控方案ID 
    int TrafficLightId;  //信号机ID
    int SignalPhaseId;   //上行相位ID
    int SignalPhaseId_2; //下行相位ID
    int distance = -1;   //上行交叉口距离
    int distance_2 = -1; //下行交叉口距离
};

class TESSINTERFACES_EXPORT ITrafficLightMainline : public QObject
{
	Q_OBJECT 

public:
	/*
	构造函数
	@param parent 父对象指针，默认为nullptr
	*/
	ITrafficLightMainline(QObject* parent = nullptr);

	/*
	析构函数
	*/
	virtual ~ITrafficLightMainline();

	/*
	获取ID
	@return 返回ID
	*/
    virtual int id();

	/*
	获取名称
	@return 返回名称
	*/
    virtual QString name();

	/*
	设置名称
	@param name 名称
	*/
    virtual void setName(QString name);

	/*
	获取干线列表
	@return 返回干线列表
	*/
    virtual QList<Mainline> mainlines();  

	/*
	添加干线
	@param tMainline 干线结构体
	*/
    virtual void addMainlines(Mainline tMainline);

	/*
	移除所有干线
	*/
    virtual void removeAllMainlines();

};

      