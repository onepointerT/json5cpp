#pragma once

#include "json5_builder.hpp"

namespace json5 {


builder::builder( document &doc )
	:	_doc( doc )
{}

const document& builder::doc() const noexcept {
	return _doc;
}

detail::value builder::new_string( detail::string_offset stringOffset ) {
	return { value_type::null, detail::value::type_string_off | stringOffset };
}

detail::value builder::new_string( std::string_view str ) {
	return new_string( string_buffer_add( str ) );
}

detail::value& builder::operator[]( std::string_view key ) {
	return ( *this )[string_buffer_add( key )];
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
detail::string_offset builder::string_buffer_offset() const noexcept
{
	return detail::string_offset( _doc._strings.size() );
}

//---------------------------------------------------------------------------------------------------------------------
detail::string_offset builder::string_buffer_add( std::string_view str )
{
	auto offset = string_buffer_offset();
	_doc + str.data();
	_doc._strings.push_back( 0 );
	return offset;
}

void builder::string_buffer_add( char ch ) {
	_doc._strings.push_back( ch );
}

//---------------------------------------------------------------------------------------------------------------------
void builder::string_buffer_add_utf8( uint32_t ch )
{
	if ( 0 <= ch && ch <= 0x7f )
	{
		_doc + char( ch );
	}
	else if ( 0x80 <= ch && ch <= 0x7ff )
	{
		_doc + char( 0xc0 | ( ch >> 6 ) );
		_doc + char( 0x80 | ( ch & 0x3f ) );
	}
	else if ( 0x800 <= ch && ch <= 0xffff )
	{
		_doc + char( 0xe0 | ( ch >> 12 ) );
		_doc + char( 0x80 | ( ( ch >> 6 ) & 0x3f ) );
		_doc + char( 0x80 | ( ch & 0x3f ) );
	}
	else if ( 0x10000 <= ch && ch <= 0x1fffff )
	{
		_doc + char( 0xf0 | ( ch >> 18 ) );
		_doc + char( 0x80 | ( ( ch >> 12 ) & 0x3f ) );
		_doc + char( 0x80 | ( ( ch >> 6 ) & 0x3f ) );
		_doc + char( 0x80 | ( ch & 0x3f ) );
	}
	else if ( 0x200000 <= ch && ch <= 0x3ffffff )
	{
		_doc + char( 0xf8 | ( ch >> 24 ) );
		_doc + char( 0x80 | ( ( ch >> 18 ) & 0x3f ) );
		_doc + char( 0x80 | ( ( ch >> 12 ) & 0x3f ) );
		_doc + char( 0x80 | ( ( ch >> 6 ) & 0x3f ) );
		_doc + char( 0x80 | ( ch & 0x3f ) );
	}
	else if ( 0x4000000 <= ch && ch <= 0x7fffffff )
	{
		_doc + char( 0xfc | ( ch >> 30 ) );
		_doc + char( 0x80 | ( ( ch >> 24 ) & 0x3f ) );
		_doc + char( 0x80 | ( ( ch >> 18 ) & 0x3f ) );
		_doc + char( 0x80 | ( ( ch >> 12 ) & 0x3f ) );
		_doc + char( 0x80 | ( ( ch >> 6 ) & 0x3f ) );
		_doc + char( 0x80 | ( ch & 0x3f ) );
	}
}

//---------------------------------------------------------------------------------------------------------------------
void builder::push_object()
{
	auto v = detail::value( value_type::object, nullptr );
	_stack.emplace_back( v );
	_counts.push_back( 0 );
}

//---------------------------------------------------------------------------------------------------------------------
void builder::push_array()
{
	auto v = detail::value( value_type::array, nullptr );
	_stack.emplace_back( v );
	_counts.push_back( 0 );
}

//---------------------------------------------------------------------------------------------------------------------
detail::value builder::pop()
{
	auto result = _stack.back();
	auto count = _counts.back();

	result.payload( _doc._values.size() );

	_doc._values.push_back( detail::value( double( count ) ) );

	auto startIndex = _values.size() - count;
	for ( size_t i = startIndex, S = _values.size(); i < S; ++i )
		_doc._values.push_back( _values[i] );

	_values.resize( _values.size() - count );

	_stack.pop_back();
	_counts.pop_back();

	if ( _stack.empty() )
	{
		_doc.assign_root( result );
		result = _doc;
	}

	return result;
}

//---------------------------------------------------------------------------------------------------------------------
builder& builder::operator+=( detail::value v )
{
	_values.push_back( v );
	_counts.back() += 1;
	return *this;
}

//---------------------------------------------------------------------------------------------------------------------
detail::value &builder::operator[]( detail::string_offset keyOffset )
{
	_values.push_back( new_string( keyOffset ) );
	_counts.back() += 2;
	return _values.emplace_back();
}

//---------------------------------------------------------------------------------------------------------------------
void builder::reset() noexcept
{
	_doc._data = detail::value::type_null;
	_doc._values.clear();
	_doc._strings.clear();
	_doc._strings.push_back( 0 );
}

} // namespace json5
