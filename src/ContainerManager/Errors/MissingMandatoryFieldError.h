#pragma once

#include "_IError.h"

namespace ContainerManager
{
	namespace Errors
	{
		class MissingFieldError : public IError
		{
		public:
			MissingFieldError() { _type = ErrorType::MissingField; }

			void AddMissingField(const std::string& path,
				const std::string& expected);
		};
	}
}