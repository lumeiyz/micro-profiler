#include <patcher/function_patch.h>

#include "assembler_intel.h"

#include <common/memory_protection.h>
#include <patcher/binary_translation.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		enum {
			jmp_size = sizeof(intel::jmp_rel_imm32),
		};
	}

	function_patch::function_patch(executable_memory_allocator &allocator_, void *fn_address, const_byte_range fn_body,
			void *instance, enter_hook_t *on_enter, exit_hook_t *on_exit)
		: _target_function(static_cast<byte *>(fn_address)),
			_chunk_length(calculate_function_length(fn_body, jmp_size))
	{
		const_byte_range source_chunk(fn_body.begin(), _chunk_length);
		scoped_unprotect su(range<byte>(_target_function, _chunk_length));
		const size_t size0 = c_thunk_size + _chunk_length + jmp_size;
		const size_t size = (size0 + 0x0F) & ~0x0F;

		_memory = allocator_.allocate(size);

		byte *thunk = static_cast<byte *>(_memory.get());

		// initialize thunk
		initialize_hooks(thunk, thunk + c_thunk_size, _target_function, instance, on_enter, on_exit);
		move_function(thunk + c_thunk_size, _target_function, source_chunk);
		reinterpret_cast<intel::jmp_rel_imm32 *>(thunk + c_thunk_size + _chunk_length)
			->init(_target_function + _chunk_length);
		memset(thunk + size0, 0xCC, size - size0);

		// place hooking jump to original body
		intel::jmp_rel_imm32 &jmp_original = *(intel::jmp_rel_imm32 *)(_target_function);
			
		memcpy(_saved, source_chunk.begin(), source_chunk.length());
		jmp_original.init(thunk);
		memset(_target_function + jmp_size, 0xCC, _chunk_length - jmp_size);
	}

	function_patch::~function_patch()
	{
		scoped_unprotect su(range<byte>(_target_function, _chunk_length));

		memcpy(_target_function, _saved, _chunk_length);
	}
}
