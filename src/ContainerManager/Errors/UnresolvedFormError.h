#pragma once

#include "_IError.h"

namespace ContainerManager
{
	namespace Errors
	{
		class UnresolvedFormError : public IError
		{
		public:
			UnresolvedFormError() { 
				_type = ErrorType::UnresolvedForm; 
				_fatal = false; // Unresolved forms are fine, empty changes/conditions aren't
			}

			void AddUnresolvedForm(const std::string& path,
				const std::string& raw);
		};
	}
}