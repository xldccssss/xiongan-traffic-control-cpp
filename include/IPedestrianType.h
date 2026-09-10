/******************************************************************************
* 行人类型结构体，用于定义行人的基本物理属性和行为特征
*******************************************************************************/

#pragma once

class IPedestrianType
{
public:
    /* 行人类型代码 */
    long pedestrianTypeCode;

    /* 模板ID */
    long templateCode{ 1 };

    /* 行人类型名称 */
    QString pedestrianTypeName;

    /* 行人半径（米） */
    qreal radius;

    /* 行人半径标准差 */
    qreal radiusStandardDeviation;

    /* 行人期望速度（米/秒） */
    qreal desiredSpeed;

    /* 行人期望速度标准差 */
    qreal desiredSpeedStandardDeviation;

    /* 行人重量（千克） */
    qreal weight;

    /* 行人重量标准差 */
    qreal weightStandardDeviation;
};
