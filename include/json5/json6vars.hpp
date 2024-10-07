#pragma once


#include "json5.hpp"


namespace json6 {


class JsonDocument final {
protected:
	const json5::document* document;

public:
	JsonDocument();
	JsonDocument( const json5::document* doc );

	const json5::document* transform() const;
};



} // namespace json6


