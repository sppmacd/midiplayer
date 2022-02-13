#pragma once

#include "Configuration.h"
#include "Info.h"

namespace Config
{

class Reader
{
public:
    explicit Reader(Info& info)
    : m_info(info) {}

    Info& info() { return m_info; }

private:
    Info& m_info;
};

}
