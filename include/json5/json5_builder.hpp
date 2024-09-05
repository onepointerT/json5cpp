#pragma once

#include "json5.hpp"

namespace json5 {

class builder
{
public:
	builder( document &doc );

	const document &doc() const noexcept;

	detail::value new_string( string_view str );
	detail::value new_string( detail::string_offset stringOffset );

	void push_object();
	void push_array();
	detail::value pop();

	template <typename... Args>
	builder &operator()( Args... values )
	{
		bool results[] = { add_item( values )... };
		return *this;
	}

	detail::value &operator[]( string_view key );
	detail::value &operator[]( detail::string_offset keyOffset );

protected:
	void reset() noexcept;

	detail::string_offset string_buffer_offset() const noexcept;
	detail::string_offset string_buffer_add( string_view str );
	void string_buffer_add( char ch );
	void string_buffer_add_utf8( uint32_t ch );

	bool add_item( detail::value v );

	builder& operator+=( detail::value v );

	document &_doc;
	std::vector<detail::value> _stack;
	std::vector<detail::value> _values;
	std::vector<size_t> _counts;
};

} // namespace json5
