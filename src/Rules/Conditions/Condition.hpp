#pragma once

#include "Rules/ContainerData.hpp"

namespace Rules
{
    namespace Conditions
    {
        enum class ConditionType
        {
            Invalid,
            Worldspace
        };

        class ICondition
        {
        public:
            virtual ConditionType GetType() const { return ConditionType::Invalid; }

            virtual bool IsValid(ContainerData& data) const = 0;
            virtual ~ICondition() = default;
        };
    }
}