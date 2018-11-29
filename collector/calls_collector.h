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

#pragma once

#include "calls_collector_thread.h"
#include "system.h"

#include <memory>
#include <patcher/platform.h>
#include <vector>
#include <wpl/mt/thread.h>

namespace micro_profiler
{
	struct calls_collector_i
	{
		struct acceptor;

		virtual ~calls_collector_i() {	}
		virtual void read_collected(acceptor &a) = 0;
		virtual timestamp_t profiler_latency() const throw() = 0;
	};

	struct calls_collector_i::acceptor
	{
		virtual void accept_calls(unsigned int threadid, const call_record *calls, size_t count) = 0;
	};


	class calls_collector : public calls_collector_i
	{
	public:
		calls_collector(size_t trace_limit);

		virtual void read_collected(acceptor &a);

		static void CC_(fastcall) on_enter(calls_collector *instance, const void **stack_ptr,
			timestamp_t timestamp, const void *callee) _CC(fastcall);
		static const void *CC_(fastcall) on_exit(calls_collector *instance, const void **stack_ptr,
			timestamp_t timestamp) _CC(fastcall);

		virtual timestamp_t profiler_latency() const throw();

	private:
		typedef std::vector< std::pair< unsigned int, std::shared_ptr<calls_collector_thread> > > call_traces_t;

	private:
		calls_collector_thread &get_current_thread_trace();
		calls_collector_thread &get_current_thread_trace_guaranteed();
		calls_collector_thread &construct_thread_trace();

	private:
		wpl::mt::tls<calls_collector_thread> _trace_pointers_tls;
		const size_t _trace_limit;
		timestamp_t _profiler_latency;
		call_traces_t _call_traces;
		mutex _thread_blocks_mtx;
	};
}