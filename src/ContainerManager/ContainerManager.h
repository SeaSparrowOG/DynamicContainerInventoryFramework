#pragma once

namespace ContainerManager
{
    struct ConditionCheckParams
    {
        ConditionCheckParams(RE::TESObjectREFR* a_container);

        RE::ActorValueOwner* playerOwner{ nullptr };
		RE::TESObjectREFR*   reference{ nullptr };
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

    protected:
        std::vector<std::size_t> ids{};
    };

    enum class FailureType
    {
        None,
        UnknownField,
        MissingField
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
        bool recoverable{ true };
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
}