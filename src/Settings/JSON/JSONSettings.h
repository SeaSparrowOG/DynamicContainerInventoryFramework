#pragma once

namespace Settings
{
	namespace JSON
	{
		[[nodiscard]] bool Preload();

		class Holder : public REX::Singleton<Holder>
		{
		public:
			using ConfigMap = std::map<std::string, Json::Value>;

			[[nodiscard]] bool ParseConfigs();
			[[nodiscard]] bool PreloadConfigs();

			void Release();
			void LogErrors() const;
		private:
			using StringMap = std::map<std::string, std::string>;

			struct ConfigStatus
			{
				std::vector<std::string> duplicateKeys{};
				std::vector<std::string> emptyObjects{};
				std::vector<std::string> deeplyNestedObjects{};

				bool IsValid() const noexcept {
					return duplicateKeys.empty() && emptyObjects.empty() && deeplyNestedObjects.empty();
				}
			};

			ConfigMap                           _configs{};
			ConfigStatus                        _currentStatus{};
			std::map<std::string, ConfigStatus> _errors{};

			StringMap          GatherData(const Json::Value& a_obj, const std::string& a_path);
			void               ShallowCanonicalization(Json::Value& a_obj, const StringMap& a_mappings);
			void               CanonicalizeObject(Json::Value& a_obj, std::string& a_path, std::size_t a_depth = 0u);

			[[nodiscard]] bool ParseConfigObject(const Json::Value& a_rawRule, const std::string& a_path);
			[[nodiscard]] bool ParseOutdatedConfig(const Json::Value& a_RulesArray, const std::string& a_name);

			inline static constexpr std::size_t RECURSION_LIMIT = 16u;
			inline static constexpr std::string_view TOP_LEVEL_RULES_FIELD = "rules"sv;
		};
	}
}