#pragma once

namespace ContainerManager
{
    struct ConditionCheckParams
    {
        ConditionCheckParams(RE::TESObjectREFR* a_container);

        RE::ActorValueOwner* playerOwner{ nullptr };
		RE::TESObjectREFR*   reference{ nullptr };
    };

    class Condition
    {
    public:
        virtual bool IsValid(const ConditionCheckParams& a_params) const = 0;
        virtual void PrintCondition(const std::string& a_pref = "    ") const = 0;
    };

    class Change
    {
    public:
        enum class ChangeType
        {
            Add = 1,
            Remove = 2,
            RemoveByKeywords = 3,
            Replace = 4,
            ReplaceByKeywords = 5,

            Invalid = 6
        };

        virtual void Apply(RE::TESObjectREFR* a_target) const = 0;

        bool CanApply(const std::unordered_set<std::size_t> a_applicableConditions) const;
        void DefineConditions(std::vector<std::size_t> a_ids);
        ChangeType GetType() const { return ChangeType::Invalid; }

    private:
        std::vector<std::size_t> ids{};
    };

    class InventorySwapper : public REX::Singleton<InventorySwapper>
    {
    public:
        void RegisterChange(std::unique_ptr<Change> a_change);
        [[nodiscard]] std::size_t RegisterCondition(std::unique_ptr<Condition> a_condition);

        void ManipulateInventory(RE::TESObjectREFR* a_container);
    private:
		std::vector<std::unique_ptr<Condition>> conditions{};
        std::vector<std::unique_ptr<Change>>    changes{};    // Sorted vector.
    };
}