#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "REX/REX.h"

#include <expected>
#include <fmt/format.h>
#include <json/json.h>
#include <unordered_set>

#include "Plugin.h"

#define DLLEXPORT __declspec(dllexport)

#ifndef NDEBUG
    #define LOG_DEBUG(msg, ...) REX::DEBUG(msg, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(msg, ...)
#endif

using namespace std::literals;

template <class T>
inline constexpr bool always_false = false;

#define SECTION_SEPARATOR REX::INFO("=========================================================="sv)