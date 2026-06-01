#include "UnresolvedFormError.h"

namespace ContainerManager::Errors
{
	void UnresolvedFormError::AddUnresolvedForm(const std::string& path,
		const std::string& raw)
	{
		std::string err = fmt::format("{}: Failed to resolve this form: {}"sv,
			path, raw);
		_data.emplace_back(std::move(err));
	}
}