#pragma once

namespace ContainerManager
{
	enum class ErrorType
	{
		None,
		InvalidFieldType,
		InvalidFormType,
		Invalid
	};

	class Error
	{
	public:
		Error(const std::string& path);

		bool      IsFatal() const;
		ErrorType GetType() const;

		void SetFatal(bool fatal);
		void SetType(ErrorType type);
		void SetPayload(const std::string& payload);

	private:
		bool        _fatal = true;
		ErrorType   _type = ErrorType::None;
		std::string _path = "";
		std::string _payload = "";
	};

	class ErrorHolder
	{
	public:
		ErrorHolder();

		bool HasFatal() const;
		void PrintErrors() const;

		void AddError(Error& err);

	private:
		std::string        _name = "";
		std::vector<Error> _errors;
	};
}