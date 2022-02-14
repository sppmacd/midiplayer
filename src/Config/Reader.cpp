#include "Reader.h"

#include "../Logger.h"

#include <fstream>
#include <iostream>

namespace Config
{

void Reader::clear()
{
    m_conditional_actions.clear();
}

void Reader::update()
{
    for(auto const& action : m_conditional_actions)
    {
        if(action.condition->is_met(*this))
            action.action->execute(*this);
    }
}

}
