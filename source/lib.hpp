#pragma once

#include <exec/async_scope.hpp>
#include <exec/task.hpp>

#include "io_context.hpp"
#include "socket.hpp"

namespace sawd
{

auto echo(std::unique_ptr<tcp::socket> sock) -> exec::task<void>;
auto echo_server(exec::async_scope& scope,
                 io_context::scheduler& sched,
                 const tcp::endpoint& ep) -> exec::task<void>;

}  // namespace sawd
