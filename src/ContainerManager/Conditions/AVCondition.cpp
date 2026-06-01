#include "AVCondition.h"

#include "Settings/JSON/JSONSettings.h"

namespace ContainerManager::Conditions
{
    static std::string GetJSONTypeAsString(const Json::Value& val) {
        switch (val.type()) {
        case Json::ValueType::arrayValue: return "Array";
        case Json::ValueType::booleanValue: return "Boolean";
        case Json::ValueType::intValue:
        case Json::ValueType::uintValue:
            return "Integer";
        case Json::ValueType::objectValue: return "Object";
        case Json::ValueType::realValue: return "Float";
        case Json::ValueType::stringValue: return "String";
        default:
            return "NULL";
        }
    }

    static std::optional<RE::ActorValue> GetAVFromString(const std::string& str) {

        auto* avl = RE::ActorValueList::GetSingleton();
        auto av = avl ? avl->LookupActorValueByName(str.c_str()) : RE::ActorValue::kNone;
        if (av == RE::ActorValue::kNone) {
            return std::nullopt;
        }
        return av;
    }

    static std::optional<IndividualCondition> ParseAVData(const std::string& str, 
        const std::string& path, 
        AVConditionFailure& failure) 
    {
        IndividualCondition result;
        auto parts = clib_util::string::split(str, "|"sv);
        auto partSize = parts.size();

        if (partSize > 3 || partSize < 2) {
            failure.AddMalformedAVString(str, path);
            return std::nullopt;
        }
        auto parsed = GetAVFromString(parts[0]);
        if (!parsed) {
            return std::nullopt;
        }
        result.val = parsed.value();

        float numOne = 0.0f;
        try {
            numOne = std::stof(parts[1]);
        }
        catch (...) {
            failure.AddMalformedAVString(str, path);
            return std::nullopt;
        }

        if (partSize == 2) {
            result.bound = false;
            result.min = numOne;
            return result;
        }

        result.bound = true;
        float numTwo = 0.0f;
        try {
            numTwo = std::stof(parts[2]);
        }
        catch (...) {
            failure.AddMalformedAVString(str, path);
            return std::nullopt;
        }

        if (numOne > numTwo) {
            std::swap(numOne, numTwo);
        }
        result.max = numTwo;
        result.min = numOne;
        return result;
    }

    static bool CreateAVConditionImpl(const Json::Value& from, std::string& path, AVCondition& result, AVConditionFailure& failure) {
        if (from.isArray()) {
            bool success = true;
            auto trimTo = path.size();
            for (Json::ArrayIndex i = 0u; i < from.size(); ++i) {
                path += "[" + std::to_string(i) + "]";
                success &= CreateAVConditionImpl(from[i], path, result, failure);
                path.resize(trimTo);
            }
            return success;
        }

        if (from.isString()) {
            auto parsed = ParseAVData(from.asString(), path, failure);
            if (!parsed) {
                return false;
            }
            std::vector<IndividualCondition> individualConditions;
            individualConditions.emplace_back(std::move(parsed.value()));
            Required required;
            required.isOr = false;
            required._data = std::move(individualConditions);
            result.AppendCondition(required);
        }
        // This should probably be in its own function
        else if (from.isObject()) {
            bool isOr = false;
            auto members = from.getMemberNames();
            auto trimTo = path.size();
            bool success = true;
            std::vector<IndividualCondition> individualConditions;
            for (const auto& member : members) {
                path += "|" + member;
                const auto& complexMember = from[member];
                if (member == "isor") {
                    if (complexMember.isBool()) {
                        isOr = complexMember.asBool();
                    }
                    else {
                        success = false;
                        auto typeStr = GetJSONTypeAsString(complexMember);
                        failure.AddWrongComplexMemberType(path, "Boolean", typeStr);
                    }
                }
                else if (member == "data") {
                    if (complexMember.isArray()) {
                        auto extraTrim = path.size();
                        for (Json::ArrayIndex i = 0u; i < complexMember.size(); ++i) {
                            path += "[" + std::to_string(i) + "]";
                            const auto& arrMember = complexMember[i];
                            if (arrMember.isString()) {
                                auto parsed = ParseAVData(arrMember.asString(), path, failure);
                                if (!parsed) {
                                    success = false;
                                }
                                else {
                                    individualConditions.emplace_back(std::move(parsed.value()));
                                }
                            }
                            else {
                                failure.AddWrongComplexMemberType(path, "String", GetJSONTypeAsString(arrMember));
                                success = false;
                            }
                            path.resize(extraTrim);
                        }
                    }
                    else if (complexMember.isString()) {
                        auto parsed = ParseAVData(complexMember.asString(), path, failure);
                        if (!parsed) {
                            success = false;
                        }
                        else {
                            individualConditions.emplace_back(std::move(parsed.value()));
                        }
                    }
                    else {
                        success = false;
                        auto typeStr = GetJSONTypeAsString(complexMember);
                        failure.AddWrongComplexMemberType(path, "String or Array", typeStr);
                    }
                }
                else {
                    success = false;
                    auto typeStr = GetJSONTypeAsString(complexMember);
                    failure.AddUnknownComplexMemberType(path, typeStr);
                }
                path.resize(trimTo);
            }
            if (!success) {
                return false;
            }
            if (individualConditions.empty()) {
                failure.FlagEmptyAVVector(path);
                return false;
            }
            Required required;
            required.isOr = isOr;
            required._data = std::move(individualConditions);
            result.AppendCondition(required);
        }
        else {
            auto asType = GetJSONTypeAsString(from);
            failure.AddWrongFieldType(path, asType, "String, Array, or Object");
            return false;
        }
        return true;
    }

    void AVConditionFailure::Report(const std::string& a_prefix) const {
        // TODO: Better printing
        if (!_emptyAVVectors.empty()) {
            for (const auto& message : _emptyAVVectors) {
                logger::error("{}  {}"sv, a_prefix, message);
            }
        }
        if (!_wrongFieldTypes.empty()) {
            for (const auto& message : _wrongFieldTypes) {
                logger::error("{}  {}"sv, a_prefix, message);
            }
        }
        if (!_malformedAVStrings.empty()) {
            for (const auto& message : _malformedAVStrings) {
                logger::error("{}  {}"sv, a_prefix, message);
            }
        }
        if (!_wrongComplexMemberTypes.empty()) {
            for (const auto& message : _wrongComplexMemberTypes) {
                logger::error("{}  {}"sv, a_prefix, message);
            }
        }
        if (!_unknownComplexMemberTypes.empty()) {
            for (const auto& message : _unknownComplexMemberTypes) {
                logger::error("{}  {}"sv, a_prefix, message);
            }
        }
    }

    void AVConditionFailure::Preamble(const std::string& a_prefix) const {
        logger::error("{}  AVCondition Errors:"sv, a_prefix);
    }

    void AVConditionFailure::FlagEmptyAVVector(const std::string& path) {
        auto result = fmt::format("{}: Resolved empty (potentially needs Actor Value Generator)", path);
        _emptyAVVectors.emplace_back(std::move(result));
    }

    void AVConditionFailure::AddWrongFieldType(const std::string& path, const std::string& receivedType, const std::string& expected) {
        auto result = fmt::format("{}: got type {}, but expected {}.", path, receivedType, expected);
        _wrongFieldTypes.emplace_back(std::move(result));
    }

    void AVConditionFailure::AddMalformedAVString(const std::string& rawAV, const std::string& path) {
        auto result = fmt::format("{}: malformed value ({}).", path, rawAV);
        _malformedAVStrings.emplace_back(std::move(result));
    }

    void AVConditionFailure::AddWrongComplexMemberType(const std::string& path, const std::string& expected, const std::string& receivedType) {
        auto result = fmt::format("{} is of type {}, but expected {}.", path, receivedType, expected);
        _wrongComplexMemberTypes.emplace_back(std::move(result));
    }

    void AVConditionFailure::AddUnknownComplexMemberType(const std::string& path, const std::string& receivedType) {
        auto result = fmt::format("Encountered {} of type {}.", path, receivedType);
        _unknownComplexMemberTypes.emplace_back(std::move(result));
    }

    bool AVCondition::IsValid(const ConditionCheckParams& a_params) const {
        auto* owner = a_params.playerOwner;
        const bool evalResult = std::ranges::all_of(_data, [&](const auto& required) {
            return required.IsValid(owner);
            });
        return _inverted ? !evalResult : evalResult; //xor
    }

    void AVCondition::PrintCondition(const std::string& a_pref) const {
        logger::info("{}AV Condition:"sv, a_pref);
        for (auto it = _data.begin(); it < _data.end(); ++it) {
            logger::info("{}  - {}{}{}{}"sv, a_pref, _inverted ? "[NOT]<" : "", 
                it->GetFormattedDescription(), it < _data.end() - 1 ? " [AND]" : "",
                _inverted ? ">" : "");
        }
    }

    void AVCondition::AppendCondition(Required& individualData) {
        _data.emplace_back(std::move(individualData));
    }

    std::expected<AVCondition, AVConditionFailure> CreateAVCondition(const Json::Value& from, std::string& path, bool inverted)
    {
        AVCondition result;
        result.SetInverted(inverted);
        AVConditionFailure failure;
        if (!CreateAVConditionImpl(from, path, result, failure)) {
            return std::unexpected(failure);
        }
        return result;
    }
}