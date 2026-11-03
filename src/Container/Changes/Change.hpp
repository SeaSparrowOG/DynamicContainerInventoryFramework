#pragma once

#include "Container/Conditions/Condition.hpp"

namespace Container::Changes {
    enum class Type
    {
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
        void RegisterConditionIDs(std::vector<size_t>& ids) noexcept { _ids = ids; }; // intentional copy

        virtual Type GetType() const noexcept = 0;
    private:
        std::vector<size_t> _ids{};
    };

    class ChangeFail {
    public:
        void Log(const std::string& prefix) const noexcept;
    };

    using Changes = std::vector<std::unique_ptr<Change>>;

    [[nodiscard]]
    std::expected<Changes, ChangeFail> create_change(const Json::Value& val) noexcept;
}