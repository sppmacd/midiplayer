#include "Property.h"

namespace Config
{

bool PropertyFormalParameter::value_matches(PropertyParameter const& param) const
{
    if(!m_match_expression)
        return true;
    if(type() == PropertyType::Float)
        return m_match_expression->matches(param.as_float());
    if(type() == PropertyType::Int)
        return m_match_expression->matches(param.as_int());
    return true; // Not supported for now. All values matches.
}

std::string_view PropertyFormalParameter::type_string() const
{
    switch(type())
    {
        case PropertyType::Int:
            return "int";
        case PropertyType::Float:
            return "float";
        case PropertyType::String:
            return "string";
        case PropertyType::ColorRGB:
            return "Color<RGB>";
        case PropertyType::ColorRGBA:
            return "Color<RGBA>";
        case PropertyType::SelectorList:
            return "SelectorList";
        default:
            return "invalid";
    }
}

}
