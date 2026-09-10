/********************************************************************************
* 度量单位接口，此接口由TESS NG实现，用户可以通过此接口设置度量单位、进行不同度量单位之间数据转换
*********************************************************************************/

#ifndef __UnitChange__
#define __UnitChange__

#include "tessinterfaces_global.h"

#include <QObject>
#include <QPointF>
#include <QMainWindow>

/*
度量单位枚举
Default: 像素单位（默认单位）
Pixel: 像素单位
Metric: 米制单位
*/
enum TESSINTERFACES_EXPORT UnitOfMeasure { Default, Pixel, Metric };

// extern bool TESSINTERFACES_EXPORT gbIsMetric;

//将以米为度量单位的数据转换成以像素为度量单位的数据
/*
米转像素转换函数
@param value 以米为单位的数值
@return 返回转换后的像素值
*/
qreal TESSINTERFACES_EXPORT m2p(qreal value);

/*
像素转米转换函数
@param value 以像素为单位的数值
@return 返回转换后的米值
*/
qreal TESSINTERFACES_EXPORT p2m(qreal value);

#endif