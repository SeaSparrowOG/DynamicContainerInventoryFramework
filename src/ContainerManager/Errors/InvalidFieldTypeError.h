#pragma once

#include "_IError.h"

namespace ContainerManager
{
	namespace Errors
	{
		class InvalidTypeError : public IError
		{
		public:
			InvalidTypeError() { _type = ErrorType::FieldType; }

			void AddInvalidField(const std::string& path, 
				const std::string& expected, 
				const Json::Value& field);
		};
	}
}