#pragma once

#include "ContainerManager/ContainerManager.h"

namespace ContainerManager
{
	namespace Conditions
	{
		inline static constexpr std::string_view OBJECT_VALUES = "values"sv;
		inline static constexpr std::string_view OBJECT_INVERT = "invert"sv;
		inline static constexpr std::string_view OBJECT_TYPE = "type"sv;
		inline static constexpr std::string_view OBJECT_AND = "add"sv;
		inline static constexpr std::string_view OBJECT_OR = "or"sv;

		// Unused for now.
		inline static constexpr std::array<std::string_view, 3> knownFields = { OBJECT_TYPE, OBJECT_INVERT, OBJECT_VALUES };

		struct AVRequirement
		{
			RE::ActorValue av{ RE::ActorValue::kNone };
			float max{ 0.0f };
			float min{ 0.0f };
			bool respectMax{ false };

			bool MeetsCondition(const RE::ActorValueOwner* a_actor) const;
			void PrintCondition(const std::string& a_pref = "  ") const;
		};

		class AVCondition : public Condition
		{
		public:
			AVCondition(std::vector<AVRequirement> a_requirements, bool a_and = true, bool invert = false);
			virtual void PrintCondition(const std::string& a_pref = "    ") const override;
			virtual bool IsValid(const ConditionCheckParams& a_params) const override;

		private:
			bool andCondition{ true };
			bool inverted{ false };
			std::vector<AVRequirement> m_requirements{};
		};

		struct FailedAV
		{
			bool incorrectSize{ false };
			bool invalidMax{ false };
			bool invalidMin{ false };
			bool invalidAV{ false };

			std::string_view text{ ""sv };
		};

		struct AVConditionResult
		{
			bool errored{ false };

			bool empty{ false };
			bool typeError{ false };
			bool invertionError{ false };
			bool invalidObjectType{ false };
			bool missingValuesField{ false };
			bool nonHomogenousArray{ false };
			std::vector<FailedAV> errors{};
			std::vector<std::string> unknownFields{};

			std::optional<AVCondition> result{ std::nullopt };
		};

		[[nodiscard]] AVConditionResult CreateAVCondition(const Json::Value& a_template, bool invert);
	}
}