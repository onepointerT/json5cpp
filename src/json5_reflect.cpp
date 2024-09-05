#pragma once

#include "json5_reflect.hpp"

#include "json5_builder.hpp"

#if !defined( JSON5_DO_NOT_USE_STL )
	#include <array>
	#include <map>
	#include <unordered_map>
#endif

namespace json5::detail {


writer::writer( document &doc, const writer_params &wp )
	:	builder( doc )
	,	_params( wp )
{}

const writer_params& writer::params() const noexcept {
	return _params;
}


//---------------------------------------------------------------------------------------------------------------------
string_view get_name_slice( const char *names, size_t index ) {
	size_t numCommas = index;
	while ( numCommas > 0 && *names )
		if ( *names++ == ',' )
			--numCommas;

	while ( *names && *names <= 32 )
		++names;

	size_t length = 0;
	while ( names[length] > 32 && names[length] != ',' )
		++length;

	return string_view( names, length );
}


//---------------------------------------------------------------------------------------------------------------------
value write( writer &w, bool in ) { return value( in ); }
value write( writer &w, int in ) { return value( double( in ) ); }
value write( writer &w, unsigned in ) { return value( double( in ) ); }
value write( writer &w, float in ) { return value( double( in ) ); }
value write( writer &w, double in ) { return value( in ); }
value write( writer &w, const char *in ) { return w.new_string( in ); }
value write( writer &w, const string &in ) { return w.new_string( in ); }

//---------------------------------------------------------------------------------------------------------------------
error read( const value &in, bool &out ) {
	if ( !in.is_boolean() )
		return { error::number_expected, in.loc() };

	out = in.get_bool();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
error read( const value &in, int &out ) { return read_number( in, out ); }
error read( const value &in, unsigned &out ) { return read_number( in, out ); }
error read( const value &in, float &out ) { return read_number( in, out ); }
error read( const value &in, double &out ) { return read_number( in, out ); }

//---------------------------------------------------------------------------------------------------------------------
error read( const value &in, const char *&out ) {
	if ( !in.is_string() )
		return { error::string_expected, in.loc() };

	out = in.get_c_str();
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
error read( const value &in, string &out ) {
	if ( !in.is_string() )
		return { error::string_expected, in.loc() };

	out = in.get_c_str();
	return { error::none };
}


} // json5::detail
