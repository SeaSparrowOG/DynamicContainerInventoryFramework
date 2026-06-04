#include "Error.h"

namespace ContainerManager
{
	Error::Error(const std::string& path) {
		_path = path;
	}

	bool Error::IsFatal() const {
		return _fatal;
	}

	ErrorType Error::GetType() const {
		return _type;
	}

	void Error::SetFatal(bool fatal) {
		_fatal = fatal;
	}

	void Error::SetType(ErrorType type) {
		_type = type;
	}

	void Error::SetPayload(const std::string& payload) {
		_payload = "- " + payload;
	}

	ErrorHolder::ErrorHolder() {
	}

	bool ErrorHolder::HasFatal() const {
		for (const auto& err : _errors) {
			if (err.IsFatal()) {
				return true;
			}
		}
		return false;
	}

	void ErrorHolder::AddError(Error& err) {
		auto it = std::find_if(_errors.begin(), _errors.end(), [err](const Error& other) {
			return err.GetType() < other.GetType();
			});
		_errors.emplace(it, err);
	}

	void ErrorHolder::PrintErrors() const {
		// tbd
	}
}