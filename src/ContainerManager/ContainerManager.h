#pragma once

namespace ContainerManager
{
    struct ConditionCheckParams
    {
        ConditionCheckParams(RE::TESObjectREFR* a_container);

		RE::TESObjectREFR* reference{ nullptr };
    };

    struct ConfigErrors
    {
        void PrintErrors() const;

        bool hasErrors{ false };
        std::string configName{};
        std::vector<std::string> emptyConditions{};
        std::vector<std::string> unexpectedErrors{};
        std::vector<std::string> invalidFieldNames{};
        std::vector<std::string> invalidFieldValues{};
        std::vector<std::string> invalidArrayObjects{};
        std::vector<std::string> unresolvedConditions{};
        std::vector<std::string> missingRequiredFields{};
    };

    class Condition
    {
    public:
        virtual bool IsValid(const ConditionCheckParams& a_params) const = 0;
    };

    class Change
    {
    public:
        virtual bool CanApply(const ConditionCheckParams& a_params) const = 0;
        virtual void Apply(ConditionCheckParams& a_params) const = 0;
    };

    class Rule
    {
    public:
        enum class RuleType
        {
            kAdd,
            kRemove,
            kReplace,
            kRemoveByKeywords,
			kReplaceByKeywords
        };

        void Apply(ConditionCheckParams& a_params) const;
        bool CheckConditions(const ConditionCheckParams& a_params) const;

        bool onlyVendorChests{ false };
        bool allowVendorChests{ false };
        bool allowNoResetChests{ false };
		RuleType type{ RuleType::kAdd };
        std::vector<Condition> conditions{};
        std::vector<Change>    changes{};
    };

    class InventorySwapper : public REX::Singleton<InventorySwapper>
    {
    public:
        bool Initialize();

        void ManipulateInventory(RE::TESObjectREFR* a_container);
    private:
		std::vector<Rule> rules{};
    };
}