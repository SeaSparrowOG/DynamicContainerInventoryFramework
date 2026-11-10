#pragma once

#include "Container/Conditions/Condition.hpp"

namespace Container::Changes {
    enum class Type
    {
        None,
        Add,
        Remove,
        RemoveByKeyword,
        Replace,
        ReplaceByKeyword,

        Total
    };

    class ContainerDelta {
    public:
        ContainerDelta(RE::TESObjectREFR* from);
    };

    class Change {
    public:
        void RegisterConditionIDs(std::optional<size_t> conditionID) noexcept { _conditionID = conditionID; };

        virtual Type GetType() const noexcept = 0;
    private:
        std::optional<size_t> _conditionID{};
    };

    class ChangeFail {
    public:
        void Log(const std::string& prefix) const noexcept;
    };

    using Changes = std::vector<std::unique_ptr<Change>>;

    [[nodiscard]]
    std::expected<Changes, ChangeFail> create_change(const Json::Value& val) noexcept;
}