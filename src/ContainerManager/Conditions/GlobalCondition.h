#pragma once

#include "ContainerManager/ContainerManager.h"

namespace ContainerManager
{
	namespace Conditions
	{
		struct IndividualGlobal
		{
			bool bound = false;
			float min = 0.0f;
			float max = 0.0f;

			RE::FormID globID = 0;

			bool IsValid() const {
				auto* res = RE::TESForm::LookupByID<RE::TESGlobal>(globID);
				if (!res) {
					return false; 
				}
				const float val = res->value;
				return bound ? val >= min && val <= max : val >= min;
			}

			std::string ToString() const {
				auto* res = RE::TESForm::LookupByID<RE::TESGlobal>(globID);
				if (!res) {
					return "Failed to resolve!";
				}

				auto str = fmt::format("{}|{}", res->GetFormEditorID(), min);
				if (bound) {
					str += "|" + std::to_string(max);
				}
				return str;
			}
		};

		struct RequiredGlobals
		{
			bool isOr = false;
			std::vector<IndividualGlobal> _data;

			bool IsValid() const {
				auto f = [&](const IndividualGlobal& condition) {
					return condition.IsValid();
					};
				return isOr ?
					std::ranges::any_of(_data, f) :
					std::ranges::all_of(_data, f);
			}

			std::string GetFormattedDescription() const {
				std::string str = "";
				for (auto it = _data.begin(); it < _data.end(); ++it) {
					str += "<" + it->ToString() + ">";
					if (it < _data.end() - 1) {
						if (isOr) {
							str += " [OR] ";
						}
						else {
							str += " [AND] ";
						}
					}
				}
				return str;
			}
		};

		class GlobCondition : public Condition
		{
		public:
			bool IsValid(const ConditionCheckParams& a_params) const override;
			void PrintCondition(const std::string& a_pref) const override;

			void AppendCondition(RequiredGlobals& individualData);
			bool Empty() const {
				return _data.empty();
			}
		private:
			bool                         _inverted = false;
			std::vector<RequiredGlobals> _data;
		};

		using condition_ptr = std::unique_ptr<ContainerManager::Condition>;
		using failure_ptr = std::unique_ptr<ContainerManager::Errors::IError>;
		using error_ptrs = std::vector<failure_ptr>;
		std::expected<condition_ptr, error_ptrs> TryCreateGlobalCondition(const Json::Value& from, std::string& path, bool inverted);
	}
}