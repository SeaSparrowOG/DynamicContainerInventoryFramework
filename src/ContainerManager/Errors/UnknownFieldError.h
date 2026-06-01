#pragma once

#include "_IError.h"

namespace ContainerManager
{
	namespace Errors
	{
		class UnknownFieldError : public IError
		{
		public:
			UnknownFieldError() { _type = ErrorType::UnknownField; }

			void AddUnknownField(const std::string& path,
				const std::string& field);
		};
	}
}