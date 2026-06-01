#pragma once

namespace ContainerManager
{
	namespace Errors
	{
		enum class ErrorType
		{
			Invalid,
			EmptyCondition,
			FieldType,
			MissingField,
			MissingForm,
			UnknownField,
			UnresolvedForm
		};

		class IError
		{
		public:
			bool IsFatal() const{ return _fatal; }
			ErrorType GetType() const { return _type; }

			virtual void Report(const std::string& prefix) const {
				for (const auto& err : _data) {
					logger::error("{}  {}"sv, prefix, err);
				}
			}
			virtual bool Errored() const { return !_data.empty(); }

			virtual ~IError() = default;

		protected:
			bool                     _fatal = true;
			ErrorType                _type = ErrorType::Invalid;
			std::vector<std::string> _data = std::vector<std::string>();
		};
	}
}