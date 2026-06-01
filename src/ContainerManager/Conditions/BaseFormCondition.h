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

		using condition_ptr = std::unique_ptr<ContainerManager::Condition>;
		using failure_ptr = std::unique_ptr<ContainerManager::Errors::IError>;
		using error_ptrs = std::vector<failure_ptr>;
		std::expected<condition_ptr, error_ptrs> TryCreateBaseFormCondition(const Json::Value& from, std::string& path, bool inverted);
	}
}