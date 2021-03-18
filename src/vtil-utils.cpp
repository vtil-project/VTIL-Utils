#include "vtil-utils.hpp"

namespace fs = std::filesystem;

args::Group& commands()
{
	static args::Group instance("Commands");
	return instance;
}

std::vector<std::filesystem::path> enum_vtil_files(const std::filesystem::path& path)
{
	std::vector<fs::path> files;
	if(fs::is_directory(path))
	{
		for (const auto& entry : fs::recursive_directory_iterator(path))
		{
			if(!entry.is_regular_file())
				continue;

			const auto& p = entry.path();
			if(p.extension() == ".vtil")
			{
				files.push_back(p);
			}
		}
	}
	else
	{
		files.push_back(path);
	}
	return files;
}

using namespace vtil::logger;

int main(int argc, const char** argv)
{
	args::ArgumentParser parser("VTIL command line utility");
	args::CompletionFlag completion(parser, { "complete" });
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
	catch (const args::Completion& e)
	{
		std::cout << e.what();
		return 0;
	}
	catch (const args::Help&)
	{
		showHelp();
	}
	catch (const std::exception& e)
	{
		log<CON_RED>("[*] Error: %s\n\n", e.what());
		showHelp();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}