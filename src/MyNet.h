#pragma once

#include "Plugin/customernet.h"

class ProjectModel;

class MyNet : public CustomerNet
{
public:
    explicit MyNet(ProjectModel* model);

    bool isPermitForCustDraw() override { return false; }
    void afterLoadNet() override;

private:
    ProjectModel* m_model = nullptr;
};
