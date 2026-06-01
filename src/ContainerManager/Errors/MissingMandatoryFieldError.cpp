#include "MissingMandatoryFieldError.h"

namespace ContainerManager::Errors
{
	void MissingFieldError::AddMissingField(const std::string& path, 
		const std::string& expected)
	{
		std::string err = fmt::format("{}: Requires at least one of these fields: {}"sv,
			path, 
			expected);
		_data.emplace_back(std::move(err));
	}
}