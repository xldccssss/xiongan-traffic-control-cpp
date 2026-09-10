/********************************************************************************
* 控制中心接口，由TESS NG实现，用于控制仿真流程和管理插件加载
*********************************************************************************/

#pragma once
#include <qobject.h>
#include <QString>

#include "tessinterfaces_global.h"

class TESSINTERFACES_EXPORT IControlCentre :
    public QObject
{
	Q_OBJECT
public:
	virtual ~IControlCentre() = default;

	/*
	启动仿真
	*/
	virtual void startSimu();

	/*
	暂停仿真
	*/
	virtual void pauseSimu();

	/*
	停止仿真
	@param stopDS 是否同时停止数据服务，默认为true
	*/
	virtual void stopSimu(bool stopDS = true);

	/*
	设置停止仿真标志
	*/
	virtual void setStopSimuFlags();

	/*
	加载所有插件
	@param tessDirPath TESS安装目录路径，默认为空字符串
	*/
	virtual void loadPlugins(QString tessDirPath = QString());

	/*
	加载单个插件
	@param tessLibPath 插件库文件路径
	*/
	virtual void loadPlugin(QString tessLibPath);

	/*
	打开文件
	*/
	virtual void openFile();

	/*
	在停止仿真时进行清理操作
	*/
	virtual void cleanInStopSimu();

	/*
	激活软件
	@param judgeFileExsit 是否判断文件是否存在
	@return 激活成功返回true，否则返回false
	*/
	virtual bool activeSoftware(bool judgeFileExsit) { return false; }

	/*
	强制激活软件
	@return 激活成功返回true，否则返回false
	*/
	virtual bool activeSoftwareWithTrue() { return false; }

public slots:
	/*
	槽函数：启动仿真
	*/
	virtual void doStartSimu();

	/*
	槽函数：停止仿真
	@param stopDS 是否同时停止数据服务，默认为true
	*/
	virtual void doStopSimu(bool stopDS = true);

	/*
	槽函数：先清理后停止仿真
	先执行清理操作，再停止仿真
	@param stopDS 是否同时停止数据服务，默认为true
	*/
	virtual void doStopSimuAfterClean(bool stopDS = true);

signals:
	/*
	信号：仿真已启动
	*/
	void startedSimu();

	/*
	信号：仿真已暂停
	*/
	void pausedSimu();

	/*
	信号：仿真已停止
	*/
	void stoppedSimu();
};

extern TESSINTERFACES_EXPORT IControlCentre* gpControlCentre;