#pragma once

#include <common/noncopyable.h>
#include <common/protocol.h>
#include <common/types.h>
#include <collector/calls_collector.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			typedef function_statistics_detailed_t<unsigned int> function_statistics_detailed;
			typedef statistics_map_detailed_t<unsigned int> statistics_map_detailed;

			class frontend_state : noncopyable, public std::enable_shared_from_this<frontend_state>
			{
			public:
				explicit frontend_state(const std::shared_ptr<void> &ownee = std::shared_ptr<void>());

				channel_t create();

			public:
				std::function<void ()> constructed;
				std::function<void ()> destroyed;

				std::function<void (const initialization_data &id)> initialized;
				std::function<void (const loaded_modules &m)> modules_loaded;
				std::function<void (const statistics_map_detailed &u)> updated;
				std::function<void (const unloaded_modules &m)> modules_unloaded;

			private:
				std::shared_ptr<void> _ownee;
			};


			class Tracer : public calls_collector_i
			{
			public:
				explicit Tracer(timestamp_t latency = 0);
				virtual ~Tracer() throw();

				template <size_t size>
				void Add(mt::thread::id threadid, call_record (&array_ptr)[size]);

				virtual void read_collected(acceptor &a);
				virtual timestamp_t profiler_latency() const throw();

			private:
				typedef std::unordered_map< mt::thread::id, std::vector<call_record> > TracesMap;

				timestamp_t _latency;
				TracesMap _traces;
				mt::mutex _mutex;
			};



			template <typename ArchiveT>
			inline void serialize(ArchiveT &a, frontend_state &state)
			{
				commands c;
				initialization_data id;
				loaded_modules lm;
				statistics_map_detailed u;
				unloaded_modules um;

				a(c);
				switch (c)
				{
				case init:
					if (state.initialized)
						a(id), state.initialized(id);
					break;

				case modules_loaded:
					if (state.modules_loaded)
						a(lm), state.modules_loaded(lm);
					break;

				case update_statistics:
					if (state.updated)
						a(u), state.updated(u);
					break;

				case modules_unloaded:
					if (state.modules_unloaded)
						a(um), state.modules_unloaded(um);
					break;
				}
			}


			inline frontend_state::frontend_state(const std::shared_ptr<void> &ownee)
				: _ownee(ownee)
			{	}


			template <size_t size>
			inline void Tracer::Add(mt::thread::id threadid, call_record (&trace_chunk)[size])
			{
				mt::lock_guard<mt::mutex> l(_mutex);
				_traces[threadid].insert(_traces[threadid].end(), trace_chunk, trace_chunk + size);
			}
		}
	}
}
