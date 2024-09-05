#pragma once

#include "json5.hpp"

namespace json5::detail {

/*

json5::detail::value

*/
// Construct null value
value::value() noexcept
	:	_data( type_null )
{}

value::value( value_type t, uint64_t data ) {
	if ( t == value_type::object )
		_data = type_object | data;
	else if ( t == value_type::array )
		_data = type_array | data;
	else if ( t == value_type::string )
		_data = type_string | data;
	else
		_data = data;
}

value::value( value_type t, const void *data )
	:	value( t, reinterpret_cast<uint64_t>( data ) )
{}

// Construct null value
value::value( std::nullptr_t ) noexcept
	:	_data( type_null )
{}

// Construct boolean value
value::value( bool val ) noexcept
	:	_data( val ? type_true : type_false )
{}

// Construct number value from int (will be converted to double)
value::value( int val ) noexcept
	:	_double( val )
{}

// Construct number value from double
value::value( double val ) noexcept
	:	_double( val )
{}

// Construct string value from null-terminated string
value::value( const char *val ) noexcept
	:	value( value_type::string, val )
{}

// Return value type
json5::value_type value::type() const noexcept {
	if ( ( _data & mask_nanbits ) != mask_nanbits )
		return value_type::number;

	if ( ( _data & mask_type ) == type_object )
		return value_type::object;
	else if ( ( _data & mask_type ) == type_array )
		return value_type::array;
	else if ( ( _data & mask_type ) == type_string )
		return value_type::string;
	if ( _data == type_true || _data == type_false )
		return value_type::boolean;

	return value_type::null;
}

// Checks, if value is null
bool value::is_null() const noexcept {
	return _data == type_null;
}

// Checks, if value stores boolean. Use 'get_bool' for reading.
bool value::is_boolean() const noexcept {
	return _data == type_true || _data == type_false;
}

// Checks, if value stores number. Use 'get_number' or 'try_get_number' for reading.
bool value::is_number() const noexcept {
	return ( _data & mask_nanbits ) != mask_nanbits;
}

// Checks, if value stores string. Use 'get_c_str' for reading.
bool value::is_string() const noexcept {
	return ( _data & mask_type ) == type_string;
}

// Checks, if value stores JSON object. Use 'object_view' wrapper
// to iterate over key-value pairs (properties).
bool value::is_object() const noexcept {
	return ( _data & mask_type ) == type_object;
}

// Checks, if value stores JSON array. Use 'array_view' wrapper
// to iterate over array elements.
bool value::is_array() const noexcept {
	return ( _data & mask_type ) == type_array;
}

// Check, if this is a root document value and can be safely casted to json5::document
bool value::is_document() const noexcept {
	return ( _data & mask_is_document ) == mask_is_document;
}

// Get stored bool. Returns 'defaultValue', if this value is not a boolean.
bool value::get_bool( bool defaultValue  ) const noexcept {
	if ( _data == type_true )
		return true;
	else if ( _data == type_false )
		return false;

	return defaultValue;
}

// Get stored string. Returns 'defaultValue', if this value is not a string.
const char* value::get_c_str( const char *defaultValue ) const noexcept  {
	return is_string() ? payload<const char *>() : defaultValue;
}

// Equality test against another value. Note that this DOES NOT DO a deep equality test
// for JSON objects nor a item-by-item equality test for arrays - only references are
// compared for objects or arrays.
bool value::operator==( const value &other ) const noexcept {
	if ( auto t = type(); t == other.type() ) {
		if ( t == value_type::null )
			return true;
		else if ( t == value_type::boolean )
			return _data == other._data;
		else if ( t == value_type::number )
			return _double == other._double;
		else if ( t == value_type::string )
			return string_view( payload<const char *>() ) == string_view( other.payload<const char *>() );
		else if ( t == value_type::array )
			return array_view( *this ) == array_view( other );
		else if ( t == value_type::object )
			return object_view( *this ) == object_view( other );
	}

	return false;
}

// Non-equality test
bool value::operator!=( const value &other ) const noexcept {
	return !( ( *this ) == other );
}

// Use value as JSON object and get property value under 'key'. If this value
// is not an object or 'key' is not found, null value is always returned.
value value::operator[]( string_view key ) const noexcept {
	if ( !is_object() )
		return {};

	object_view ov( *this );
	return ov[key];
}

// Use value as JSON array and get item at 'index'. If this value is not
// an array or index is out of bounds, null value is returned.
value value::operator[]( size_t index ) const noexcept {
	if ( !is_array() )
		return {};

	array_view av( *this );
	return av[index];
}

// Get location in the original file (line, column & byte offset)
location value::loc() const noexcept {
	return _loc;
}

void value::relink( const class document *prevDoc, class document &doc ) noexcept {
	if ( ( _data & mask_type ) == type_string )
	{
		if ( prevDoc )
			payload( payload<const char *>() - prevDoc->strings_data() );
		else
		{
			if ( auto *str = get_c_str(); str < doc.strings_data() || str >= doc.strings_data() + doc._strings.size() )
				payload( doc.alloc_string( str ) );
			else
				payload( payload<const char *>() - doc.strings_data() );
		}

		_data &= ~mask_type;
		_data |= type_string_off;
	}
	else if ( is_object() || is_array() )
	{
		if ( prevDoc )
			payload( payload<const value *>() - prevDoc->_values.data() );

		payload( doc._values.data() + payload<uint64_t>() );
	}
}

// Stores lower 48bits of uint64 as payload
void value::payload( uint64_t p ) noexcept {
	_data = ( _data & ~mask_payload ) | p;
}

// Stores lower 48bits of a pointer as payload
void value::payload( const void *p ) noexcept {
	payload( reinterpret_cast<uint64_t>( p ) );
}


} // namespace json5::detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace json5 {

/*

json5::document

*/
// Construct empty document
document::document()
	:	detail::value()
{
	reset();
}

// Construct a document copy
document::document( const document &copy ) 
	:	detail::value()
{
	assign_copy( copy );
}

// Construct a document from r-value
document::document( document&& rValue ) noexcept
	:	detail::value()
{
	assign_rvalue( _JSON5_FORWARD<document>( rValue ) );
}

// Copy data from another document (does a deep copy)
document& document::operator=( const document &copy ) {
	assign_copy( copy );
	return *this;
}

// Assign data from r-value (does a swap)
document& document::operator=( document &&rValue ) noexcept {
	assign_rvalue( _JSON5_FORWARD<document>( rValue ) );
	return *this;
}


// Add UTF-8 characters to the strings array
document& document::operator+( const char ch ) {
	char c = ch;
	std::basic_string<char> s;
	s += c;
	return (*this + s.data());
}

document& document::operator+( const char* ch ) {
	alloc_string( ch, -1 );
	return *this;
}


detail::string_offset document::alloc_string( const char *str, size_t length ) {
	if ( length == size_t( -1 ) )
		length = str ? strlen( str ) : 0;

	if ( !str || !length )
		return 0;

	auto result = detail::string_offset( _strings.size() );

	_strings.resize( _strings.size() + length + 1 );
	memcpy( _strings.data() + result, str, length );
	_strings[result + length] = 0;
	return result;
}

void document::reset() noexcept {
	_data = value::type_null | value::mask_is_document;
	_values.clear();
	_strings.clear();
	_strings.push_back( 0 );
}

void document::convert_string_offsets() {
	for ( auto &v : _values )
	{
		if ( ( v._data & mask_type ) == type_string_off )
		{
			v.payload( strings_data() + v.payload<uint64_t>() );
			v._data &= ~mask_type;
			v._data |= type_string;
		}
	}
}

void document::assign_copy( const document &copy ) {
	_data = copy._data;
	_strings = copy._strings;
	_values = copy._values;

	for ( auto &v : _values )
		v.relink( &copy, *this );

	relink( &copy, *this );
	convert_string_offsets();
}

void document::assign_rvalue( document &&rValue ) noexcept {
	_data = _JSON5_MOVE( rValue._data );
	_strings = _JSON5_MOVE( rValue._strings );
	_values = _JSON5_MOVE( rValue._values );

	for ( auto &v : _values )
		v.relink( &rValue, *this );

	convert_string_offsets();
}

void document::assign_root( detail::value root ) noexcept {
	_data = root._data | mask_is_document;

	for ( auto &v : _values )
		v.relink( nullptr, *this );

	relink( nullptr, *this );
	convert_string_offsets();
} 

const char* document::strings_data() const noexcept {
	return reinterpret_cast<const char *>( _strings.data() );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*

json5::object_view

*/

// Construct object view over a value. If the provided value does not reference a JSON object,
// this object_view will be created empty (and invalid)
object_view::object_view( const detail::value &v ) noexcept
	: _pair( v.is_object() ? ( v.payload<const detail::value *>() + 1 ) : nullptr )
	, _count( _pair ? ( _pair[-1].get_number<size_t>() / 2 ) : 0 )
{}

// Checks, if object view was constructed from valid value
bool object_view::is_valid() const noexcept {
	return _pair != nullptr;
}

// Source JSON value (first key value in first key-value pair)
const detail::value* object_view::source() const noexcept {
	return _pair;
}

// Location of the source value, returns invalid location for invalid view
location object_view::loc() const noexcept {
	return _pair ? _pair->loc() : location();
}


object_view::iterator::iterator( const detail::value* p ) noexcept
	:	_pair( p )
{}

bool object_view::iterator::operator!=( const iterator &other ) const noexcept {
	return _pair != other._pair;
}

bool object_view::iterator::operator==( const iterator &other ) const noexcept {
	return _pair == other._pair;
}
object_view::iterator& object_view::iterator::operator++() noexcept {
	_pair += 2;
	return *this;
}
object_view::key_value_pair object_view::iterator::operator*() const noexcept {
	return { _pair[0].get_c_str(), _pair[1] };
}

// Get an iterator to the beginning of the object (first key-value pair)
object_view::iterator object_view::begin() const noexcept {
	return { _pair };
}

// Get an iterator to the end of the object (past the last key-value pair)
object_view::iterator object_view::end() const noexcept {
	return { _pair + _count * 2 };
}

// Find property value with 'key'. Returns end iterator, when not found.
object_view::iterator object_view::find( string_view key ) const noexcept {
	if ( !key.empty() )
	{
		for ( auto iter = begin(); iter != end(); ++iter )
			if ( key == ( *iter ).first )
				return iter;
	}

	return end();
}

// Get number of key-value pairs
size_t object_view::size() const noexcept {
	return _count;
}

// True, when object is empty
bool object_view::empty() const noexcept {
	return size() == 0;
}

// Returns value associated with specified key or invalid value, when key is not found
detail::value object_view::operator[]( string_view key ) const noexcept {
	const auto iter = find( key );
	return ( iter != end() ) ? ( *iter ).second : detail::value();
}

// Returns key-value pair at specified index
object_view::key_value_pair object_view::operator[]( size_t index ) const noexcept{
	if ( index >= _count )
		return {};

	return { _pair[index * 2].get_c_str(), _pair[index * 2 + 1] };
}


bool object_view::operator==( const object_view &other ) const noexcept {
	return _pair == other._pair;
}

bool object_view::operator!=( const object_view &other ) const noexcept {
	return !( ( *this ) == other );
}


/*

json5::array_view

*/
// Construct array view over a value. If the provided value does not reference a JSON array,
// this array_view will be created empty (and invalid)
array_view::array_view( const detail::value &v ) noexcept
	: _value( v.is_array() ? ( v.payload<const detail::value *>() + 1 ) : nullptr )
	, _count( _value ? _value[-1].get_number<size_t>() : 0 )
{}

// Checks, if array view was constructed from valid value
bool array_view::is_valid() const noexcept {
	return _value != nullptr;
}

// Source JSON value (first array item)
const detail::value* array_view::source() const noexcept {
	return _value;
}

// Location of the source value, returns invalid location for invalid view
location array_view::loc() const noexcept {
	return _value ? _value->loc() : location();
}

array_view::iterator array_view::begin() const noexcept {
	return _value;
}

array_view::iterator array_view::end() const noexcept {
	return _value + _count;
}

size_t array_view::size() const noexcept {
	return _count;
}

bool array_view::empty() const noexcept {
	return _count == 0;
}

detail::value array_view::operator[]( size_t index ) const noexcept {
	return ( index < _count ) ? _value[index] : detail::value();
}

bool array_view::operator==( const array_view &other ) const noexcept {
	return _value == other._value;
}

bool array_view::operator!=( const array_view &other ) const noexcept {
	return !( ( *this ) == other );
}

} // namespace json5
