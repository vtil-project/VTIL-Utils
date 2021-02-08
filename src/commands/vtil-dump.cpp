#include "vtil-utils.hpp"

using namespace vtil;
using namespace logger;

// TODO: add format argument (to print to html)
static args::Command dump(commands(), "dump", "Dump a .vtil file", [](args::Subparser& parser) {
	// Argument handling
	args::Positional<std::string> input(parser, "input", "File to dump", args::Options::Required);
	parser.Parse();

	// Command implementation
	auto rtn = load_routine(input.Get());
	debug::dump(rtn);
});