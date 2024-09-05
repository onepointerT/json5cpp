#pragma once

#include "json5_builder.hpp"

#include <ctype.h>

#if __has_include(<charconv>)
	#include <charconv>
	#if !defined(_JSON5_HAS_CHARCONV)
		#define _JSON5_HAS_CHARCONV
	#endif
#endif

namespace json5 {

// Parse json5::document from string
error from_string( string_view str, document &doc );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class parser final : builder
{
public:
	parser( document &doc, const char *utf8Str, size_t len = size_t( -1 ) );

	error parse();

private:
	int next();
	int peek() const;
	bool eof() const;
	error make_error( int type ) const noexcept;

	enum class token_type
	{
		unknown, identifier, string, number, colon, comma,
		object_begin, object_end, array_begin, array_end,
		literal_true, literal_false, literal_null, literal_NaN
	};

	error parse_value( detail::value &result );
	error parse_object();
	error parse_array();
	error peek_next_token( token_type &result );
	error parse_number( double &result );
	error parse_string( detail::string_offset &result );
	error parse_identifier( detail::string_offset &result );
	error parse_literal( token_type &result );

	const char *_cursor = nullptr;
	size_t _size = 0;
	location _loc = { };
};

} // namespace json5
