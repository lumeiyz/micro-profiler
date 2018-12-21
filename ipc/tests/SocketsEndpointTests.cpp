#include <ipc/endpoint_sockets.h>

#include "helpers_sockets.h"
#include "mocks.h"

#include <mt/event.h>
#include <stdexcept>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			begin_test_suite( SocketsEndpointTests )
				test( CreatingEndpointReturnsNonNullObjects )
				{
					// INIT / ACT
					shared_ptr<endpoint> e1 = sockets::create_endpoint();
					shared_ptr<endpoint> e2 = sockets::create_endpoint();

					// ASSERT
					assert_not_null(e1);
					assert_not_null(e2);
					assert_not_equal(e1, e2);
				}


				test( CreatingPassiveSessionOpensExpectedPort )
				{
					// INIT
					shared_ptr<endpoint> e = sockets::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);

					// ACT
					shared_ptr<void> s1 = e->create_passive("6101", f);

					// ASSERT
					assert_is_true(is_local_port_open(6101));
					assert_is_false(is_local_port_open(6103));

					// ACT
					shared_ptr<void> s2 = e->create_passive("6103", f);

					// ASSERT
					assert_is_true(is_local_port_open(6101));
					assert_is_true(is_local_port_open(6103));
				}


				test( CreatingPassiveSessionAtTheSamePortThrowsException )
				{
					// INIT
					shared_ptr<endpoint> e = sockets::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);
					shared_ptr<void> s1 = e->create_passive("6101", f);

					// ACT / ASSERT
					assert_throws(e->create_passive("6101", f), runtime_error);
				}


				test( CreatingStreamConstructsSession )
				{
					// INIT
					int times = 1;
					mt::event ready;
					shared_ptr<endpoint> e = sockets::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);
					shared_ptr<void> h = e->create_passive("6101", f);

					f->session_opened = [&] (const shared_ptr<void> &) {
						if (!--times)
							ready.set();
					};

					// ACT
					sender s1(6101);
					ready.wait();

					// ASSERT
					assert_equal(1u, f->sessions.size());
					assert_is_false(f->sessions[0].unique());

					// INIT
					times = 2;

					// ACT
					sender s2(6101);
					sender s3(6101);
					ready.wait();

					// ASSERT
					assert_equal(3u, f->sessions.size());
					assert_is_false(f->sessions[1].unique());
					assert_is_false(f->sessions[2].unique());
				}


				test( ReleasingStreamDisconnectsSession )
				{
					// INIT
					mt::event ready;
					shared_ptr<endpoint> e = sockets::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);
					shared_ptr<void> h = e->create_passive("6101", f);

					f->session_opened = [&] (const shared_ptr<mocks::session> &s) {
						s->disconnected = [&] {
							ready.set();
						};
					};

					// ACT
					sender(6101);
					ready.wait();

					// ASSERT
					mt::this_thread::sleep_for(mt::milliseconds(50));
					assert_equal(1u, f->sessions.size());
					assert_is_true(f->sessions[0].unique());

					// ACT
					sender(6101);
					ready.wait();

					// ASSERT
					mt::this_thread::sleep_for(mt::milliseconds(50));
					assert_equal(2u, f->sessions.size());
					assert_is_true(f->sessions[1].unique());
				}


				test( MessagesSentAreReceivedBySession )
				{
					// INIT
					shared_ptr<endpoint> e = sockets::create_endpoint();
					shared_ptr<mocks::session_factory> f(new mocks::session_factory);
					shared_ptr<void> s = e->create_passive("6101", f);
					sender stream(6101);
					byte data1[] = "if you�re going to try, go all the way.";
//					byte data2[] = "otherwise, don�t even start.";
	//				byte data3[] = "this could mean losing girlfriends, wives, relatives, jobs and maybe your mind.";

					// ACT
					stream(data1, sizeof(data1));

					// ASSERT
				}
			end_test_suite
		}
	}
}
