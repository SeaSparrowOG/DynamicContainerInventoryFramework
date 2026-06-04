#pragma once

#include "Error.h"
#include "Helpers/FormHelpers.hpp"

namespace ContainerManager
{
    struct ConditionCheckParams
    {
        ConditionCheckParams(RE::TESObjectREFR* a_container);

        RE::ActorValueOwner* playerOwner = nullptr;
		RE::TESObjectREFR*   reference   = nullptr;
        RE::FormID           baseformID  = 0;
    };

    struct ContainerDeltas
    {
        std::unordered_map<RE::TESBoundObject*, std::uint16_t> counts{};
    };
    
    class Condition
    {
    public:
        virtual bool IsValid(const ConditionCheckParams& a_params) const = 0;
        virtual void PrintCondition(const std::string& a_pref) const = 0;
        virtual ~Condition() = default;
        void SetInverted(bool inverted) { _inverted = inverted; }

    protected:
        bool _inverted = false;
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
        void DefineConditions(const std::vector<std::size_t>& a_ids);
        virtual ChangeType GetType() const { return ChangeType::Invalid; }
        virtual void Report(const std::string& a_prefix) const = 0;

        void SetOnlyVendors(bool allow) { _onlyVendors = allow; };
        void SetAllowVendors(bool allow) { _allowVendors = allow; };
        void SetAllowNoReset(bool allow) { _allowNoReset = allow; };
        void SetRandomAdd(bool allow) { _emulateRandomAdd = allow; };

    protected:
        bool                     _onlyVendors = false;
        bool                     _allowVendors = false;
        bool                     _allowNoReset = false;
        bool                     _emulateRandomAdd = false;
        std::vector<std::size_t> ids{};

        struct PendingList
        {
            uint16_t        count = 0u;
            RE::TESLevItem* ll = nullptr;
        };

        void ApplyLeveledList(RE::TESLevItem* list, uint16_t playerLevel, int16_t count, ContainerDeltas& deltas) {
            if (!list) {
                return;
            }

            std::stack<PendingList> stack;
            PendingList pending;
            pending.ll = list;
            pending.count = count;
            stack.push(std::move(pending));

            auto& counts = deltas.counts;
            RE::BSScrapArray<RE::CALCED_OBJECT> llResult;

            while (!stack.empty()) {
                llResult.clear();
                auto& top = stack.top();
                top.ll->CalculateCurrentFormList(playerLevel, top.count, llResult, 0, true);
                stack.pop();

                for (auto& calcedObj : llResult) {
                    auto* form = calcedObj.form;
                    if (!form) {
                        continue;
                    }

                    // limitation:
                    // several leveled lists do this: [LL, LL, LL]
                    // this is fine and expected. However, if the sub list references a list
                    // above it (LL -> [ObjA, ObjB, LL]) it can actually create an infinite loop.
                    // Maybe fix it in Leveled List Crash fix? Otherwise upward unordered_set
                    if (form->FORMTYPE == RE::FormType::LeveledItem) {
                        auto* ll = form->As<RE::TESLevItem>();
                        if (!ll) {
                            continue;
                        }

                        PendingList subPending;
                        subPending.ll = ll;
                        subPending.count = calcedObj.count;
                        stack.push(std::move(subPending));
                    }
                    else {
                        auto* bound = skyrim_cast<RE::TESBoundObject*>(form);
                        counts[bound] += calcedObj.count;
                    }
                }
            }
        }
    };

    class InventorySwapper : public REX::Singleton<InventorySwapper>
    {
    public:
        void Report() const;
        void RegisterChange(std::unique_ptr<Change> a_change);
        void RegisterConfigError(ErrorHolder& err);
        [[nodiscard]] std::size_t RegisterCondition(std::unique_ptr<Condition> a_condition);

        void ManipulateInventory(RE::TESObjectREFR* a_container);
    private:
        std::vector<ErrorHolder> _errors{};

        std::vector<std::unique_ptr<Condition>>    conditions{};
        std::vector<std::unique_ptr<Change>>       changes{}; // Sorted vector.
    };

    void PrintSwaps();
}