#pragma once

#include "json5_base.hpp"

#if !defined( JSON5_DO_NOT_USE_STL )
#include <string>
#include <vector>
#define _JSON5_MOVE                std::move
#define _JSON5_FORWARD             std::forward
#define _JSON5_DECAY( _Type )      std::decay_t<_Type>
#define _JSON5_UNDERLYING( _Type ) std::underlying_type_t<_Type>

namespace json5 {
using string = std::string;
using string_view = std::string_view;
} // namespace json5
#else

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace json5::detail {

/*

json5::detail::value

*/
class value
{
public:
	// Construct null value
	value() noexcept;

	// Construct null value
	value( std::nullptr_t ) noexcept;

	// Construct boolean value
	value( bool val ) noexcept;

	// Construct number value from int (will be converted to double)
	value( int val ) noexcept;

	// Construct number value from double
	value( double val ) noexcept;

	// Construct string value from null-terminated string
	value( const char *val ) noexcept;

	// Return value type
	value_type type() const noexcept;

	// Checks, if value is null
	bool is_null() const noexcept;

	// Checks, if value stores boolean. Use 'get_bool' for reading.
	bool is_boolean() const noexcept;

	// Checks, if value stores number. Use 'get_number' or 'try_get_number' for reading.
	bool is_number() const noexcept;

	// Checks, if value stores string. Use 'get_c_str' for reading.
	bool is_string() const noexcept;

	// Checks, if value stores JSON object. Use 'object_view' wrapper
	// to iterate over key-value pairs (properties).
	bool is_object() const noexcept;

	// Checks, if value stores JSON array. Use 'array_view' wrapper
	// to iterate over array elements.
	bool is_array() const noexcept;
	// Check, if this is a root document value and can be safely casted to json5::document
	bool is_document() const noexcept;

	// Get stored bool. Returns 'defaultValue', if this value is not a boolean.
	bool get_bool( bool defaultValue = false ) const noexcept;

	// Get stored string. Returns 'defaultValue', if this value is not a string.
	const char *get_c_str( const char *defaultValue = "" ) const noexcept;

	// Get stored number as type 'T'. Returns 'defaultValue', if this value is not a number.
	template <typename T>
	T get_number( T defaultValue = 0 ) const noexcept
	{
		return is_number() ? T( _double ) : defaultValue;
	}

	// Try to get stored number as type 'T'. Returns false, if this value is not a number.
	template <typename T>
	bool try_get_number( T &out ) const noexcept
	{
		if ( !is_number() )
			return false;

		out = T( _double );
		return true;
	}

	// Equality test against another value. Note that this DOES NOT DO a deep equality test
	// for JSON objects nor a item-by-item equality test for arrays - only references are
	// compared for objects or arrays.
	bool operator==( const value &other ) const noexcept;

	// Non-equality test
	bool operator!=( const value &other ) const noexcept;

	// Use value as JSON object and get property value under 'key'. If this value
	// is not an object or 'key' is not found, null value is always returned.
	value operator[]( string_view key ) const noexcept;

	// Use value as JSON array and get item at 'index'. If this value is not
	// an array or index is out of bounds, null value is returned.
	value operator[]( size_t index ) const noexcept;

	// Get value payload (lower 48bits of _data) converted to type 'T'
	template <typename T>
	T payload() const noexcept
	{
		return ( T )( _data & mask_payload );
	}

	// Get location in the original file (line, column & byte offset)
	location loc() const noexcept;

	template <typename T>
	[[deprecated( "Use get_number instead" )]] T get( T defaultValue = 0 ) const noexcept
	{
		return get_number<T>( defaultValue );
	}

	template <typename T>
	[[deprecated( "Use try_get_number" )]] bool try_get( T &out ) const noexcept
	{
		return try_get_number( out );
	}

protected:
	value( value_type t, uint64_t data );
	value( value_type t, const void *data );

	void relink( const class document *prevDoc, class document &doc ) noexcept;

	// NaN-boxed data
	union
	{
		double _double;
		uint64_t _data;
	};

	// Location in source file
	location _loc = {};

	// clang-format off
	static constexpr uint64_t mask_nanbits     = 0xFFF0000000000000ull;
	static constexpr uint64_t mask_type        = 0xFFF7000000000000ull;
	static constexpr uint64_t mask_is_document = 0x0008000000000000ull;
	static constexpr uint64_t mask_payload     = 0x0000FFFFFFFFFFFFull;
	static constexpr uint64_t type_false       = 0xFFF1000000000000ull;
	static constexpr uint64_t type_true        = 0xFFF2000000000000ull;
	static constexpr uint64_t type_string      = 0xFFF3000000000000ull;
	static constexpr uint64_t type_string_off  = 0xFFF4000000000000ull;
	static constexpr uint64_t type_array       = 0xFFF5000000000000ull;
	static constexpr uint64_t type_object      = 0xFFF6000000000000ull;
	static constexpr uint64_t type_null        = 0xFFF7000000000000ull;
	// clang-format on

	// Stores lower 48bits of uint64 as payload
	void payload( uint64_t p ) noexcept;

	// Stores lower 48bits of a pointer as payload
	void payload( const void *p ) noexcept;

	friend document;
	friend builder;
	friend parser;
};

} // namespace json5::detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace json5 {

/*

json5::document

*/
class document final
	:	public detail::value
{
public:
	// Construct empty document
	document();

	// Construct a document copy
	document( const document &copy );

	// Construct a document from r-value
	document( document &&rValue ) noexcept;

	// Copy data from another document (does a deep copy)
	document &operator=( const document &copy );

	// Assign data from r-value (does a swap)
	document &operator=( document &&rValue ) noexcept;

	// Add UTF-8 characters to the strings array
	document &operator+( const char* ch );
	document &operator+( char ch );

private:
	detail::string_offset alloc_string( const char *str, size_t length = size_t( -1 ) );

	void reset() noexcept;

	void convert_string_offsets();

	void assign_copy( const document &copy );
	void assign_rvalue( document &&rValue ) noexcept;
	void assign_root( detail::value root ) noexcept;

	const char *strings_data() const noexcept;

	std::vector<uint8_t> _strings;
	std::vector<detail::value> _values;

	friend detail::value;
	friend builder;
	template< typename T >
	friend struct class_wrapper;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*

json5::object_view

*/
class object_view final
{
public:
	// Construct an empty object view
	object_view() noexcept = default;

	// Construct object view over a value. If the provided value does not reference a JSON object,
	// this object_view will be created empty (and invalid)
	object_view( const detail::value &v ) noexcept;

	// Checks, if object view was constructed from valid value
	bool is_valid() const noexcept;

	// Source JSON value (first key value in first key-value pair)
	const detail::value *source() const noexcept;

	// Location of the source value, returns invalid location for invalid view
	location loc() const noexcept;

	struct key_value_pair
	{
		string_view first = string_view();
		detail::value second = detail::value();
	};

	class iterator final
	{
	public:
		iterator( const detail::value *p = nullptr ) noexcept;
		bool operator!=( const iterator &other ) const noexcept;
		bool operator==( const iterator &other ) const noexcept;
		iterator &operator++() noexcept;
		key_value_pair operator*() const noexcept;

	private:
		const detail::value *_pair = nullptr;
	};

	// Get an iterator to the beginning of the object (first key-value pair)
	iterator begin() const noexcept;

	// Get an iterator to the end of the object (past the last key-value pair)
	iterator end() const noexcept;

	// Find property value with 'key'. Returns end iterator, when not found.
	iterator find( string_view key ) const noexcept;

	// Get number of key-value pairs
	size_t size() const noexcept;

	// True, when object is empty
	bool empty() const noexcept;

	// Returns value associated with specified key or invalid value, when key is not found
	detail::value operator[]( string_view key ) const noexcept;

	// Returns key-value pair at specified index
	key_value_pair operator[]( size_t index ) const noexcept;

	bool operator==( const object_view &other ) const noexcept;
	bool operator!=( const object_view &other ) const noexcept;

private:
	const detail::value *_pair = nullptr;
	size_t _count = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*

json5::array_view

*/
class array_view final
{
public:
	// Construct an empty array view
	array_view() noexcept = default;

	// Construct array view over a value. If the provided value does not reference a JSON array,
	// this array_view will be created empty (and invalid)
	array_view( const detail::value &v ) noexcept;

	// Checks, if array view was constructed from valid value
	bool is_valid() const noexcept;

	// Source JSON value (first array item)
	const detail::value *source() const noexcept;

	// Location of the source value, returns invalid location for invalid view
	location loc() const noexcept;

	using iterator = const detail::value *;

	iterator begin() const noexcept;
	iterator end() const noexcept;
	size_t size() const noexcept;
	bool empty() const noexcept;
	detail::value operator[]( size_t index ) const noexcept;

	bool operator==( const array_view &other ) const noexcept;
	bool operator!=( const array_view &other ) const noexcept;

private:
	const detail::value *_value = nullptr;
	size_t _count = 0;
};


} // namespace json5
