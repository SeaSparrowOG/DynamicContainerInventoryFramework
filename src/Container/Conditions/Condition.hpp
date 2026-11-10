#pragma once

#include "Cache/Cache.hpp"
#include "Settings/INI/INISettings.hpp"

#include "Settings/JSON/JSONSettings.hpp" // this pulls in FormUtils and json/json. This is intentional.

namespace Container::Conditions {
    
    // Enum representing condition types AND how easy they are to check
    enum class ConditionType {
        None,
        BaseForm,
        Reference,

        Total
    };

    // Little optimization cheat. We populate all relevant fields that only need to be polled once per reference
    // when the reference loads. This should HOPEFULLY reduce the complexity of loops, allowing for better
    // performance? If it doesn;t, I can always fall back to mutating the param as more stuff gets requested.
    class ConditionParam {
    public:
        bool noReset =  false;
        bool vendorChest = false;
        RE::BGSLocation*   location = nullptr;
        RE::TESObjectREFR* ref = nullptr;
        RE::TESWorldSpace* world = nullptr;
        RE::TESObjectCELL* cell = nullptr;
        RE::TESBoundObject* base = nullptr;

        RE::LOCK_LEVEL lock;

        std::unordered_set<RE::BGSKeyword*> locKeywords{};

        std::unordered_set<RE::BGSLocation*> locParents{};
        std::unordered_set<RE::TESWorldSpace*> worldParents{};

        ConditionParam(RE::TESObjectREFR* from) noexcept;
    };

    /// @brief Abstract class that implements IsValid. 
    class Condition {
    public:
        [[nodiscard]]
        virtual bool IsValid(const ConditionParam& param) const noexcept { (void)param; return false; };

        [[nodiscard]]
        virtual ConditionType GetType() const noexcept { return ConditionType::None; };

        virtual ~Condition();
    private:
        bool inverted = false;
    };

    using Condition_ptr = std::unique_ptr<Condition>;
    using Conditions = std::vector<Condition_ptr>;

    // This bears some explaining. A change can have any number of conditions, but
    // in order for it to not apply, it only needs 1 to fail. So, for example:
    // >  Must be of base form <0x800> in <cooldungeonclutter.esp>
    // >  Must be in a location with keywords <A, B, C>
    // We can resolve filter 1 immediately. Filter 2 is significantly harder, since it is
    // 3 lookups, AND if they fail, we also move forward to that location's parent and repeat.
    // So, when conditions are created, they are sorted from fastest to slowest, and slowest
    // ones get moved later. 
    class ConditionContainer {
    public:
        [[nodiscard]] 
        bool IsValid(const ConditionParam& param) const noexcept { 
            return _conditions.empty() || std::ranges::all_of(_conditions,
                [param](const auto& condition) {
                    return condition->IsValid(param);
                });
        };

        void Finalize() noexcept;
        void AddCondition(Condition_ptr& condition) noexcept;

    private:
        bool       _finalized = false;
        Conditions _conditions{};
    };

    class ConditionFail {
    public:
        void Log(const std::string& prefix) const noexcept;
    };

    [[nodiscard]]
    std::expected<ConditionContainer, ConditionFail> create_condition(const Json::Value& val) noexcept;
}