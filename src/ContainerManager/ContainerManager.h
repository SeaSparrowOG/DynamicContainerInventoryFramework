#pragma once

namespace ContainerManager
{
    struct ConditionCheckParams
    {
        ConditionCheckParams(RE::TESObjectREFR* a_container);

        RE::ActorValueOwner* playerOwner{ nullptr };
		RE::TESObjectREFR*   reference{ nullptr };
    };

    struct ContainerDeltas
    {
        std::unordered_map<RE::TESBoundObject*, std::uint16_t> counts{};
    };

    class Condition
    {
    public:
        virtual bool IsValid(const ConditionCheckParams& a_params) const = 0;
        virtual void PrintCondition(const std::string& a_pref = "    ") const = 0;
        virtual ~Condition() = default;
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

        virtual void Apply(RE::TESObjectREFR* a_target, ContainerDeltas& a_deltas) const = 0;

        virtual bool CanApply(const std::unordered_set<std::size_t> a_validConditions, [[maybe_unused]] const ContainerDeltas& a_deltas) const;
        void DefineConditions(std::vector<std::size_t> a_ids);
        virtual ChangeType GetType() const { return ChangeType::Invalid; }
        virtual void Report(const std::string& a_prefix = "    ") const = 0;

    protected:
        std::vector<std::size_t> ids{};
    };

    class Failure
    {
    public:
        virtual void Report(const std::string& a_prefix = "    ") const = 0;
    };

    class InventorySwapper : public REX::Singleton<InventorySwapper>
    {
    public:
        bool Report() const;
        void RegisterChange(std::unique_ptr<Change> a_change);
        void RegisterFailure(const std::string& a_config, std::unique_ptr<Failure> a_failure);
        [[nodiscard]] std::size_t RegisterCondition(std::unique_ptr<Condition> a_condition);

        void ManipulateInventory(RE::TESObjectREFR* a_container);
    private:
        std::map<std::string, std::vector<std::unique_ptr<Failure>>> failures{};
        std::vector<std::unique_ptr<Condition>>                      conditions{};
        std::vector<std::unique_ptr<Change>>                         changes{}; // Sorted vector.
    };

    bool PrintSwaps();
}