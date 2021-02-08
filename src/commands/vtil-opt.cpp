#include "vtil-utils.hpp"

#include <vtil/compiler>

using namespace vtil;
using namespace logger;

// TODO: add arguments for calling convention/stack purge/passes
// TODO: add flag to enable/disable profiling output
static args::Command opt(commands(), "opt", "Optimize a .vtil file", [](args::Subparser& parser) {
	// Argument handling
	args::Positional<std::string> input(parser, "input", "Input .vtil file", args::Options::Required);
	args::Positional<std::string> output(parser, "output", "Output .vtil file", args::Options::Required);
	parser.Parse();

	// Command implementation
	auto rtn = load_routine(input.Get());
	optimizer::apply_all_profiled(rtn);
	save_routine(rtn, output.Get());
});
