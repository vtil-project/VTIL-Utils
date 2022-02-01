#include "vtil-utils.hpp"

#include <vtil/compiler>

using namespace vtil;
using namespace logger;

static void vm_run(vtil::lambda_vm<symbolic_vm>& vm, vtil::routine* rtn)
{
	auto it = rtn->entry_point->begin();

	while (true)
	{
		auto [lim, rsn] = vm.run(it);
		if (lim.is_end())
			break;

		auto get_imm = [&](const operand& op) -> vip_t {
			if (op.is_immediate())
				return op.imm().u64;
			return *vm.read_register(op.reg())->get<vip_t>();
		};

		if (*lim->base == ins::vexit)
		{
			break;
		}
		else if (*lim->base == ins::js)
		{
			vip_t next = *vm.read_register(lim->operands[0].reg())->get<bool>()
							 ? get_imm(lim->operands[1])
							 : get_imm(lim->operands[2]);
			if (auto jit = rtn->explored_blocks.find(next); jit != rtn->explored_blocks.end())
				it = jit->second->begin();
			else
				break;
			vm.write_register(REG_SP, vm.read_register(REG_SP) + lim.block->sp_offset);
			continue;
		}
		else if (*lim->base == ins::jmp)
		{
			vip_t next = get_imm(lim->operands[0]);
			if (auto jit = rtn->explored_blocks.find(next); jit != rtn->explored_blocks.end())
				it = jit->second->begin();
			else
				break;
			vm.write_register(REG_SP, vm.read_register(REG_SP) + lim.block->sp_offset);
			continue;
		}

		else if (*lim->base == ins::vemit)
		{
			continue;
		}

		printf("unhandled lim->base: %s\n", lim->base->name.c_str());

		unreachable();
	}
}

static args::Command run(commands(), "run", "Run a .vtil file", [](args::Subparser& parser) {
	// Argument handling
	args::Positional<std::string> input(parser, "input", "Input .vtil file", args::Options::Required);
	args::Flag debug(parser, "debug", "Enable debug mode", { 'd', "debug" });
	parser.Parse();

	// Command implementation
	auto rtn = load_routine(input.Get());

	lambda_vm<symbolic_vm> vm;
	vm.hooks.execute = [&vm](const instruction& ins) -> vm_exit_reason {
		if (*ins.base == vtil::ins::vemit)
			warning("vemit\n");

		if (*ins.base == vtil::ins::vpinr || *ins.base == vtil::ins::vpinw)
			return vm_exit_reason::none;

		if (ins.base->is_branching_virt() || ins.base->is_branching_real())
			return vm_exit_reason::unknown_instruction;

		return vm.symbolic_vm::execute(ins);
	};

	vm_run(vm, rtn);
});
