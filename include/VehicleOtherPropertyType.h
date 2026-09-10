/********************************************************************************
* 车辆其他属性类型定义
* 定义车辆二次开发中可能用到的其他属性类型常量
*********************************************************************************/

#pragma once

#include "tessinterfaces_global.h"

class TESSINTERFACES_EXPORT VehicleOtherPropertyType
{
public:
    virtual ~VehicleOtherPropertyType() = default;

    /* 车辆原始颜色属性类型标识 */
    static const int VehicleOriginalColor;

    /* 交叉口加减速决策器变更加速度属性类型标识 */
    static const int IntersectionAcceDeciderChangeAcce;
};