#pragma once

#include "_IError.h"

namespace ContainerManager
{
	namespace Errors
	{
		class EmptyConditionError : public IError
		{
		public:
			EmptyConditionError() { _type = ErrorType::EmptyCondition; }
			void AddEmptyCondition(const std::string& path);
		};
	}
}