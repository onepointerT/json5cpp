#pragma once

#include "json5_input.hpp"

#include "json5_builder.hpp"

#include <ctype.h>


namespace json5 {

// Parse json5::document from string
error from_string( string_view str, document &doc )  {
	parser r( doc, str.data(), str.size() );
	return r.parse();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool parser::eof() const {
	return _size == 0;
}
error parser::make_error( int type ) const noexcept {
	return error{ type, _loc };
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
parser::parser( document &doc, const char *utf8Str, size_t len )
	: builder( doc )
	, _cursor( utf8Str )
{
	if ( _cursor && len == size_t( -1 ) )
		_size = strlen( _cursor );
	else
		_size = len;
}

//---------------------------------------------------------------------------------------------------------------------
error parser::parse()
{
	reset();

	_loc = { };

	if ( _cursor && _size )
		_loc = { 1, 1, 0 };

	if ( auto err = parse_value( _doc ) )
		return err;

	if ( !_doc.is_array() && !_doc.is_object() )
		return make_error( error::invalid_root );

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
int parser::next()
{
	if ( _size == 0 )
		return -1;

	int ch = uint8_t( *_cursor++ );

	if ( ch == '\n' )
	{
		_loc.column = 0;
		++_loc.line;
	}

	++_loc.column;
	++_loc.offset;
	--_size;
	return ch;
}

//---------------------------------------------------------------------------------------------------------------------
int parser::peek() const
{
	if ( _size == 0 )
		return -1;

	return uint8_t( *_cursor );
}

//---------------------------------------------------------------------------------------------------------------------
error parser::parse_value( detail::value &result )
{
	token_type tt = token_type::unknown;
	if ( auto err = peek_next_token( tt ) )
		return err;

	location loc = _loc;

	switch ( tt )
	{
		case token_type::number:
		{
			if ( double number = 0.0; auto err = parse_number( number ) )
				return err;
			else
				result = detail::value( number );
		}
		break;

		case token_type::string:
		{
			if ( detail::string_offset offset = 0; auto err = parse_string( offset ) )
				return err;
			else
				result = new_string( offset );
		}
		break;

		case token_type::identifier:
		{
			if ( token_type lit = token_type::unknown; auto err = parse_literal( lit ) )
				return err;
			else
			{
				if ( lit == token_type::literal_true )
					result = detail::value( true );
				else if ( lit == token_type::literal_false )
					result = detail::value( false );
				else if ( lit == token_type::literal_null )
					result = detail::value();
				else if ( lit == token_type::literal_NaN )
					result = detail::value( NAN );
				else
					return make_error( error::invalid_literal );
			}
		}
		break;

		case token_type::object_begin:
		{
			push_object();
			{
				if ( auto err = parse_object() )
					return err;
			}
			result = pop();
		}
		break;

		case token_type::array_begin:
		{
			push_array();
			{
				if ( auto err = parse_array() )
					return err;
			}
			result = pop();
		}
		break;

		default:
			return make_error( error::syntax_error );
	}

	result._loc = loc;
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
error parser::parse_object()
{
	next(); // Consume '{'

	bool expectComma = false;
	while ( !eof() )
	{
		token_type tt = token_type::unknown;
		if ( auto err = peek_next_token( tt ) )
			return err;

		detail::string_offset keyOffset;
		location keyLoc = { };

		switch ( tt )
		{
			case token_type::identifier:
			case token_type::string:
			{
				if ( expectComma )
					return make_error( error::comma_expected );

				keyLoc = _loc;
				if ( auto err = parse_identifier( keyOffset ) )
					return err;
			}
			break;

			case token_type::object_end:
				next(); // Consume '}'
				return { error::none };

			case token_type::comma:
				if ( !expectComma )
					return make_error( error::syntax_error );

				next(); // Consume ','
				expectComma = false;
				continue;

			default:
				return expectComma ? make_error( error::comma_expected ) : make_error( error::syntax_error );
		}

		if ( auto err = peek_next_token( tt ) )
			return err;

		if ( tt != token_type::colon )
			return make_error( error::colon_expected );

		next(); // Consume ':'

		detail::value newValue;
		if ( auto err = parse_value( newValue ) )
			return err;

		detail::value key = new_string( keyOffset );
		key._loc = keyLoc;

		( *this )( key, newValue );
		expectComma = true;
	}

	return make_error( error::unexpected_end );
}

//---------------------------------------------------------------------------------------------------------------------
error parser::parse_array()
{
	next(); // Consume '['

	bool expectComma = false;
	while ( !eof() )
	{
		token_type tt = token_type::unknown;
		if ( auto err = peek_next_token( tt ) )
			return err;

		if ( tt == token_type::array_end && next() ) // Consume ']'
			return { error::none };
		else if ( expectComma )
		{
			expectComma = false;

			if ( tt != token_type::comma )
				return make_error( error::comma_expected );

			next(); // Consume ','
			continue;
		}

		detail::value newValue;
		if ( auto err = parse_value( newValue ) )
			return err;

		add_item( newValue );
		expectComma = true;
	}

	return make_error( error::unexpected_end );
}

//---------------------------------------------------------------------------------------------------------------------
error parser::peek_next_token( token_type &result )
{
	enum class comment_type { none, line, block } parsingComment = comment_type::none;

	while ( !eof() )
	{
		int ch = peek();
		if ( ch == '\n' )
		{
			if ( parsingComment == comment_type::line )
				parsingComment = comment_type::none;
		}
		else if ( parsingComment != comment_type::none || ( ch > 0 && ch <= 32 ) )
		{
			if ( parsingComment == comment_type::block && ch == '*' && next() ) // Consume '*'
			{
				if ( peek() == '/' )
					parsingComment = comment_type::none;
			}
		}
		else if ( ch == '/' && next() ) // Consume '/'
		{
			if ( peek() == '/' )
				parsingComment = comment_type::line;
			else if ( peek() == '*' )
				parsingComment = comment_type::block;
			else
				return make_error( error::syntax_error );
		}
		else if ( strchr( "{}[]:,", ch ) )
		{
			if ( ch == '{' )
				result = token_type::object_begin;
			else if ( ch == '}' )
				result = token_type::object_end;
			else if ( ch == '[' )
				result = token_type::array_begin;
			else if ( ch == ']' )
				result = token_type::array_end;
			else if ( ch == ':' )
				result = token_type::colon;
			else if ( ch == ',' )
				result = token_type::comma;

			return { error::none };
		}
		else if ( isalpha( ch ) || ch == '_' )
		{
			result = token_type::identifier;
			return { error::none };
		}
		else if ( isdigit( ch ) || ch == '.' || ch == '+' || ch == '-' )
		{
			if ( ch == '+' ) next(); // Consume leading '+'

			result = token_type::number;
			return { error::none };
		}
		else if ( ch == '"' || ch == '\'' )
		{
			result = token_type::string;
			return { error::none };
		}
		else
			return make_error( error::syntax_error );

		next();
	}

	return make_error( error::unexpected_end );
}

//---------------------------------------------------------------------------------------------------------------------
error parser::parse_number( double &result )
{
	char buff[256] = { };
	size_t length = 0;

	while ( !eof() && length < sizeof( buff ) )
	{
		buff[length++] = next();

		int ch = peek();
		if ( ( ch > 0 && ch <= 32 ) || ch == ',' || ch == '}' || ch == ']' )
			break;
	}

#if defined(_JSON5_HAS_CHARCONV)
	auto convResult = std::from_chars( buff, buff + length, result );

	if ( convResult.ec != std::errc() )
		return make_error( error::syntax_error );
#else
	char *buffEnd = nullptr;
	result = strtod( buff, &buffEnd );

	if ( result == 0.0 && buffEnd == buff )
		return make_error( error::syntax_error );
#endif

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
error parser::parse_string( detail::string_offset &result )
{
	static const constexpr char *hexChars = "0123456789abcdefABCDEF";

	bool singleQuoted = peek() == '\'';
	next(); // Consume '\'' or '"'

	result = string_buffer_offset();

	while ( !eof() )
	{
		int ch = peek();
		if ( ( ( singleQuoted && ch == '\'' ) || ( !singleQuoted && ch == '"' ) ) && next() ) // Consume '\'' or '"'
			break;
		else if ( ch == '\\' && next() ) // Consume '\\'
		{
			ch = peek();
			if ( ch == '\n' || ch == 'v' || ch == 'f' )
				next();
			else if ( ch == 't' && next() )
				string_buffer_add( '\t' );
			else if ( ch == 'n' && next() )
				string_buffer_add( '\n' );
			else if ( ch == 'r' && next() )
				string_buffer_add( '\r' );
			else if ( ch == 'b' && next() )
				string_buffer_add( '\b' );
			else if ( ch == '\\' && next() )
				string_buffer_add( '\\' );
			else if ( ch == '\'' && next() )
				string_buffer_add( '\'' );
			else if ( ch == '"' && next() )
				string_buffer_add( '"' );
			else if ( ch == '\\' && next() )
				string_buffer_add( '\\' );
			else if ( ch == '/' && next() )
				string_buffer_add( '/' );
			else if ( ch == '0' && next() )
				string_buffer_add( 0 );
			else if ( ( ch == 'x' || ch == 'u' ) && next() )
			{
				char code[5] = { };

				for ( size_t i = 0, S = ( ch == 'x' ) ? 2 : 4; i < S; ++i )
					if ( !strchr( hexChars, code[i] = char( next() ) ) )
						return make_error( error::invalid_escape_seq );

				uint64_t unicodeChar = 0;

#if defined(_JSON5_HAS_CHARCONV)
				std::from_chars( code, code + 5, unicodeChar, 16 );
#else
				char *codeEnd = nullptr;
				unicodeChar = strtoull( code, &codeEnd, 16 );

				if ( !unicodeChar && codeEnd == code )
					return make_error( error::invalid_escape_seq );
#endif

				string_buffer_add_utf8( uint32_t( unicodeChar ) );
			}
			else
				return make_error( error::invalid_escape_seq );
		}
		else
			string_buffer_add( next() );
	}

	if ( eof() )
		return make_error( error::unexpected_end );

	string_buffer_add( 0 );
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
error parser::parse_identifier( detail::string_offset &result )
{
	result = string_buffer_offset();

	int firstCh = peek();
	bool isString = ( firstCh == '\'' ) || ( firstCh == '"' );

	if ( isString && next() ) // Consume '\'' or '"'
	{
		int ch = peek();
		if ( !isalpha( ch ) && ch != '_' )
			return make_error( error::syntax_error );
	}

	while ( !eof() )
	{
		string_buffer_add( next() );

		int ch = peek();
		if ( !isalpha( ch ) && !isdigit( ch ) && ch != '_' )
			break;
	}

	if ( isString && firstCh != next() ) // Consume '\'' or '"'
		return make_error( error::syntax_error );

	string_buffer_add( 0 );
	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
error parser::parse_literal( token_type &result )
{
	int ch = peek();

	// "true"
	if ( ch == 't' )
	{
		if ( next() && next() == 'r' && next() == 'u' && next() == 'e' )
		{
			result = token_type::literal_true;
			return { error::none };
		}
	}
	// "false"
	else if ( ch == 'f' )
	{
		if ( next() && next() == 'a' && next() == 'l' && next() == 's' && next() == 'e' )
		{
			result = token_type::literal_false;
			return { error::none };
		}
	}
	// "null"
	else if ( ch == 'n' )
	{
		if ( next() && next() == 'u' && next() == 'l' && next() == 'l' )
		{
			result = token_type::literal_null;
			return { error::none };
		}
	}
	// "NaN"
	else if ( ch == 'N' )
	{
		if ( next() && next() == 'a' && next() == 'N' )
		{
			result = token_type::literal_NaN;
			return { error::none };
		}
	}


	return make_error( error::invalid_literal );
}


} // namespace json5
