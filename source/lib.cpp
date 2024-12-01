#include "lib.hpp"

#include <exec/async_scope.hpp>
#include <exec/create.hpp>
#include <exec/finally.hpp>
#include <exec/task.hpp>
#include <fmt/format.h>

#include "io_context.hpp"
#include "socket.hpp"

namespace sawd
{

auto echo(std::unique_ptr<tcp::socket> sock) -> exec::task<void>
{
  auto read_sender = sock->read();
  for (;;) {
    auto [buf, nread] = co_await read_sender;
    auto data = std::string {buf.get(), nread};
    fmt::print("{}: read {}", (void*)sock.get(), data);
    co_await sock->write({buf.get(), nread});
    fmt::print("{}: write {}", (void*)sock.get(), data);
  }
}

auto echo_server(exec::async_scope& scope,
                 io_context::scheduler& sched,
                 const tcp::endpoint& ep) -> exec::task<void>
{
  tcp::socket sock(sched);
  auto acceptor = sock.listen(ep);
  fmt::println("listening on port 1234");
  for (;;) {
    auto peer = co_await acceptor;
    fmt::println("accepted connection {}", (void*)peer.get());
    scope.spawn(echo(std::move(peer)));
  }
}

}  // namespace sawd
