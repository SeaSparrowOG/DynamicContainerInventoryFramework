#pragma once

namespace Settings
{
	namespace JSON
	{
		enum class ParseResult
		{
			Success,
			RecoverableFailure,
			UnRecoverableFailure
		};

		inline static ParseResult ParseConfig(const Json::Value& a_from);
	}
}