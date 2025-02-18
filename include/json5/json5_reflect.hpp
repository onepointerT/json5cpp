#pragma once

#include "json5_builder.hpp"

#if !defined( JSON5_DO_NOT_USE_STL )
	#include <array>
	#include <map>
	#include <unordered_map>
#endif

namespace json5 {

// Serialize instance of type 'T' into json5::document
template <typename T> void to_document( document &doc, const T &in, const writer_params &wp = writer_params() );

// Serialize instance of type 'T' into JSON string
template <typename T> void to_string( string &str, const T &in, const writer_params &wp = writer_params() );

// Serialize instance of type 'T' into JSON string
template <typename T> string to_string( const T &in, const writer_params &wp = writer_params() );

// Deserialize json5::document into instance of type 'T'
template <typename T> error from_document( const document &doc, T &out );

// Deserialize JSON string into instance of type 'T'
template <typename T> error from_string( string_view str, T &out );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail {

/* Forward declarations */
template <typename T> error read( const value &in, T &out );

class writer final : public builder
{
public:
	writer( document &doc, const writer_params &wp );

	const writer_params &params() const noexcept;

private:
	writer_params _params;
};

//---------------------------------------------------------------------------------------------------------------------
string_view get_name_slice( const char *names, size_t index );

/* Forward declarations */
template <typename T> value write( writer &w, const T &in );

//---------------------------------------------------------------------------------------------------------------------
value write( writer &w, bool in );
value write( writer &w, int in );
value write( writer &w, unsigned in );
value write( writer &w, float in );
value write( writer &w, double in );
value write( writer &w, const char *in );
value write( writer &w, const string &in );

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline value write_array( writer &w, const T *in, size_t numItems )
{
	w.push_array();
	for ( size_t i = 0; i < numItems; ++i )
		w( in[i] );

	return w.pop();
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T, typename A>
inline value write( writer &w, const std::vector<T, A> &in ) { return write_array( w, in.data(), in.size() ); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T, size_t N>
inline value write( writer &w, const T( &in )[N] ) { return write_array( w, in, N ); }


//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline value write_map( writer &w, const T &in )
{
	w.push_object();

	for ( const auto &[k, v] : in )
		w[k] = write( w, v );

	return w.pop();
}

#if !defined( JSON5_DO_NOT_USE_STL )
//---------------------------------------------------------------------------------------------------------------------
template <typename T, size_t N>
inline value write( writer &w, const std::array<T, N> &in )
{
	return write_array( w, in.data(), N );
}

//---------------------------------------------------------------------------------------------------------------------
template <typename K, typename T, typename P, typename A>
inline value write( writer &w, const std::map<K, T, P, A> &in )
{
	return write_map( w, in );
}

//---------------------------------------------------------------------------------------------------------------------
template <typename K, typename T, typename H, typename EQ, typename A>
inline value write( writer &w, const std::unordered_map<K, T, H, EQ, A> &in )
{
	return write_map( w, in );
}

#endif

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline value write_enum( writer &w, T in )
{
	size_t index = 0;
	const auto *names = enum_table<T>::names;
	const auto *values = enum_table<T>::values;

	while ( true )
	{
		auto name = get_name_slice( names, index );

		// Underlying value fallback
		if ( name.empty() )
			return write( w, _JSON5_UNDERLYING( T )( in ) );

		if ( in == values[index] )
			return w.new_string( name );

		++index;
	}

	return {};
}

//---------------------------------------------------------------------------------------------------------------------
template <size_t Index = 0, typename... Types>
inline void write( writer &w, const json5::detail::named_ref_list<Types...> &t )
{
	const auto &in = t.get( json5::detail::index<Index>() );
	using Type = _JSON5_DECAY( decltype( in ) );

	if ( auto name = get_name_slice( t.names(), Index ); !name.empty() )
	{
		if constexpr ( std::is_enum_v<Type> )
		{
			if constexpr ( enum_table<Type>() )
				w[name] = write_enum( w, in );
			else
				w[name] = write( w, _JSON5_UNDERLYING( Type )( in ) );
		}
		else
			w[name] = write( w, in );
	}

	if constexpr ( Index + 1 != sizeof...( Types ) )
		write < Index + 1 > ( w, t );
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline value write( writer &w, const T &in )
{
	w.push_object();
	if ( std::is_void<T>() ) {
		return value("");
	}
	write( w, in );
	return w.pop();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Forward declarations */
template <typename T> error read( const value &in, T &out );

//---------------------------------------------------------------------------------------------------------------------
error read( const value &in, bool &out );

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error read_number( const value &in, T &out )
{
	return in.try_get_number( out ) ? error() : error{ error::number_expected, in.loc() };
}

//---------------------------------------------------------------------------------------------------------------------
error read( const value &in, int &out );
error read( const value &in, unsigned &out );
error read( const value &in, float &out );
error read( const value &in, double &out );

//---------------------------------------------------------------------------------------------------------------------
error read( const value &in, const char *&out );

//---------------------------------------------------------------------------------------------------------------------
inline error read( const value &in, string &out );

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error read_array( const value &in, T *out, size_t numItems )
{
	if ( !in.is_array() )
		return { error::array_expected, in.loc() };

	auto arr = json5::array_view( in );
	if ( arr.size() != numItems )
		return { error::wrong_array_size, in.loc() };

	for ( size_t i = 0; i < numItems; ++i )
		if ( auto err = read( arr[i], out[i] ) )
			return err;

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T, size_t N>
inline error read( const value &in, T( &out )[N] ) { return read_array( in, out, N ); }

//---------------------------------------------------------------------------------------------------------------------
template <typename T, typename A>
inline error read( const value &in, std::vector<T, A> &out )
{
	if ( !in.is_array() && !in.is_null() )
		return { error::array_expected, in.loc() };

	auto arr = json5::array_view( in );

	out.clear();
	out.reserve( arr.size() );
	for ( const auto &i : arr )
		if ( auto err = read( i, out.emplace_back() ) )
			return err;

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error read_map( const value &in, T &out )
{
	if ( !in.is_object() && !in.is_null() )
		return { error::object_expected, in.loc() };

	out.clear();
	for ( auto jsKV : json5::object_view( in ) )
	{
		std::pair<typename T::key_type, typename T::mapped_type> kvp;

		kvp.first = jsKV.first;

		if ( auto err = read( jsKV.second, kvp.second ) )
			return err;

		out.emplace( std::move( kvp ) );
	}

	return { error::none };
}

#if !defined( JSON5_DO_NOT_USE_STL )
//---------------------------------------------------------------------------------------------------------------------
template <typename T, size_t N>
inline error read( const value &in, std::array<T, N> &out )
{
	return read_array( in, out.data(), N );
}

//---------------------------------------------------------------------------------------------------------------------
template <typename K, typename T, typename P, typename A>
inline error read( const value &in, std::map<K, T, P, A> &out )
{
	return read_map( in, out );
}

template <typename K, typename T, typename H, typename EQ, typename A>
inline error read( const value &in, std::unordered_map<K, T, H, EQ, A> &out )
{
	return read_map( in, out );
}
#endif

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error read_enum( const value &in, T &out )
{
	if ( !in.is_string() && !in.is_number() )
		return { error::string_expected, in.loc() };

	size_t index = 0;
	const auto *names = enum_table<T>::names;
	const auto *values = enum_table<T>::values;

	while ( true )
	{
		auto name = get_name_slice( names, index );

		if ( name.empty() )
			break;

		if ( in.is_string() && name == in.get_c_str() )
		{
			out = values[index];
			return { error::none };
		}
		else if ( in.is_number() && in.get_number( 0 ) == int( values[index] ) )
		{
			out = values[index];
			return { error::none };
		}

		++index;
	}

	return { error::invalid_enum, in.loc() };
}

//---------------------------------------------------------------------------------------------------------------------
template <size_t Index = 0, typename... Types>
inline error read( const json5::object_view &obj, json5::detail::named_ref_list<Types...> &t )
{
	auto &out = t.get( json5::detail::index<Index>() );
	using Type = _JSON5_DECAY( decltype( out ) );

	auto name = get_name_slice( t.names(), Index );

	auto iter = obj.find( name );
	if ( iter != obj.end() )
	{
		if constexpr ( std::is_enum_v<Type> )
		{
			if constexpr ( enum_table<Type>() )
			{
				if ( auto err = read_enum( ( *iter ).second, out ) )
					return err;
			}
			else
			{
				_JSON5_UNDERLYING( Type ) temp = {};
				if ( auto err = read( ( *iter ).second, temp ) )
					return err;

				out = Type( temp );
			}
		}
		else
		{
			if ( auto err = read( ( *iter ).second, out ) )
				return err;
		}
	}

	if constexpr ( Index + 1 != sizeof...( Types ) )
	{
		if ( auto err = read < Index + 1 > ( obj, t ) )
			return err;
	}

	return { error::none };
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error read( const value &in, T &out )
{
	if ( !in.is_object() )
		return { error::object_expected, in.loc() };

	auto namedTuple = class_wrapper<T>::make_named_ref_list( out );
	return read( json5::object_view( in ), namedTuple );
}

//---------------------------------------------------------------------------------------------------------------------
template <size_t Index = 0, typename Head, typename... Tail>
inline error read( const json5::array_view &arr, Head &out, Tail &... tail )
{
	if constexpr ( Index == 0 )
	{
		if ( !arr.is_valid() )
			return { error::array_expected, arr.loc() };

		if ( arr.size() != ( 1 + sizeof...( Tail ) ) )
			return { error::wrong_array_size, arr.loc() };
	}

	if ( auto err = read( arr[Index], out ) )
		return err;

	if constexpr ( sizeof...( Tail ) > 0 )
		return read < Index + 1 > ( arr, tail... );

	return { error::none };
}

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void to_document( document &doc, const T &in, const writer_params &wp )
{
	detail::writer w( doc, wp );
	detail::write( w, in );
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void to_string( string &str, const T &in, const writer_params &wp )
{
	if ( std::is_same<const char*, T>() ) {
		return;
	}
	document doc;
	to_document( doc, in );
	to_string( str, doc.get_c_str(), wp );
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline string to_string( const T &in, const writer_params &wp )
{
	string result;
	to_string( result, in, wp );
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error from_document( const document &doc, T &out )
{
	return detail::read( doc, out );
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline error from_string( string_view str, T &out )
{
	document doc;
	if ( auto err = from_string( str, doc ) )
		return err;

	return from_document( doc, out );
}

} // json5
