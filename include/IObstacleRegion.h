/********************************************************************************
* 障碍物区域接口，用于判断和设置面域是否为障碍物
*********************************************************************************/

#pragma once

class IObstacleRegion
{
public:
    virtual ~IObstacleRegion() = default;

    /*
    判断面域是否为障碍物
    @return 如果面域是障碍物返回true，否则返回false
    */
    virtual bool isObstacle() const = 0;

    /*
    设置面域是否为障碍物
    @param b true表示设为障碍物，false表示设为非障碍物
    */
    virtual void setObstacle(bool b) = 0;
};