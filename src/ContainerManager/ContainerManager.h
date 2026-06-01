#pragma once

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
    };

    enum class FailureType
    {
        None,
        UnknownField,
        MissingField,
        InvalidFieldType,

        AVCondition,
        BaseFormCondition
    };

    class ParseFailure
    {
    public:
        virtual bool Recoverable() const { return recoverable; };
        virtual void Report(const std::string& a_prefix) const = 0;
        virtual void Preamble(const std::string& a_prefix) const = 0;
        virtual FailureType GetType() const { return type; };

        virtual ~ParseFailure() = default;

    protected:
        bool recoverable{ false };
        FailureType type{ FailureType::None };
    };

    class UnknownFieldFailure : public ParseFailure
    {
    public:
        virtual void Report(const std::string& a_prefix) const override {
            for (const auto& unknown : _unknownFields) {
                logger::error("{}    - {}"sv, a_prefix, unknown);
            }
        }

        virtual void Preamble(const std::string& a_prefix) const override {
            logger::error("{}  The following fields were present in the config, but were not recognized:"sv, a_prefix);
        }

        void AddUnknownField(const std::string& a_path) { _unknownFields.emplace_back(fmt::format("{}|{}"sv, _root, a_path)); }
        UnknownFieldFailure(const std::string& path) :
            _root{ path }
        {
            recoverable = false;
            type = FailureType::UnknownField;
        }

    private:
        std::string              _root = {};
        std::vector<std::string> _unknownFields = {};
    };

    class MissingFieldFailure : public ParseFailure
    {
    public:
        virtual void Report(const std::string& a_prefix) const override {
            for (const auto& missing : _missingFields) {
                logger::error("{}    - {}"sv, a_prefix, missing);
            }
        }

        virtual void Preamble(const std::string& a_prefix) const override {
            logger::error("{}  There are missing fields for this config. At least one of these were expected:"sv, a_prefix);
        }

        void AddMissingField(const std::string& a_path) { _missingFields.emplace_back(fmt::format("{}|{}"sv, _root, a_path)); }
        MissingFieldFailure(const std::string& path) :
            _root{ path }
        {
            recoverable = false;
            type = FailureType::MissingField;
        }

    private:
        std::string              _root = {};
        std::vector<std::string> _missingFields = {};
    };

    class InvalidFieldTypeFailure : public ParseFailure
    {
    public:
        virtual void Report(const std::string& a_prefix) const override {
            for (const auto& missing : _invalidField) {
                logger::error("{}    - {}"sv, a_prefix, missing);
            }
        }

        virtual void Preamble(const std::string& a_prefix) const override {
            logger::error("{}  Some fields did not resolve to their expected type:"sv, a_prefix);
        }

        void AddInvalidField(const std::string& a_path) { _invalidField.emplace_back(fmt::format("{}|{}"sv, _root, a_path)); }
        InvalidFieldTypeFailure(const std::string& path) :
            _root{ path }
        {
            recoverable = false;
            type = FailureType::InvalidFieldType;
        }

    private:
        std::string              _root = {};
        std::vector<std::string> _invalidField = {};
    };

    class InventorySwapper : public REX::Singleton<InventorySwapper>
    {
    public:
        void Report() const;
        void RegisterChange(std::unique_ptr<Change> a_change);
        void RegisterFailure(std::unique_ptr<ParseFailure> a_failure);
        [[nodiscard]] std::size_t RegisterCondition(std::unique_ptr<Condition> a_condition);

        void ManipulateInventory(RE::TESObjectREFR* a_container);
    private:
        std::vector<std::unique_ptr<ParseFailure>> parseErrors{};
        std::vector<std::unique_ptr<Condition>>    conditions{};
        std::vector<std::unique_ptr<Change>>       changes{}; // Sorted vector.
    };

    void PrintSwaps();

    inline static std::string GetJSONTypeAsString(const Json::Value& val) {
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

    // Helper function for extracting forms from a string
    template <typename T>
    struct form_type
    {
        static constexpr RE::FormType value = RE::FormType::None;
    };

    template <>
    struct form_type<RE::BGSKeyword> { static constexpr RE::FormType value = RE::FormType::Keyword; };
    template <>
    struct form_type<RE::BGSLocationRefType> { static constexpr RE::FormType value = RE::FormType::LocationRefType; };
    template<>
    struct form_type<RE::TESGlobal> { static constexpr RE::FormType value = RE::FormType::Global; };
    template<>
    struct form_type<RE::TESRace> { static constexpr RE::FormType value = RE::FormType::Race; };
    template<>
    struct form_type<RE::TESSound> { static constexpr RE::FormType value = RE::FormType::Sound; };
    template<>
    struct form_type<RE::TESObjectCELL> { static constexpr RE::FormType value = RE::FormType::Cell; };
    template<>
    struct form_type<RE::TESWorldSpace> { static constexpr RE::FormType value = RE::FormType::WorldSpace; };
    template<>
    struct form_type<RE::TESQuest> { static constexpr RE::FormType value = RE::FormType::Quest; };
    template<>
    struct form_type<RE::TESIdleForm> { static constexpr RE::FormType value = RE::FormType::Idle; };
    template<>
    struct form_type<RE::TESObjectANIO> { static constexpr RE::FormType value = RE::FormType::AnimatedObject; };
    template<>
    struct form_type<RE::TESImageSpaceModifier> { static constexpr RE::FormType value = RE::FormType::ImageAdapter; };
    template<>
    struct form_type<RE::BGSVoiceType> { static constexpr RE::FormType value = RE::FormType::VoiceType; };
    template<>
    struct form_type<RE::BGSMusicType> { static constexpr RE::FormType value = RE::FormType::MusicType; };
    template<>
    struct form_type<RE::BGSSoundDescriptorForm> { static constexpr RE::FormType value = RE::FormType::SoundRecord; };

    constexpr bool SupportsEDIDWithoutTweaks(RE::FormType type)
    {
        switch (type) {
        case RE::FormType::Keyword:
        case RE::FormType::LocationRefType:
        case RE::FormType::Global:
        case RE::FormType::Race:
        case RE::FormType::Sound:
        case RE::FormType::Cell:
        case RE::FormType::WorldSpace:
        case RE::FormType::Quest:
        case RE::FormType::Idle:
        case RE::FormType::AnimatedObject:
        case RE::FormType::ImageAdapter:
        case RE::FormType::VoiceType:
        case RE::FormType::MusicType:
        case RE::FormType::SoundRecord:
            return true;
        default:
            return false;
        }
    }

    enum class QueryResult
    {
        Success,          // Success
        FormatError,      // String is in an invalid format (EditorID while PO3's Tweaks is not present, FormID not hex, etc)
        FileNotFound,     // ESP/ESM/ESL missing
        FormNotInFile,    // Master exists, but form is not present
        WrongFormtype,    // Form exists in given file, but type is wrong.

        MissingPo3Tweaks, // EditorID query that requires PO3's tweaks but PO3's tweaks is not present.
        GenericFailure    // Catchall (might be missing data handler, cosmic ray, etc)
    };

    template <typename T>
    struct QueryData
    {
        std::optional<T*> value;
        QueryResult status;
    };


    template <typename T>
    QueryData<T> GetFormFromString(const std::string& a_str) {
        constexpr bool supportsEDID = SupportsEDIDWithoutTweaks(form_type<T>::value);

        auto response = QueryData<T>{ std::nullopt, QueryResult::Success };

        static auto* dh = RE::TESDataHandler::GetSingleton();
        if (!dh) {
            response.status = QueryResult::GenericFailure;
            return response;
        }

        static auto* tweaks = REX::W32::GetModuleHandleW(L"po3_Tweaks.dll");

        auto parts = clib_util::string::split(a_str, "|");
        std::string modName = "";
        RE::FormID formID = 0;
        RE::TESForm* form = nullptr;
        T* castForm = nullptr;

        switch (parts.size()) {
        case 1:
            // EDID
            if constexpr (!supportsEDID) {
                if (!tweaks) {
                    response.status = QueryResult::MissingPo3Tweaks;
                    return response;
                }
            }
            form = RE::TESForm::LookupByEditorID<RE::TESForm>(a_str);
            if (!form) {
                response.value = nullptr;
                return response;
            }
            castForm = form->As<T>();
            if (!castForm) {
                response.status = QueryResult::WrongFormtype;
                return response;
            }
            response.value = castForm;
            return response;
        case 2:
            // FormID
            if (clib_util::string::is_only_hex(parts[1])) {
                formID = clib_util::string::to_num<RE::FormID>(parts[1], true);
                modName = parts[0];
            }
            else if (clib_util::string::is_only_hex(parts[0])) {
                formID = clib_util::string::to_num<RE::FormID>(parts[0], true);
                modName = parts[1];
            }
            else {
                response.status = QueryResult::FormatError;
                return response;
            }
            if (!dh->LookupModByName(modName)) {
                response.status = QueryResult::FileNotFound;
                return response;
            }

            form = dh->LookupForm(formID, modName);
            if (!form) {
                response.status = QueryResult::FormNotInFile;
                return response;
            }
            castForm = form->As<T>();
            if (!castForm) {
                response.status = QueryResult::WrongFormtype;
                return response;
            }
            response.value = castForm;
            return response;
        default:
            response.status = QueryResult::FormatError;
            break;
        }
        return response;
    }
}