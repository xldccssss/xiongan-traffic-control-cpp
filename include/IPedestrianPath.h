/******************************************************************************
* 行人路径接口，由TESS NG实现，用于管理行人路径的基本信息
*******************************************************************************/

#pragma once

class IPedestrianPathPoint;

class IPedestrianPath
{
public:
    virtual ~IPedestrianPath() = default;

    /*
    获取行人路径ID
    @return 返回路径ID
    */
    virtual long getId() const = 0;

    /*
    获取行人路径起始点
    @return 返回路径起始点接口指针
    */
    virtual IPedestrianPathPoint* getPathStartPoint() const = 0;

    /*
    获取行人路径终点
    @return 返回路径终点接口指针
    */
    virtual IPedestrianPathPoint* getPathEndPoint() const = 0;

    /*
    获取行人路径中间点
    @return 返回路径中间点接口指针列表
    */
    virtual QList<IPedestrianPathPoint*> getPathMiddlePoints() const = 0;

    /*
    判断是否是局部路径
    @return 是局部路径返回true，否则返回false
    */
    virtual bool isLocalPath() const = 0;
};
