#pragma once

#include "json5.hpp"

#include <cinttypes>

namespace json5 {

// Converts json5::document to string
void to_string( string &str, const document &doc, const writer_params &wp = writer_params() );

// Returns json5::document converted to string
string to_string( const document &doc, const writer_params &wp = writer_params() );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
void to_string( string &str, const char *utf8Str, char quotes, bool escapeUnicode );

//---------------------------------------------------------------------------------------------------------------------
void to_string( string &str, const detail::value &v, const writer_params &wp, int depth );

//---------------------------------------------------------------------------------------------------------------------
string to_string( const document &doc, const writer_params &wp );

} // namespace json5
