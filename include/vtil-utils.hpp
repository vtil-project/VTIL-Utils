#pragma once

#include <vtil/io>
#include <vtil/arch>

#include <args.hxx>

// Some extensions for the arguments library
namespace args
{
struct HexValueReader
{
	template<typename T>
	typename std::enable_if<!std::is_assignable<T, std::string>::value, bool>::type
	operator()(const std::string& name, const std::string& value, T& destination)
	{
		std::istringstream ss(value);
		bool failed = !(ss >> std::hex >> destination);

		if (!failed)
		{
			ss >> std::ws;
		}

		if (ss.rdbuf()->in_avail() > 0 || failed)
		{
#ifdef ARGS_NOEXCEPT
			(void)name;
			return false;
#else
			std::ostringstream problem;
			problem << "Argument '" << name << "' received invalid value type '" << value << "'";
			throw args::ParseError(problem.str());
#endif
		}
		return true;
	}

	template<typename T>
	typename std::enable_if<std::is_assignable<T, std::string>::value, bool>::type
	operator()(const std::string&, const std::string& value, T& destination)
	{
		destination = value;
		return true;
	}
};

template<typename T>
using HexValueFlag = args::ValueFlag<T, HexValueReader>;

template<typename T>
using HexPositional = args::Positional<T, HexValueReader>;
} // namespace args

args::Group& commands();

template<typename... Args>
inline void fatal(const char* fmt, Args&&... args)
{
	auto str = vtil::format::str(fmt, std::forward<Args>(args)...);
	while (!str.empty() && (str.back() == '\r' || str.back() == '\n'))
		str.pop_back();
	throw std::runtime_error(str);
}
