#include "vtil-utils.hpp"
#include "pe_input.hpp"

#include <lifters/core>
#include <lifters/amd64>

using namespace vtil;
using namespace logger;

static args::Command lift(commands(), "lift", "Lift a .vtil file", [](args::Subparser& parser) {
	// Argument handling
	args::Positional<std::string> input(parser, "input", "Input executable file", args::Options::Required);
	args::Positional<std::string> output(parser, "output", "Output .vtil file", args::Options::Required);
	args::HexPositional<uintptr_t> argAddr(parser, "addr", "Virtual address of the function to lift", args::Options::Required);
	parser.Parse();

	// Command implementation
	auto addr = argAddr.Get();
	std::ifstream pe_stream(input.Get(), std::ifstream::binary);
	pe_stream.unsetf(std::ios::skipws); // ehhh
	if (!pe_stream.is_open())
		fatal("Could not open executable '%s'", input.Get());

	std::istream_iterator<uint8_t> pe_start(pe_stream), pe_end;
	std::vector<uint8_t> pe_bytes(pe_start, pe_end);

	pe_image image{ pe_bytes };
	if (!image.is_valid() || !image.is_pe64())
		fatal("Image is not a 64-bit PE file");

	if (addr < image.get_image_base() || addr >= image.get_image_base() + image.get_image_size())
		fatal("Address 0x%llx not inside image", addr);

	pe_input pe_vtil{ image };
	using amd64_recursive_descent = lifter::recursive_descent<pe_input, lifter::amd64::lifter_t>;
	amd64_recursive_descent rd(&pe_vtil, addr);

	rd.explore();
	auto rtn = rd.entry->owner;
	rtn->routine_convention = amd64::default_call_convention;
	rtn->routine_convention.purge_stack = true;

	save_routine(rtn, output.Get());
});
