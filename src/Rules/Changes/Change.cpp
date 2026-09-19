#include "Change.hpp"

namespace Rules::Changes
{
    void IChange::Apply(ContainerData &data) const {
        for (const auto& condition : _conditions) {
            if (!condition->IsValid(data)) {
                return;
            }
        }
        ModifyData(data);
    }

    void IChange::AddCondition(Condition &condition) {
        if (_conditions.empty()) {
            _conditions.emplace_back(std::move(condition));
            return;
        }

        auto where = std::ranges::find_if(_conditions, [type = condition->GetType()](const auto& rhs){
            return rhs->GetType() < type;
        });
        _conditions.emplace(where, std::move(condition));
    }
}