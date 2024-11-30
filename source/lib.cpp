#include "lib.hpp"
#include <exec/create.hpp>
#include <exec/finally.hpp>
#include <fmt/format.h>
#include "io_context.hpp"
#include <exec/async_scope.hpp>
#include <exec/start_now.hpp>
#include <exec/task.hpp>

namespace sawd
{

auto test() -> void
{
  io_context ctx;
  exec::async_scope scope;
  auto snd = ctx.get_scheduler().schedule()
    | stdexec::then([] { return 43; })
    | stdexec::then([](int x) { return x * x; })
    | stdexec::then([](int x) { fmt::println("{}", x); });

  scope.spawn(std::move(snd));
  // scope.spawn([&]() -> exec::task<void> {
  //   co_await ctx.get_scheduler().schedule();
  //   fmt::println("{}", 1234);
  // }());
  ctx.run();
  stdexec::sync_wait(scope.on_empty());
  // using __receiver_t = stdexec::__t<exec::__start_now_::__receiver<stdexec::__root_env>>;
  // using __sender_t = io_context::scheduler::schedule_sender;
  // static_assert(stdexec::__connect::__connectable_with_tag_invoke<__sender_t, __receiver_t>);
  //   static_assert(
  //   requires(__sender_t&& __sndr, __receiver_t&& __rcvr) {
  // stdexec::connect(static_cast<__sender_t&&>(__sndr), static_cast<__receiver_t&&>(__rcvr));
  //     });
  // static_assert(stdexec::__nofail_sender<io_context::scheduler::schedule_sender&&, stdexec::__root_env>);
  // static_assert(stdexec::__nofail_sender<decltype(snd), stdexec::__root_env>);
  // exec::start_now(scope, ctx.get_scheduler().schedule());
}

}  // namespace sawd
