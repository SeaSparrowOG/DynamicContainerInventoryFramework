#pragma once




/* added: quest comparison */

namespace Comparison {
    enum class CompareOp {
        kGreater,
        kGreaterOrEqual,
        kEqual,
        kLessOrEqual,
        kLess,

        kUndefined
    }; 

    static constexpr inline CompareOp ParseOperator(std::string_view str) {
        if (str == ">") {
            return CompareOp::kGreater;
        } 
        if (str == ">=") {
            return CompareOp::kGreaterOrEqual;
        } 
        if (str == "=") {
            return CompareOp::kEqual;
        } 
        if (str == "<") {
            return CompareOp::kLess;
        } 
        if (str == "<=") {
            return CompareOp::kLessOrEqual;
        } 

        else {
            return CompareOp::kUndefined;
        }


    }

    static constexpr std::string_view OperatorToString(CompareOp op) {
        if (op == CompareOp::kGreater) {
            return ">";
        } 
        if (op == CompareOp::kGreaterOrEqual) {
            return ">=";
        } 
        if (op == CompareOp::kEqual) {
            return "=";
        } 
        if (op == CompareOp::kLess) {
            return "<";
        } 
        if (op == CompareOp::kLessOrEqual) {
            return "<=";
        } 



        else {
            return "UNDEFINED";
        }


    }


    static constexpr inline bool CompareResult(
        size_t num1,
        CompareOp op,
        size_t num2
    ) {

        switch (op) {
            case CompareOp::kGreater : {
                return num1 > num2;
            }
            case CompareOp::kGreaterOrEqual : {
                return num1 >= num2;
            }
            case CompareOp::kEqual : {
                return num1 == num2;
            }
            case CompareOp::kLessOrEqual : {
                return num1 <= num2;
            }
            case CompareOp::kLess : {
                return num1 < num2;
            }
            default : {
                return false;
            }
        } 
    }

};

using QuestCondition = std::pair<RE::TESQuest*, std::pair<uint16_t, Comparison::CompareOp>>;
/* end of add */