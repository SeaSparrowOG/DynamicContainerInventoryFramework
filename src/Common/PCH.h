#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "REX/REX.h"

#include <expected>
#include <spdlog/sinks/basic_file_sink.h>
#include <unordered_set>
#include <json/json.h>

#include "Plugin.h"

#define DLLEXPORT __declspec(dllexport)

#ifndef NDEBUG
#define LOG_DEBUG(msg, ...) logger::debug(msg, ##__VA_ARGS__)
#else
#define LOG_DEBUG(msg, ...)
#endif

using namespace std::literals;

// Used as a compile guard in certain templated function (see INISettings.h, if present)
template <class T>
inline constexpr bool always_false = false;

#define SECTION_SEPARATOR REX::INFO("=========================================================="sv)