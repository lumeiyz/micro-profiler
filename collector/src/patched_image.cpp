//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <collector/patched_image.h>
#include <collector/calls_collector.h>
#include <collector/dynamic_hooking.h>
#include <collector/binary_image.h>

#include "assembler_intel.h"

#include <common/module.h>
#include <common/symbol_resolver.h>
#include <iostream>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		void CC_(fastcall) profile_enter(void *instance, const void *callee, timestamp_t timestamp, void **return_address_ptr) _CC(fastcall)
		{	static_cast<calls_collector *>(instance)->profile_enter(callee, timestamp, return_address_ptr);	}

		void *CC_(fastcall) profile_exit(void *instance, timestamp_t timestamp) _CC(fastcall)
		{	return static_cast<calls_collector *>(instance)->profile_exit(timestamp);	}
	}

	class executable_memory : wpl::noncopyable
	{
	public:
		~executable_memory()
		{	::VirtualFree(_start, 0, MEM_RELEASE); }

		static byte *allocate(shared_ptr<executable_memory> &em, unsigned size)
		{
			byte *block = 0;

			if (!em || !(block = em->allocate(size)))
				em.reset(new executable_memory);
			if (!block)
				block = em->allocate(size);
			return block;
		}

	private:
		executable_memory(unsigned max_size = 0x00010000)
			: _start((byte *)::VirtualAlloc(0, max_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)),
				_next(_start), _end(_start + max_size)
		{
			if (!_start)
				throw bad_alloc();
		}

		byte *allocate(unsigned size)
		{
			if (size <= _end - _next)
			{
				byte *block = (byte *)_next;
				_next += size;
				return block;
			}
			return 0;
		}

	private:
		byte * const _start, *_next, * const _end;
	};

	class scoped_unprotect : wpl::noncopyable
	{
	public:
		scoped_unprotect(void *address, unsigned size)
		{
			if (!::VirtualProtect(address, size, PAGE_EXECUTE_WRITECOPY, &_access))
				throw runtime_error("Cannot change protection mode!");
		}

		~scoped_unprotect()
		{
			DWORD dummy;
			::VirtualProtect(_address, _size, PAGE_EXECUTE/*_access*/, &dummy);
		}

	private:
		void *_address;
		unsigned _size;
		DWORD _access;
	};

	class patched_image::patch : wpl::noncopyable
	{
	public:
		patch(shared_ptr<executable_memory> &em, const function_body &fn)
			: _location(static_cast<byte *>(fn.effective_address()))
		{
			scoped_unprotect su(_location, fn.size());

			_size = c_thunk_size + fn.size();

			if (_size & 0x0F)
				_size &= ~0xF, _size += 0x10;

			_thunk = executable_memory::allocate(em, _size);
			_em = em;

			// initialize thunk
			initialize_hooks(_thunk, _thunk + c_thunk_size, _location, micro_profiler::calls_collector::instance(),
				&profile_enter, &profile_exit);
			fn.copy_relocate_to(_thunk + c_thunk_size);

			// place hooking jump to original body
			intel::jmp_rel_imm32 &jmp_original = *(intel::jmp_rel_imm32 *)(_location);
			
			memcpy(_saved, _location, sizeof(_saved));
			jmp_original.init(_thunk);
			::FlushInstructionCache(::GetCurrentProcess(), _location, fn.size());
		}

		~patch()
		{
			scoped_unprotect su(_location, sizeof(_saved));

			memcpy(_location, _saved, sizeof(_saved));
		}

	private:
		shared_ptr<executable_memory> _em;
		byte * const _location;
		unsigned _size;
		byte *_thunk;
		byte _saved[sizeof(intel::jmp_rel_imm32)];
	};

	void patched_image::patch_image(void *in_image_address)
	{
		std::shared_ptr<binary_image> image = load_image_at((void *)get_module_info(in_image_address).load_address);
		shared_ptr<executable_memory> em;
		int n = 0;

		image->enumerate_functions([this, &em, &n] (const function_body &fn) {
			try
			{
				if (fn.size() >= 5)
				{
					_patches.push_back(make_shared<patch>(em, fn));
					++n;
				}
				else
				{
					cout << fn.name() << " - the function is too short!" << endl;
				}
			}
			catch (const exception &e)
			{
				cout << fn.name() << " - " << e.what() << endl;
			}
		});
	}
}
