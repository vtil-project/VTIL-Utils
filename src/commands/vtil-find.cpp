#include "vtil-utils.hpp"

#include <vtil/compiler>
#include <filesystem>

using namespace vtil;
using namespace logger;
namespace fs = std::filesystem;

static args::Command find(commands(), "find", "Search in .vtil file(s)", [](args::Subparser& parser) {
	// Argument handling
	args::Positional<std::string> argInput(parser, "input", "Input .vtil file/folder", args::Options::Required);
	args::HexPositional<uintptr_t> argValue(parser, "value", "Value to find", args::Options::Required);
	parser.Parse();

	// Command implementation
	auto input = argInput.Get();
	auto value = argValue.Get();

	for (const auto& file : enum_vtil_files(input))
	{
		log("%s:\n", file.string());

		auto* rtn = load_routine(file);

		auto found = 0;
		for (const vtil::basic_block* blk : *rtn)
		{
			int ins_idx = 0;
			for (auto it = blk->begin(); !it.is_end(); ++it, ins_idx++)
			{
				const vtil::instruction& ins = *it;

				for (const vtil::operand& op : ins.operands)
				{
					if (op.is_immediate())
					{
						if (op.imm().u64 == value)
						{
							found++;

							// Print string context if any.
							//
							if (it->context.has<std::string>())
							{
								const std::string& cmt = it->context;
								log<CON_GRN>("// %s\n", cmt);
							}

							log<CON_BLU>("%04d: ", ins_idx);
							if (it->vip == invalid_vip)
								log<CON_DEF>("[ PSEUDO ] ");
							else
								log<CON_DEF>("[%08x] ", (uint32_t)it->vip);
							debug::dump(*it, it.is_begin() ? nullptr : &*std::prev(it));

							break;
						}
					}
				}
			}
		}

		if (found == 0)
		{
			log("Nothing found");
		}
	}
	return;

	//auto rtn = load_routine(input.Get());
	//optimizer::apply_all_profiled(rtn);
	//save_routine(rtn, output.Get());
});
