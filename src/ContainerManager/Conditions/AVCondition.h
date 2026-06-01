#pragma once

#include "ContainerManager/ContainerManager.h"

namespace ContainerManager
{
	namespace Conditions
	{
		struct IndividualCondition
		{
			bool bound = false;
			float min = 0.0f;
			float max = 0.0f;
			RE::ActorValue val = RE::ActorValue::kNone;

			bool IsValid(RE::ActorValueOwner* owner) const {
				const float skill = owner->GetActorValue(val);
				return bound ? 
					skill > min && skill < max :
					skill > min;
			}

			std::string ToString() const {
				auto str = fmt::format("{}|{}", RE::ActorValueToString(val), min);
				if (bound) {
					str += "|" + std::to_string(max);
				}
				return str;
			}
		};

		struct Required
		{
			bool isOr = false;
			std::vector<IndividualCondition> _data;

			bool IsValid(RE::ActorValueOwner* owner) const {
				auto f = [&](const IndividualCondition& condition) {
					return condition.IsValid(owner); 
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

		class AVCondition : public Condition
		{
		public:
			bool IsValid(const ConditionCheckParams& a_params) const override;
			void PrintCondition(const std::string& a_pref) const override;

			void AppendCondition(Required& individualData);
			void SetInverted(bool inverted) { _inverted = inverted; }
		private:
			bool                  _inverted = false;
			std::vector<Required> _data;
		};

		class AVConditionFailure : public ParseFailure
		{
		public:
			void Report(const std::string& a_prefix) const override;
			virtual void Preamble(const std::string& a_prefix) const override;

			AVConditionFailure() { type = FailureType::AVCondition; }

			void FlagEmptyAVVector(const std::string& path);
			void AddWrongFieldType(const std::string& path, const std::string& receivedType, const std::string& expected);
			void AddMalformedAVString(const std::string& rawAV, const std::string& path);
			void AddWrongComplexMemberType(const std::string& path, const std::string& expected, const std::string& received);
			void AddUnknownComplexMemberType(const std::string& path, const std::string& receivedType);

		private:
			std::vector<std::string> _emptyAVVectors;
			std::vector<std::string> _wrongFieldTypes;
			std::vector<std::string> _malformedAVStrings;
			std::vector<std::string> _wrongComplexMemberTypes;
			std::vector<std::string> _unknownComplexMemberTypes;
		};

		std::expected<AVCondition, AVConditionFailure> CreateAVCondition(const Json::Value& from, std::string& path, bool inverted);
	}
}