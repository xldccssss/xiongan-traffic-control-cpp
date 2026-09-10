/********************************************************************************
* TESS NG 接口主类
* 提供 TESS NG 仿真软件的接口访问入口
*********************************************************************************/

#ifndef __TessInterfaces__
#define __TessInterfaces__

#include "tessinterfaces_global.h"
#include "tessinterface.h"

class TESSINTERFACES_EXPORT TessInterfaces
{
public:
	/*
	构造函数
	初始化 TESS 接口实例
	*/
	TessInterfaces();

	/*
	析构函数
	清理 TESS 接口资源
	*/
	virtual ~TessInterfaces() = default;
};

#endif