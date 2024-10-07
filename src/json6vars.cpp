
#include "json6vars.hpp"

namespace json6 {



JsonDocument::JsonDocument( const json5::document* doc )
    :   document( doc )
{}



const json5::document* JsonDocument::transform() const {
    

    unsigned int varpos = 0;
    std::string docstr( document->get_c_str() );
    unsigned int count_vars = 0;
    for ( varpos = docstr.find_first_of("##", varpos); varpos < docstr.length()-1; ++count_vars ) {}

    varpos = 0;
    unsigned int worked_on_vars = 0;
    while ( varpos != docstr.npos && worked_on_vars < count_vars ) {
        varpos = docstr.find_first_of("##", varpos);
        std::string varname = docstr.substr( varpos+2, docstr.find_first_of(" ", varpos)-varpos );

        json5::detail::value val = (*document)[varname];
        docstr.replace( varpos, 2+varname.length(), val.get_c_str() );
    }

    json5::document* doc = new json5::document( docstr.c_str() );

    return doc;
}


} // namespace json5
