#include <collector/binary_translation.h>

#include <test-helpers/helpers.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( X86BinaryImageTests )
			test( RelativeOutsideJumpsAreTranslatedBasedOnTheirTargetAddress )
			{
				// INIT
				byte instructions1[] = {
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0xE9, 0xF0, 0xFF, 0xFF, 0xFF, 0xE9, 0x00, 0x00, 0x10, 0x00,
				};

				// ACT
				move_function(instructions1 + 1, instructions1 + 48, instructions1 + 48, 10);

				// ASSERT
				byte reference1[] = {
					0xE9, 0x1F, 0x00, 0x00, 0x00, 0xE9, 0x2F, 0x00, 0x10, 0x00,
				};

				assert_is_true(equal(reference1, array_end(reference1), instructions1 + 1));

				// ACT
				move_function(instructions1 + 17, instructions1 + 48, instructions1 + 48, 10);

				// ASSERT
				byte reference2[] = {
					0xE9, 0x0F, 0x00, 0x00, 0x00, 0xE9, 0x1F, 0x00, 0x10, 0x00,
				};

				assert_is_true(equal(reference2, array_end(reference2), instructions1 + 17));

				// INIT
				byte instructions2[0x1000] = {
					0xE9, 0x21, 0x10, 0x00, 0x10, 0xE9, 0x00, 0xFF, 0xFF, 0xFF, 0xE9, 0xFF, 0x00, 0x00, 0x00,
				};
		
				// ACT
				move_function(instructions2 + 0x0F12, instructions2, instructions2, 15);

				// ASSERT
				byte reference3[] = {
					0xE9, 0x0F, 0x01, 0x00, 0x10, 0xE9, 0xEE, 0xEF, 0xFF, 0xFF, 0xE9, 0xED, 0xF1, 0xFF, 0xFF,
				};

				assert_is_true(equal(reference3, array_end(reference3), instructions2 + 0x0F12));
			}


			test( RelativeOutsideCallsAreTranslatedBasedOnTheirTargetAddress )
			{
				// INIT
				byte instructions1[] = {
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0xE8, 0xF0, 0xFF, 0xFF, 0xFF, 0xE8, 0x00, 0x00, 0x10, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00,
				};

				// ACT
				move_function(instructions1 + 1, instructions1 + 48, instructions1 + 48, 15);

				// ASSERT
				byte reference1[] = {
					0xE8, 0x1F, 0x00, 0x00, 0x00, 0xE8, 0x2F, 0x00, 0x10, 0x00, 0xE8, 0x2F, 0x00, 0x00, 0x00,
				};

				assert_is_true(equal(reference1, array_end(reference1), instructions1 + 1));

				// ACT
				move_function(instructions1 + 17, instructions1 + 48, instructions1 + 48, 15);

				// ASSERT
				byte reference2[] = {
					0xE8, 0x0F, 0x00, 0x00, 0x00, 0xE8, 0x1F, 0x00, 0x10, 0x00, 0xE8, 0x1F, 0x00, 0x00, 0x00,
				};

				assert_is_true(equal(reference2, array_end(reference2), instructions1 + 17));

				// INIT
				byte instructions2[0x1000] = {
					0xE8, 0x21, 0x10, 0x00, 0x10, 0xE8, 0x00, 0xFF, 0xFF, 0xFF, 0xE8, 0xFF, 0x00, 0x00, 0x00,
					0xE8, 0x00, 0x00, 0x00, 0x00,
				};
		
				// ACT
				move_function(instructions2 + 0x0F11, instructions2, instructions2, 20);

				// ASSERT
				byte reference3[] = {
					0xE8, 0x10, 0x01, 0x00, 0x10, 0xE8, 0xEF, 0xEF, 0xFF, 0xFF, 0xE8, 0xEE, 0xF1, 0xFF, 0xFF,
					0xE8, 0xEF, 0xF0, 0xFF, 0xFF,
				};

				assert_is_true(equal(reference3, array_end(reference3), instructions2 + 0x0F11));
			}


			test( RelativeInnerJumpsAreNotTranslatedWhenMoved )
			{
				// INIT
				byte instructions[0x100] = {
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0xE9, 0x05, 0x00, 0x00, 0x00, 0xE9, 0x00, 0x00, 0x00, 0x00, 0xE9, 0xF1, 0xFF, 0xFF, 0xFF,
					0xE9, 0xF6, 0xFF, 0xFF, 0xFF,
				};

				// ACT
				move_function(instructions, instructions + 0xE1, instructions + 0x20, 20);
				move_function(instructions + 0x40, instructions + 0x23, instructions + 0x20, 20);

				// ASSERT
				byte reference[] = {
					0xE9, 0x05, 0x00, 0x00, 0x00, 0xE9, 0x00, 0x00, 0x00, 0x00, 0xE9, 0xF1, 0xFF, 0xFF, 0xFF,
					0xE9, 0xF6, 0xFF, 0xFF, 0xFF
				};

				assert_is_true(equal(reference, array_end(reference), instructions));
				assert_is_true(equal(reference, array_end(reference), instructions + 0x40));
			}


			test( RelativeInnerCallsAreNotTranslatedWhenMoved )
			{
				// INIT
				byte instructions[0x100] = {
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0xE8, 0x05, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0xE8, 0xF6, 0xFF, 0xFF, 0xFF,
					0xE8, 0xF6, 0xFF, 0xFF, 0xFF,
				};

				// ACT
				move_function(instructions, instructions + 0x01, instructions + 0x20, 20);
				move_function(instructions + 0x40, instructions + 0x23, instructions + 0x20, 20);

				// ASSERT
				byte reference[] = {
					0xE8, 0x05, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0xE8, 0xF6, 0xFF, 0xFF, 0xFF,
					0xE8, 0xF6, 0xFF, 0xFF, 0xFF,
				};

				assert_is_true(equal(reference, array_end(reference), instructions));
				assert_is_true(equal(reference, array_end(reference), instructions + 0x40));
			}


			test( InstructionMixWithExternalJumpsAndCallsIsTranslatedToTargetAddress )
			{
				// INIT
				byte instructions[0x400] = {
					0xE8, 0xD4, 0x54, 0x02, 0x00,			// call memset
					0x83, 0xC4, 0x0C,							// add esp, 0Ch
					0x6A, 0x0F,									// push 0Fh
					0x8D, 0x8D, 0xB4, 0xEF, 0xFF, 0xFF,	// lea ecx, [instructions2]
					0x51,											// push ecx
					0x8D, 0x95, 0xC5, 0xFE, 0xFF, 0xFF,	// lea edx, [ebp-13Bh]
					0x52,											// push edx
					0xE8, 0x8C, 0x98, 0x01, 0x00,			// call micro_profiler::move_function
					0x83, 0xC4, 0x0C,							// add esp, 0Ch
					0xE9, 0x12, 0x34, 0x56, 0x78,			// jmp somewhere...
				};
				byte *translated = instructions + 0x0123;

				// ACT
				move_function(translated, instructions, instructions, 0x0025);

				// ASSERT
				byte reference1[] = {
					0xE8, 0xB1, 0x53, 0x02, 0x00,
					0x83, 0xC4, 0x0C,
					0x6A, 0x0F,
					0x8D, 0x8D, 0xB4, 0xEF, 0xFF, 0xFF,
					0x51,
					0x8D, 0x95, 0xC5, 0xFE, 0xFF, 0xFF,
					0x52,
					0xE8, 0x69, 0x97, 0x01, 0x00,
					0x83, 0xC4, 0x0C,
					0xE9, 0xEF, 0x32, 0x56, 0x78,
				};

				assert_is_true(equal(reference1, array_end(reference1), translated));

				// ACT
				move_function(translated, instructions + 7, instructions, 0x0025);

				// ASSERT
				byte reference2[] = {
					0xE8, 0xB8, 0x53, 0x02, 0x00,
					0x83, 0xC4, 0x0C,
					0x6A, 0x0F,
					0x8D, 0x8D, 0xB4, 0xEF, 0xFF, 0xFF,
					0x51,
					0x8D, 0x95, 0xC5, 0xFE, 0xFF, 0xFF,
					0x52,
					0xE8, 0x70, 0x97, 0x01, 0x00,
					0x83, 0xC4, 0x0C,
					0xE9, 0xF6, 0x32, 0x56, 0x78,
				};

				assert_is_true(equal(reference2, array_end(reference2), translated));
			}

		end_test_suite
	}
}
