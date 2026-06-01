#include "GlobalCondition.h"

#include "Settings/JSON/JSONSettings.h"

namespace ContainerManager::Conditions
{
    static std::optional<IndividualGlobal> ParseGlobData(const std::string& str,
        const std::string& path,
        GlobConditionFailure& failure)
    {
        IndividualGlobal result;
        auto parts = clib_util::string::split(str, "|"sv);
        auto partSize = parts.size();

        if (partSize > 3 || partSize < 2) {
            failure.AddMalformedGlobString(str, path);
            return std::nullopt;
        }
        auto parsed = GetFormFromString<RE::TESGlobal>(parts[0]);
        if (!parsed.value.has_value()) {
            switch (parsed.status) {
            case QueryResult::WrongFormtype:
            default:
                failure.AddWrongFormType(path, str);
                break;
            }
            return std::nullopt;
        }

        switch (parsed.status) {
        case QueryResult::FileNotFound: LOG_DEBUG("FileNotFound"sv); break;
        case QueryResult::FormatError: LOG_DEBUG("FormatError"sv); break;
        case QueryResult::FormNotInFile: LOG_DEBUG("FormNotInFile"sv); break;
        case QueryResult::GenericFailure: LOG_DEBUG("GenericFailure"sv); break;
        case QueryResult::MissingPo3Tweaks: LOG_DEBUG("MissingPo3Tweaks"sv); break;
        case QueryResult::Success: LOG_DEBUG("Success"sv); break;
        case QueryResult::WrongFormtype: LOG_DEBUG("WrongFormType"sv); break;
        }

        result.globID = parsed.value.value()->GetFormID();

        float numOne = 0.0f;
        try {
            numOne = std::stof(parts[1]);
        }
        catch (...) {
            failure.AddMalformedGlobString(str, path);
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
            failure.AddMalformedGlobString(str, path);
            return std::nullopt;
        }

        if (numOne > numTwo) {
            std::swap(numOne, numTwo);
        }
        result.max = numTwo;
        result.min = numOne;
        return result;
    }

    static bool CreateGlobConditionImpl(const Json::Value& from, 
        std::string& path, 
        GlobCondition& result,
        GlobConditionFailure& failure)
    {
        if (from.isArray()) {
            bool success = true;
            auto trimTo = path.size();
            for (Json::ArrayIndex i = 0u; i < from.size(); ++i) {
                path += "[" + std::to_string(i) + "]";
                success &= CreateGlobConditionImpl(from[i], path, result, failure);
                path.resize(trimTo);
            }
            return success;
        }
        if (from.isString()) {
            auto parsed = ParseGlobData(from.asString(), path, failure);
            if (!parsed) {
                return false;
            }
            std::vector<IndividualGlobal> individualConditions;
            individualConditions.emplace_back(std::move(parsed.value()));
            RequiredGlobals required;
            required.isOr = false;
            required._data = std::move(individualConditions);
            result.AppendCondition(required);
        }
        else if (from.isObject()) {
            bool isOr = false;
            auto members = from.getMemberNames();
            auto trimTo = path.size();
            bool success = true;
            std::vector<IndividualGlobal> individualConditions;
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
                                auto parsed = ParseGlobData(arrMember.asString(), path, failure);
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
                        auto parsed = ParseGlobData(complexMember.asString(), path, failure);
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
                failure.FlagEmptyCondition(path);
                return false;
            }
            RequiredGlobals required;
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

	std::expected<GlobCondition, GlobConditionFailure> CreateGlobCondition(const Json::Value& from, 
        std::string& path, 
        bool inverted)
	{
        GlobCondition result;
        result.SetInverted(inverted);
        GlobConditionFailure failure;
        if (!CreateGlobConditionImpl(from, path, result, failure)) {
            return std::unexpected(failure);
        }
        if (result.Empty()) {
            failure.FlagEmptyCondition(path);
        }
        if (failure.Errored()) {

            return std::unexpected(failure);
        }
        return result;
	}

    void GlobConditionFailure::Report(const std::string& a_prefix) const
    {
        if (!_empty.empty()) {
            logger::error("{}  {}"sv, a_prefix, _empty);
        }
        if (!_wrongFormTypes.empty()) {
            for (const auto& message : _wrongFormTypes) {
                logger::error("{}  {}"sv, a_prefix, message);
            }
        }
        if (!_wrongFieldTypes.empty()) {
            for (const auto& message : _wrongFieldTypes) {
                logger::error("{}  {}"sv, a_prefix, message);
            }
        }
        if (!_malformedGlobString.empty()) {
            for (const auto& message : _malformedGlobString) {
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

    void GlobConditionFailure::Preamble(const std::string& a_prefix) const {
        logger::error("{}  Global Condition Errors:"sv, a_prefix);
    }

    void GlobConditionFailure::FlagEmptyCondition(const std::string& path) {
        _empty = fmt::format("{} resolved as empty.", path);
    }

    void GlobConditionFailure::AddWrongFieldType(const std::string& path, 
        const std::string& receivedType, 
        const std::string& expected) 
    {
        std::string err = fmt::format("{}: Received field type {}, but expected {}."sv, path, receivedType, expected);
        _wrongFieldTypes.emplace_back(std::move(err));
    }

    void GlobConditionFailure::AddWrongFormType(const std::string& path, 
        const std::string& str) 
    {
        auto result = fmt::format("{}: {} resolved to a game form, but was not a global.", path, str);
        _wrongFormTypes.emplace_back(std::move(result));
    }

    void GlobConditionFailure::AddMalformedGlobString(const std::string& rawGlob, 
        const std::string& path) 
    {
        auto result = fmt::format("{}: malformed value ({}).", path, rawGlob);
        _malformedGlobString.emplace_back(std::move(result));
    }

    void GlobConditionFailure::AddWrongComplexMemberType(const std::string& path, 
        const std::string& expected, 
        const std::string& received) 
    {
        auto result = fmt::format("{} is of type {}, but expected {}.", path, received, expected);
        _wrongComplexMemberTypes.emplace_back(std::move(result));
    }

    void GlobConditionFailure::AddUnknownComplexMemberType(const std::string& path, 
        const std::string& receivedType) 
    {
        auto result = fmt::format("Encountered {} of type {}.", path, receivedType);
        _unknownComplexMemberTypes.emplace_back(std::move(result));
    }

    bool GlobCondition::IsValid(const ConditionCheckParams& a_params) const {
        (void)a_params;
        const bool evalResult = std::ranges::all_of(_data, [](const auto& required) {
            return required.IsValid();
            });
        return _inverted ? !evalResult : evalResult;
    }

    void GlobCondition::PrintCondition(const std::string& a_pref) const {
        logger::info("{}Global Variable Condition:"sv, a_pref);
        for (auto it = _data.begin(); it < _data.end(); ++it) {
            logger::info("{}  - {}{}{}{}"sv, a_pref, _inverted ? "[NOT]<" : "",
                it->GetFormattedDescription(), it < _data.end() - 1 ? " [AND]" : "",
                _inverted ? ">" : "");
        }
    }

    void GlobCondition::AppendCondition(RequiredGlobals& individualData) {
        _data.emplace_back(std::move(individualData));
    }
}