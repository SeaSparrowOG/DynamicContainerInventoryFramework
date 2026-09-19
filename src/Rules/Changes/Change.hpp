#pragma once

#include "Rules/ContainerData.hpp"
#include "Rules/Conditions/Condition.hpp"

namespace Rules
{
    namespace Changes
    {
        enum class ChangeType
        {
            Invalid,
            Add,
            Remove,
            RemoveByKeyword,
            Replace,
            ReplaceByKeyword
        };

        class IChange
        {
        public:
            using Condition = std::unique_ptr<Rules::Conditions::ICondition>;

            void Apply(ContainerData& data) const;
            void AddCondition(Condition& condition);

            virtual ChangeType GetType() const { return ChangeType::Invalid; }

            virtual ~IChange() = default;
        private:
            virtual void ModifyData(ContainerData& data) const = 0;

            std::vector<Condition> _conditions{};
        };
    }
}