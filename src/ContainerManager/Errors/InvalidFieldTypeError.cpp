#include "InvalidFieldTypeError.h"

#include <magic_enum/magic_enum.hpp>

namespace ContainerManager::Errors
{
	void InvalidTypeError::AddInvalidField(const std::string& path, 
		const std::string& expected, 
		const Json::Value& field)
	{
		std::string err = fmt::format("{}: Received JSON Field of type {}, but expected {}."sv,
			path, magic_enum::enum_name(field.type()), expected);
		_data.emplace_back(std::move(err));
	}
}