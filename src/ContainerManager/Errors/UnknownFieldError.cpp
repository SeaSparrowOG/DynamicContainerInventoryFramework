#include "UnknownFieldError.h"

namespace ContainerManager::Errors
{
	void UnknownFieldError::AddUnknownField(const std::string& path, 
		const std::string& field)
	{
		std::string err = fmt::format("{}: Unknown member {}"sv,
			path,
			field);
		_data.emplace_back(std::move(err));
	}
}