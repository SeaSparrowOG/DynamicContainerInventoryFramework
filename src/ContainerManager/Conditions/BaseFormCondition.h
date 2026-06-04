#pragma once

#include "ContainerManager/ContainerManager.h"

namespace ContainerManager
{
	namespace Conditions
	{
		class BaseFormCondition : public Condition
		{
		public:
			bool Empty() const { return _data.empty(); }
			bool IsValid(const ConditionCheckParams& a_params) const override;
			void PrintCondition(const std::string& a_pref) const override;

			void AppendBaseForm(RE::TESBoundObject* base);
		private:
			bool                           _inverted = false;
			std::unordered_set<RE::FormID> _data;
		};

		using condition = std::unique_ptr<ContainerManager::Condition>;
		using error = ErrorHolder;
		std::expected<condition, error> TryCreateBaseFormCondition(const Json::Value& from, std::string& path, bool inverted);
	}
}