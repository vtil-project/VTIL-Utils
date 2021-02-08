#include "vtil-utils.hpp"

args::Group& commands()
{
	static args::Group instance("Commands");
	return instance;
}

using namespace vtil::logger;

int main(int argc, const char** argv)
{
	args::ArgumentParser parser("VTIL command line utility");
	parser.Add(commands());

	args::Group arguments("Arguments");
	args::HelpFlag help(arguments, "help", "Display this help menu", { 'h', "help" });
	args::GlobalOptions globals(parser, arguments);

	auto showHelp = [&parser]() {
		auto help = parser.Help();
		while (!help.empty() && help.back() == '\n')
			help.pop_back();
		log("%s\n", help);
	};

	try
	{
		parser.ParseCLI(argc, argv);
	}
	catch (args::Help&)
	{
		showHelp();
	}
	catch (std::exception& e)
	{
		log<CON_RED>("[*] Error: %s\n\n", e.what());
		showHelp();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}