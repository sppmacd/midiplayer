#include "Statement.h"

#include "Reader.h"

namespace Config
{

bool PropertyStatement::execute(Reader& reader) const
{
    reader.info().set_property(m_name, m_args);
    return true;
}

}
