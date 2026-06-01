#include "EmptyConditionError.h"

namespace ContainerManager::Errors
{
	void EmptyConditionError::AddEmptyCondition(const std::string& path)
	{
		std::string err = fmt::format("{}: Condition resolved empty."sv, path);
		_data.emplace_back(std::move(err));
	}
}