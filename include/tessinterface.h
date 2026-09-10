/********************************************************************************
* TESS NG顶层接口，此接口由TESS NG实现，用户可以通过此接口获取三个子接口、从内存加载插件、释放插件
*********************************************************************************/

#ifndef TESSINTERFACE_H
#define TESSINTERFACE_H

#include "tessinterfaces_global.h"

#include <QObject>
#include <QJsonObject>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QtCore/qglobal.h>

class GraphicsView;
class GraphicsScene;
class GuiInterface;
class NetInterface;
class SimuInterface;
class TessPlugin;

/*
TESSNG 的顶级接口，提供三个子接口用于访问或控制路网、仿真过程和界面：
- NetInterface: 路网接口，用于访问和管理路网数据
- SimuInterface: 仿真接口，用于控制仿真过程和获取仿真数据
- GuiInterface: 界面接口，用于控制和定制用户界面
*/
class TESSINTERFACES_EXPORT TessInterface : public QObject
{
	Q_OBJECT

public:
	/*
	构造函数
	初始化 TESS 接口实例
	*/
	TessInterface();

	/*
	析构函数
	清理 TESS 接口资源
	*/
	virtual ~TessInterface();

	/*
	初始化数据
	在加载路网前调用此方法进行必要的数据初始化
	*/
	virtual void initData();

	/*
	获取配置信息
	获取json对象，其中保存了config.json配置文件中的信息
	每次加载路网时会重新加载配置信息，上次通过setConfigProperty()方法设置的属性会在重新加载路网后丢失
	@return 返回包含配置信息的QJsonObject对象
	*/
	virtual QJsonObject config();

	/*
	设置配置属性
	@param key 属性键名
	@param value 属性值
	*/
	virtual void setConfigProperty(QString key, QVariant value);

	/*
	获取TESSNG节点标识
	@return 返回节点标识
	*/
	virtual int workerKey();

	/*
	获取当前状态信息
	便于在网页端查看当前系统状态
	@return 返回当前状态信息字符串
	*/
	virtual QString currInfo();

	/*
	从内存加载插件
	@param pPlugin 插件指针
	@return 加载成功返回true，否则返回false
	*/
	virtual bool loadPluginFromMem(TessPlugin *pPlugin);

	/*
	卸载并释放所有插件
	*/
	virtual void releasePlugins();

	/*
	获取当前授权用户账户
	@return 返回当前key的授权用户账户名
	*/
	virtual QString activeKeyAccount();

	/*
	获取当前授权用户名
	@return 返回当前key的授权用户名
	*/
	virtual QString activeKeyName();

	/*
	获取界面接口
	@return 返回GuiInterface接口指针
	*/
	virtual GuiInterface *guiInterface();

	/*
	获取路网接口
	@return 返回NetInterface接口指针
	*/
	virtual NetInterface *netInterface();

	/*
	获取仿真接口
	@return 返回SimuInterface接口指针
	*/
	virtual SimuInterface *simuInterface();

private:

};

/*
全局TESS接口指针，用于访问TESS NG的所有功能
*/
extern TessInterface TESSINTERFACES_EXPORT *gpTessInterface;

/*
启动TESS NG应用程序
@param netFilePath 路网文件路径，默认为空
@return 返回主窗口指针
*/
extern QMainWindow TESSINTERFACES_EXPORT *tessng(QString netFilePath = QString());

/*
获取TESSNG主窗口指针
@param argc 命令行参数数量
@param argv 命令行参数数组
@param netFilePath 路网文件路径，默认为空
@return 返回主窗口指针
*/
extern QMainWindow TESSINTERFACES_EXPORT *tessng(int argc, char* argv[], QString netFilePath = QString());

#endif // TESSINTERFACE_H