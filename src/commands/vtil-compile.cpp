// Copyright © 2021 Keegan Saunders
//
// Permission to use, copy, modify, and/or distribute this software for
// any purpose with or without fee is hereby granted.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
// OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//

#include <asmjit/x86.h>
#include <asmjit/x86/x86operand.h>
#include <set>
#include <unordered_map>
#include <vtil/arch>
#include <vtil/vtil>
#include <vtil-utils.hpp>

using namespace asmjit;
namespace ins
{
using namespace vtil::ins;
};

static void compile(vtil::basic_block* basic_block, struct routine_state* state);

struct routine_state
{
	std::unordered_map<vtil::vip_t, Label> label_map;
	std::set<vtil::vip_t> is_compiled;
	std::unordered_map<vtil::operand::register_t, x86::Gp> reg_map;
	x86::Gp flags_reg;
	x86::Compiler& cc;

	routine_state(x86::Compiler& cc)
		: cc(cc)
	{
	}

	Label get_label(vtil::vip_t address)
	{
		if (label_map.count(address))
		{
			return label_map.at(address);
		}
		else
		{
			// TODO: Create a namedLabel
			//
			Label label = cc.newLabel();
			label_map.insert({ address, label });
			return label;
		}
	}

	x86::Gp reg_for_size(vtil::operand const& operand)
	{
		switch (operand.bit_count())
		{
		// TODO: Handle sized register access
		//
		case 1:
		case 8:
		case 16:
		case 32:
		case 64:
			return cc.newGpq();
		default:
			unreachable();
		}
	}

	x86::Gp tmp_imm(vtil::operand const& reg)
	{
		x86::Gp tmp = reg_for_size(reg);
		cc.mov(tmp, reg.imm().i64);
		return tmp;
	}

	x86::Gp get_reg(vtil::operand::register_t const& operand)
	{
		using vtil::logger::log;

		// TODO: Handle bit selectors on registers

		log("get_reg: %s\n", operand.to_string());
		if (operand.is_physical())
		{
			log("\tis_physical\n");
			// Transform the VTIL register into an AsmJit one.
			//
			// TODO: This shouldnt be a separate condition, but just
			// in the same switch
			//
			if (operand.is_stack_pointer())
			{
				log("\t\tis_stack_pointer\n");
				// TODO: this might cause problems, the stack
				// of the program and of VTIL are shared
				//
				return x86::rsp;
			}
			else if (operand.is_flags())
			{
				log("\t\tis_flags: %d\n", flags_reg.isValid());
				if (!flags_reg.isValid())
				{
					flags_reg = cc.newGpq();
				}

				return flags_reg;
			}
			else
			{
				log("\t\tmachine_register: %s\n", vtil::amd64::name(operand.combined_id));
				switch (operand.combined_id)
				{
				case X86_REG_R8:
					return x86::r8;
				case X86_REG_R9:
					return x86::r9;
				case X86_REG_R10:
					return x86::r10;
				case X86_REG_R11:
					return x86::r11;
				case X86_REG_R12:
					return x86::r12;
				case X86_REG_R13:
					return x86::r13;
				case X86_REG_R14:
					return x86::r14;
				case X86_REG_R15:
					return x86::r15;
				case X86_REG_RSI:
					return x86::rsi;
				case X86_REG_RBP:
					return x86::rbp;
				case X86_REG_RDI:
					return x86::rdi;
				case X86_REG_RAX:
					return x86::rax;
				case X86_REG_RBX:
					return x86::rbx;
				case X86_REG_RCX:
					return x86::rcx;
				case X86_REG_RDX:
					return x86::rdx;
				default:
					abort();
				}
			}
		}
		else
		{
			log("\tis_virtual\n");

			if (operand.is_image_base())
			{
				log("\t\tis_image_base\n");
				// TODO: This obviously won't work for different
				// base addresses
				//
				x86::Gp base_reg = reg_for_size(operand);
				cc.mov(base_reg, Imm(0x140'000'000));
				return base_reg;
			}
			else if (operand.is_flags())
			{
				log("\t\tis_flags\n");
				abort();
			}
			// Grab the register from the map, or create and insert otherwise.
			//
			else if (reg_map.count(operand))
			{
				return reg_map[operand];
			}
			else
			{
				x86::Gp reg = reg_for_size(operand);
				reg_map[operand] = reg;
				return reg;
			}
		}
	}
};

using fn_instruction_compiler_t = std::function<void(const vtil::il_iterator&, routine_state*)>;
static const std::map<vtil::instruction_desc, fn_instruction_compiler_t> handler_table = {
	{
		ins::ldd,
		[](const vtil::il_iterator& instr, routine_state* state) {
			auto dest = instr->operands[0].reg();
			auto src = instr->operands[1].reg();
			auto offset = instr->operands[2].imm();

			// FIXME: Figure out how to determine if the offset is signed or not
			//
			state->cc.mov(state->get_reg(dest), x86::ptr(state->get_reg(src), offset.i64));
		},
	},
	{
		ins::str,
		[](const vtil::il_iterator& instr, routine_state* state) {
			auto base = instr->operands[0].reg();
			auto offset = instr->operands[1].imm();
			auto v = instr->operands[2];

			// FIXME: There is an issue here where it cannot deduce the size
			// of the move?
			//

			auto reg_base = state->get_reg(base);
			x86::Mem dest;
			switch (v.bit_count())
			{
			case 8:
				dest = x86::ptr_8(reg_base, offset.i64);
				break;
			case 16:
				dest = x86::ptr_16(reg_base, offset.i64);
				break;
			case 32:
				dest = x86::ptr_32(reg_base, offset.i64);
				break;
			case 64:
				dest = x86::ptr_64(reg_base, offset.i64);
				break;
			default:
				unreachable();
			}

			if (v.is_immediate())
			{
				state->cc.mov(dest, v.imm().i64);
			}
			else
			{
				state->cc.mov(dest, state->get_reg(v.reg()));
			}
		},
	},
	{
		ins::mov,
		[](const vtil::il_iterator& instr, routine_state* state) {
			auto dest = instr->operands[0].reg();
			auto src = instr->operands[1];

			if (src.is_immediate())
			{
				state->cc.mov(state->get_reg(dest), src.imm().i64);
			}
			else
			{
				state->cc.mov(state->get_reg(dest), state->get_reg(src.reg()));
			}
		},
	},
	{
		ins::sub,
		[](const vtil::il_iterator& instr, routine_state* state) {
			auto dest = instr->operands[0].reg();
			auto src = instr->operands[1];

			if (src.is_immediate())
			{
				x86::Gp tmp = state->reg_for_size(src);
				state->cc.mov(tmp, src.imm().i64);
				state->cc.sub(state->get_reg(dest), tmp);

				// AsmJit shits its pants when I use this, so we move to a temporary
				// instead. TODO: Investigate
				// state->cc.sub( state->get_reg( dest ), src.imm().i64 );
				//
			}
			else
			{
				state->cc.sub(state->get_reg(dest), state->get_reg(src.reg()));
			}
		},
	},
	{
		ins::add,
		[](const vtil::il_iterator& instr, routine_state* state) {
			auto lhs = instr->operands[0].reg();
			auto rhs = instr->operands[1];

			if (rhs.is_immediate())
			{
				x86::Gp tmp = state->reg_for_size(rhs);
				state->cc.mov(tmp, rhs.imm().i64);
				state->cc.add(state->get_reg(lhs), tmp);

				// See note on sub
				//
			}
			else
			{
				state->cc.add(state->get_reg(lhs), state->get_reg(rhs.reg()));
			}
		},
	},
	{
		ins::js,
		[](const vtil::il_iterator& it, routine_state* state) {
			auto cond = it->operands[0].reg();
			auto dst_1 = it->operands[1];
			auto dst_2 = it->operands[2];

			fassert(dst_1.is_immediate() && dst_2.is_immediate());

			// TODO: We should check if the block is compiled in order to avoid the
			// jump here, but I think the optimizer removes this?
			//
			state->cc.test(state->get_reg(cond), state->get_reg(cond));

			state->cc.jnz(state->get_label(dst_1.imm().u64));
			state->cc.jmp(state->get_label(dst_2.imm().u64));

			for (vtil::basic_block* destination : it.block->next)
			{
				if (!state->is_compiled.count(destination->entry_vip))
					compile(destination, state);
			}
		},
	},
	{
		ins::jmp,
		[](const vtil::il_iterator& it, routine_state* state) {
			vtil::debug::dump(*it);
			if (it->operands[0].is_register())
			{
				vtil::tracer tracer = {};
				std::vector<vtil::vip_t> destination_list;
				auto branch_info = vtil::optimizer::aux::analyze_branch(it.block, &tracer, { .pack = true });
				fassert(branch_info.is_jcc && !branch_info.is_vm_exit);
				using namespace vtil::logger;
				log<CON_YLW>("cc: %s\n", branch_info.cc.simplify().to_string());
				// TODO: handle jmp register
				for(const auto& dest : branch_info.destinations) {
					log<CON_YLW>("dest: %s\n", dest.simplify().to_string());
				}
				__debugbreak();
			}
			else
			{
				fassert(it->operands[0].is_immediate());

				auto dest = it.block->next[0]->entry_vip;

				state->cc.jmp(state->get_label(dest));

				if (!state->is_compiled.count(dest))
					compile(it.block->next[0], state);
			}
		},
	},
	{
		ins::vexit,
		[](const vtil::il_iterator& it, routine_state* state) {
			// TODO: Call out into handler
			//
			state->cc.ret();
		},
	},
	{
		ins::vxcall,
		[](const vtil::il_iterator& it, routine_state* state) {
			// TODO: This should be a call, but you need to create
			// a call, etc. for the register allocator
			// if ( it->operands[ 0 ].is_immediate() )
			// {
			//     state->cc.jmp( it->operands[ 0 ].imm().u64 );
			// }
			// else
			// {
			//     state->cc.jmp( state->get_reg( it->operands[ 0 ].reg() ) );
			// }
			//

			auto dest = it.block->next[0]->entry_vip;

			// Jump to next block.
			//
			state->cc.jmp(state->get_label(dest));

			if (!state->is_compiled.count(dest))
				compile(it.block->next[0], state);
		},
	},
	{
		ins::bshl,
		[](const vtil::il_iterator& it, routine_state* state) {
			auto dest = it->operands[0].reg();
			auto shift = it->operands[1];

			if (shift.is_immediate())
			{
				state->cc.shl(state->get_reg(dest), shift.imm().i64);
			}
			else
			{
				state->cc.shl(state->get_reg(dest), state->get_reg(shift.reg()));
			}
		},
	},
	{
		ins::bshr,
		[](const vtil::il_iterator& it, routine_state* state) {
			auto dest = it->operands[0].reg();
			auto shift = it->operands[1];

			if (shift.is_immediate())
			{
				state->cc.shr(state->get_reg(dest), shift.imm().i64);
			}
			else
			{
				state->cc.shr(state->get_reg(dest), state->get_reg(shift.reg()));
			}
		},
	},
	{
		ins::band,
		[](const vtil::il_iterator& it, routine_state* state) {
			auto dest = it->operands[0].reg();
			auto bit = it->operands[1];

			if (bit.is_immediate())
			{
				state->cc.and_(state->get_reg(dest), state->tmp_imm(bit));
			}
			else
			{
				state->cc.and_(state->get_reg(dest), state->get_reg(bit.reg()));
			}
		},
	},
	{
		ins::bor,
		[](const vtil::il_iterator& it, routine_state* state) {
			auto lhs = it->operands[0].reg();
			auto rhs = it->operands[1];

			if (rhs.is_immediate())
			{
				state->cc.or_(state->get_reg(lhs), rhs.imm().i64);
			}
			else
			{
				state->cc.or_(state->get_reg(lhs), state->get_reg(rhs.reg()));
			}
		},
	},
	{
		ins::bxor,
		[](const vtil::il_iterator& it, routine_state* state) {
			auto lhs = it->operands[0].reg();
			auto rhs = it->operands[1];

			if (rhs.is_immediate())
			{
				state->cc.xor_(state->get_reg(lhs), rhs.imm().i64);
			}
			else
			{
				state->cc.xor_(state->get_reg(lhs), state->get_reg(rhs.reg()));
			}
		},
	},
	{
		ins::bnot,
		[](const vtil::il_iterator& it, routine_state* state) {
			state->cc.not_(state->get_reg(it->operands[0].reg()));
		},
	},
	{
		ins::neg,
		[](const vtil::il_iterator& it, routine_state* state) {
			state->cc.neg(state->get_reg(it->operands[0].reg()));
		},
	},
	{
		ins::vemit,
		[](const vtil::il_iterator& it, routine_state* state) {
			auto data = it->operands[0].imm().u64;
			// TODO: Are we guarenteed that the registers used by these
			// embedded instructions are actually live at the point these are executed?
			//
			state->cc.embedUInt8((uint8_t)data);
		},
	},
#define MAP_CONDITIONAL(instrT, opcode, ropcode)                                    \
	{                                                                               \
		ins::instrT, [](const vtil::il_iterator& instr, routine_state* state) {     \
			vtil::logger::log("1_is_imm: %d\n", instr->operands[0].is_immediate()); \
			vtil::logger::log("2_is_imm: %d\n", instr->operands[1].is_immediate()); \
			vtil::logger::log("3_is_imm: %d\n", instr->operands[2].is_immediate()); \
			if (instr->operands[1].is_immediate())                                  \
			{                                                                       \
				x86::Gp tmp = state->reg_for_size(instr->operands[1]);              \
				state->cc.mov(tmp, instr->operands[1].imm().i64);                   \
				state->cc.cmp(state->get_reg(instr->operands[2].reg()), tmp);       \
				state->cc.ropcode(state->get_reg(instr->operands[0].reg()));        \
			}                                                                       \
			else                                                                    \
			{                                                                       \
				if (instr->operands[2].is_immediate())                              \
				{                                                                   \
					x86::Gp tmp = state->reg_for_size(instr->operands[2]);          \
					state->cc.mov(tmp, instr->operands[2].imm().i64);               \
					state->cc.cmp(state->get_reg(instr->operands[1].reg()), tmp);   \
				}                                                                   \
				else                                                                \
				{                                                                   \
					state->cc.cmp(state->get_reg(instr->operands[1].reg()),         \
						state->get_reg(instr->operands[2].reg()));                  \
				}                                                                   \
				state->cc.ropcode(state->get_reg(instr->operands[0].reg()));        \
			}                                                                       \
		},                                                                          \
	}
	MAP_CONDITIONAL(tg, setg, setle),
	MAP_CONDITIONAL(tge, setge, setl),
	MAP_CONDITIONAL(te, sete, setne),
	MAP_CONDITIONAL(tne, setne, sete),
	MAP_CONDITIONAL(tle, setle, setg),
	MAP_CONDITIONAL(tl, setl, setge),
	MAP_CONDITIONAL(tug, seta, setbe),
	MAP_CONDITIONAL(tuge, setae, setb),
	MAP_CONDITIONAL(tule, setbe, seta),
	MAP_CONDITIONAL(tul, setb, setae),
#undef MAP_CONDITIONAL
	{
		ins::ifs,
		[](const vtil::il_iterator& it, routine_state* state) {
			auto dest = it->operands[0].reg();
			auto cc = it->operands[1];
			auto res = it->operands[2];

			state->cc.xor_(state->get_reg(dest), state->get_reg(dest));
			// TODO: CC can be an immediate, how does that work?
			//
			state->cc.test(state->get_reg(cc.reg()), state->get_reg(cc.reg()));

			if (res.is_immediate())
			{
				x86::Gp tmp = state->reg_for_size(res);
				state->cc.mov(tmp, res.imm().i64);
				state->cc.cmovnz(state->get_reg(dest), tmp);
			}
			else
			{
				state->cc.cmovnz(state->get_reg(dest), state->get_reg(res.reg()));
			}
		},
	},
};

static void compile(vtil::basic_block* basic_block, routine_state* state)
{
	Label L_entry = state->get_label(basic_block->entry_vip);
	state->cc.bind(L_entry);
	state->is_compiled.insert(basic_block->entry_vip);

	for (auto it = basic_block->begin(); !it.is_end(); it++)
	{
		vtil::debug::dump(*it);
		auto handler = handler_table.find(*it->base);
		if (handler == handler_table.end())
		{
			vtil::logger::log("\n[!] ERROR: Unrecognized instruction '%s'\n\n", it->base->name);
			exit(1);
		}
		handler->second(it, state);
	}
}

class DemoErrorHandler : public ErrorHandler
{
public:
	void handleError(Error err, const char* message, BaseEmitter* origin) override
	{
		std::cerr << "AsmJit error: " << message << "\n";
	}
};

static args::Command command_compile(commands(), "compile", "Compile a .vtil file", [](args::Subparser& parser) {
	// Argument handling
	args::Positional<std::string> input(parser, "input", "Input .vtil file", args::Options::Required);
	args::Positional<std::string> output(parser, "output", "Output x86-64 file", args::Options::Required);
	parser.Parse();

	// Command implementation
	vtil::routine* rtn = vtil::load_routine(input.Get());

	vtil::basic_block* bb = rtn->find_block(0x13df89);
	vtil::instruction kurwa(&vtil::ins::str, vtil::REG_SP, 0, -0x13000000ull);
	bb->insert(bb->begin(), kurwa);

	JitRuntime rt;
	FileLogger logger(stdout);
	DemoErrorHandler errorHandler;
	CodeHolder code;

	code.init(rt.environment());
	code.setErrorHandler(&errorHandler);

	code.setLogger(&logger);
	x86::Compiler cc(&code);

	cc.addFunc(FuncSignatureT<void>());

	vtil::optimizer::stack_propagation_pass pass;
	pass(rtn);

	/*vtil::optimizer::branch_correction_pass pass;
	pass(rtn);*/

	routine_state state(cc);
	compile(rtn->entry_point, &state);

	cc.endFunc();
	cc.finalize();

	CodeBuffer& buffer = code.sectionById(0)->buffer();

	save_routine(rtn, output.Get());
});
