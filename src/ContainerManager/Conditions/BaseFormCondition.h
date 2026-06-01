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

		class BaseFormConditionFailure : public ParseFailure
		{
		public:
			void Report(const std::string& a_prefix) const override;
			virtual void Preamble(const std::string& a_prefix) const override;

			void AddWrongFieldType(const std::string& path, const std::string& expected, const std::string& received);
			void AddWrongFormType(const std::string& path, const std::string& rawForm);
			void AddMissingPO3Tweaks(const std::string& path, const std::string& rawForm);
			void AddBadFormatString(const std::string& path, const std::string& rawForm);
			void MarkConditionsEmpty(const std::string& path);

			BaseFormConditionFailure() { type = FailureType::BaseFormCondition; }

			bool Errored() const {
				return !_emptyCondition.empty() ||
					!_wrongFieldType.empty() ||
					!_wrongFormTypes.empty() ||
					!_badStringFormat.empty() ||
					!_formsRelyingOnTweaks.empty();
			}
		private:
			std::string              _emptyCondition = "";
			std::vector<std::string> _wrongFieldType;
			std::vector<std::string> _wrongFormTypes;
			std::vector<std::string> _badStringFormat;
			std::vector<std::string> _formsRelyingOnTweaks;
		};

		std::expected<BaseFormCondition, BaseFormConditionFailure> CreateBaseFormCondition(const Json::Value& from, std::string& path, bool inverted);
	}
}