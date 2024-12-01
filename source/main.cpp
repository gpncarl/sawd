#include <exec/async_scope.hpp>

#include "io_context.hpp"
#include "lib.hpp"
#include "socket.hpp"

auto main() -> int
{
  sawd::io_context ctx;
  sawd::io_context::scheduler sched = ctx.get_scheduler();
  exec::async_scope scope;
  sawd::tcp::endpoint ep {"0.0.0.0", 1234};
  scope.spawn(echo_server(scope, sched, ep));
  ctx.run();
  stdexec::sync_wait(scope.on_empty());
  return 0;
}
