#pragma once

#include "ContainerManager/ContainerManager.h"

namespace ContainerManager
{
	namespace Changes
	{
		class AddChange : public Change
		{
		public:
			virtual void Apply(RE::TESObjectREFR* a_target, ContainerDeltas& a_deltas) const override;
			virtual void Report(const std::string& a_prefix = "    ") const override;

			ChangeType GetType() const override { return ChangeType::Add; }

			AddChange() = default;
			void AddObject(RE::TESBoundObject* a_obj);
			void SetRandomAdd();
			void SetCount(std::uint16_t a_count);
			void SetName(const std::string& a_name);

		private:
			bool randomAdd{ false };
			std::uint16_t count{ 0u };
			std::string m_name{ "" };
			std::vector<RE::TESBoundObject*> m_additions{};
		};

		class AddChangeFailure : public ParseFailure
		{
		public:
			virtual void Report(const std::string& a_prefix) const;

			void FlagMixedArray();
			void FlagRandomAddField(const std::string& a_reason);
			void FlagCountField(const std::string& a_type);
			void FlagAddField(const std::string& a_type);
			void FlagBadForm(const std::string& a_rawString, const std::string& a_reason);
			void FlagEmpty();

			bool Errored() const;

			AddChangeFailure(const std::string& a_root) { path = a_root; };
		private:
			bool empty{ false };
			bool mixedArrayTypes{ false };

			std::string addFieldType{ "" };
			std::string countFieldType{ "" };
			std::string randomAddFieldReason{ "" };
			std::string path{ "" };

			std::vector<std::string> badForms{};
		};

		inline static constexpr std::string_view ADD_FIELD = "add"sv;
		inline static constexpr std::string_view COUNT_FIELD = "count"sv;
		inline static constexpr std::string_view RANDOM_ADD_FIELD = "randomadd"sv;

		[[nodiscard]] std::expected<AddChange, AddChangeFailure> CreateAddRule(const Json::Value& a_add, bool a_hasCount, const Json::Value& a_count);
	}
}