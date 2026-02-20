#pragma once

namespace Settings
{
	namespace JSON
	{
		struct FormatErrors
		{
			using StringVec = std::vector<std::string>;

			bool errored{ false };
			StringVec recursionTooDeep{};
			StringVec duplicateKeys{};
			StringVec emptyKeys{};
			std::string invalidTopLevel{""};
			std::string jsonError{ "" };
		};

		class ConfigHolder : public REX::Singleton<ConfigHolder>
		{
		public:
			void AddFailedConfig(const std::string& configName, FormatErrors& a_reason);
			void AddGoodConfig(const std::string& a_configName, Json::Value a_value);
			void Report() const;
			void Clear();

			/// <summary>
			/// Use this function to traverse the internal map of configs. This exposes the name and the value, as const references.
			/// </summary>
			/// <typeparam name="Func">Derived automatically from the passed in function.</typeparam>
			/// <param name="func">The function to apply to each key/val pair.</param>
			template<typename Func>
			requires std::invocable<Func,
				const std::string&,
				const Json::Value&>&&
				std::same_as<
				std::invoke_result_t<Func,
				const std::string&,
				const Json::Value&>,
				bool>
			bool Traverse(Func&& func) const {
				bool success = true;
				for (const auto& [key, value] : data_) {
					sucess &= func(key, value);
				}
				return success;
			}

		private:
			struct FailedConfig
			{
				std::string  config{};
				FormatErrors reason{};

				FailedConfig(FormatErrors& a_reason, const std::string& a_name) {
					config = a_name;
					reason = std::move(a_reason);
				}

				static void Delimit() {
					logger::error("-------------------------------------------------"sv);
				}

				void PrintReason(std::string a_whiteSpace = "    ") const {
					Delimit();
					logger::error("{}>{}"sv, a_whiteSpace, config);
					if (!reason.duplicateKeys.empty()) {
						logger::error("{}  [Duplicate Keys]"sv, a_whiteSpace);
						logger::error("{}    Duplicate Keys are keys that exist in the same object that have the same name. Capitalization doesn't matter."sv, a_whiteSpace);
						for (const auto& e : reason.duplicateKeys) {
							logger::error("{}      >{}"sv, a_whiteSpace, e);
						}
					}
					if (!reason.emptyKeys.empty()) {
						logger::error("{}  [Empty Objects/Arrays]"sv, a_whiteSpace);
						logger::error("{}    Empty Objects/Arrays are flagged as a mistake (and usually are)."sv, a_whiteSpace);
						for (const auto& e : reason.emptyKeys) {
							logger::error("{}      >{}"sv, a_whiteSpace, e);
						}
					}
					if (!reason.invalidTopLevel.empty()) {
						logger::error("{}  [Invalid Top Level]"sv, a_whiteSpace);
						logger::error("{}    The top level of the config MUST be either an object, or an array."sv, a_whiteSpace);
						logger::error("{}    Instead, this config's top level is: {}"sv, a_whiteSpace, reason.invalidTopLevel);
					}
					if (!reason.recursionTooDeep.empty()) {
						logger::error("{}  [Nesting Error]"sv, a_whiteSpace);
						logger::error("{}    Deeply nested objects are indicative of a config error, and stop the parsing."sv, a_whiteSpace);
						for (const auto& e : reason.recursionTooDeep) {
							logger::error("{}      >{}"sv, a_whiteSpace, e);
						}
					}
				}
			};

			std::vector<FailedConfig>          failedConfigs{};
			std::map<std::string, Json::Value> configs{};
		};

		/// <summary>
		/// Preloads all configs in the appropriate directory, reporting any structural errors in the process. Safe to call before kDataLoaded.
		/// </summary>
		/// <returns>True on success, False on failure.</returns>
		[[nodiscard]] bool Preload();

		static std::string GetFieldType(const Json::Value& a_field);

		inline static constexpr int PLUGIN_INDEX = 0;
		inline static constexpr int FORMID_INDEX = 1;
		static_assert(PLUGIN_INDEX >= 0, "PLUGIN_INDEX must be greater than or equal to 0.");
		static_assert(FORMID_INDEX >= 0, "FORMID_INDEX must be greater than or equal to 0.");
		static_assert(FORMID_INDEX != PLUGIN_INDEX, "PLUGIN_INDEX must be different from FORMID_INDEX.");

		// Used for form validation. Certain form types do NOT need to be retrieved through PO3's tweaks.
		template <typename T>
		struct form_type { static constexpr RE::FormType value = RE::FormType::None; };

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
			FormatError,      //String is in an invalid format (EditorID while PO3's Tweaks is not present, FormID not hex, etc)
			FileNotFound,     // ESP/ESM/ESL missing
			FormNotInFile,    // Master exists, but form is not present
			WrongFormtype,    // Form exists in given file, but type is wrong.

			MissingPo3Tweaks, // EditorID query that requires PO3's tweaks but PO3's tweaks is not present.
			GenericFailure    // Catchall (might be missing data handler, cosmic ray, etc)
		};

		static std::string QueryResultToString(QueryResult a_flag) {
			switch (a_flag) {
			case QueryResult::FileNotFound: return "FileNotFound";
			case QueryResult::FormatError: return "FormatError";
			case QueryResult::FormNotInFile: return "FormNotInFile";
			case QueryResult::GenericFailure: return "GenericFailure";
			case QueryResult::MissingPo3Tweaks: return "MissingPo3Tweaks";
			case QueryResult::WrongFormtype: return "WrongFormtype";
			default: return "Success";
			}
		}

		template <typename T>
		struct QueryData
		{
			std::optional<T*> value{ std::nullopt };
			QueryResult status{ QueryResult::Success };
		};


		template <typename T>
		QueryData<T> GetFormFromString(const std::string& a_str) {
			constexpr bool supportsEDID = SupportsEDIDWithoutTweaks(form_type<T>::value);

			auto response = QueryData<T>{};

			auto* dh = RE::TESDataHandler::GetSingleton();
			if (!dh) {
				response.status = QueryResult::GenericFailure;
				return response;
			}

			static auto* tweaks = REX::W32::GetModuleHandleW(L"po3_Tweaks.dll");

			auto parts = clib_util::string::split(a_str, "|");
			const RE::TESFile* mod;
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
				if (!clib_util::string::is_only_hex(parts[FORMID_INDEX], true)) {
					response.status = QueryResult::FormatError;
					return response;
				}
				formID = clib_util::string::to_num<RE::FormID>(parts[FORMID_INDEX], true);

				mod = dh->LookupModByName(parts[PLUGIN_INDEX]);
				if (!mod) {
					response.status = QueryResult::FileNotFound;
					return response;
				}
				form = dh->LookupForm(formID, mod->GetFilename());
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

		enum class JsonParseResult
		{
			Success,
			NotStringOrArray,
			NonHomogenousArray
		};

		static std::string JsonParseResultToString(JsonParseResult a_flag) {
			switch (a_flag) {
			case JsonParseResult::NotStringOrArray: return "NotStringOrArray";
			case JsonParseResult::NonHomogenousArray: return "NonHomogenousArray";
			default: return "Success";
			}
		}

		static JsonParseResult LoadFormStrings(const Json::Value& a_value, std::vector<std::string>& a_result)
		{
			if (a_value.isString()) {
				a_result.push_back(a_value.asString());
				return JsonParseResult::Success;
			}
			else if (a_value.isArray()) {
				const auto size = a_value.size();
				a_result.clear();
				a_result.reserve(size);

				for (const auto& value : a_value) {
					if (!value.isString()) {
						a_result.clear();
						return JsonParseResult::NonHomogenousArray;
					}
					a_result.push_back(value.asString());
				}
				return JsonParseResult::Success;
			}
			return JsonParseResult::NotStringOrArray;
		}
	}
}