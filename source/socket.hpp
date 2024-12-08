#pragma once
#include <memory>
#include <span>
#include <string>

#include <exec/create.hpp>

#include "io_context.hpp"
#include "uv.h"

namespace sawd::tcp
{

struct endpoint
{
  std::string ip;
  int port;
};

struct socket
{
  socket(io_context::scheduler& sched)
      : sched {sched}
  {
    uv_tcp_init(sched.ctx.loop(), &handle);
  }

  virtual ~socket()
  {
    uv_close(reinterpret_cast<uv_handle_t*>(&handle), nullptr);
  }

  auto listen(const endpoint& ep) -> decltype(auto)
  {
    return exec::create<stdexec::set_value_t(std::unique_ptr<socket>)>(
        [this, ep]<class Context>(Context& ctx) noexcept
        {
          handle.data = &ctx;
          sockaddr_in addr {};
          uv_ip4_addr(ep.ip.c_str(), ep.port, &addr);
          uv_tcp_bind(&handle, (const struct sockaddr*)&addr, 0);
          uv_listen(
              reinterpret_cast<uv_stream_t*>(&handle),
              4096,
              [](uv_stream_t* h, int status)
              {
                // TODO: handle errors
                // if (status < 0) {
                //   stdexec::set_error(
                //       static_cast<R&&>(thiz->r),
                //       std::error_code {status, std::system_category()});
                //   return;
                // }

                auto ctx = static_cast<Context*>(h->data);
                auto sock = std::make_unique<socket>(*std::get<0>(ctx->args));
                uv_accept(h, reinterpret_cast<uv_stream_t*>(&sock->handle));
                stdexec::set_value(std::move(ctx->receiver), std::move(sock));
              });
        },
        &sched);
  }

  auto connect(const endpoint& ep) -> decltype(auto)
  {
    return exec::create<stdexec::set_value_t()>(
        [this, ep]<class Context>(Context& ctx) noexcept
        {
          handle.data = &ctx;
          struct sockaddr_in dest;
          uv_ip4_addr(ep.ip.c_str(), ep.port, &dest);

          uv_tcp_connect(&std::get<0>(ctx.args),
                         &handle,
                         (const struct sockaddr*)&dest,
                         [](uv_connect_t* req, int status)
                         {
                           // TODO: handle errors
                           auto ctx = static_cast<Context*>(req->handle->data);
                           stdexec::set_value(std::move(ctx->receiver));
                         });
        },
        uv_connect_t {});
  }

  auto read() -> decltype(auto)
  {
    return exec::create<stdexec::set_value_t(std::unique_ptr<char[]>, size_t),
                        stdexec::set_stopped_t()>(
        [this]<class Context>(Context& ctx) noexcept
        {
          handle.data = &ctx;
          uv_read_start(
              reinterpret_cast<uv_stream_t*>(&handle),
              [](uv_handle_t* h, size_t len, uv_buf_t* uv_buf)
              {
                uv_buf->base = new char[len];
                uv_buf->len = len;
              },
              [](uv_stream_t* h, ssize_t nread, const uv_buf_t* uv_buf)
              {
                auto ctx = static_cast<Context*>(h->data);
                auto buf = std::unique_ptr<char[]>(uv_buf->base);
                if (nread == UV_EOF) {
                  stdexec::set_stopped(std::move(ctx->receiver));
                  return;
                }
                stdexec::set_value(std::move(ctx->receiver),
                                   std::move(buf),
                                   static_cast<size_t>(nread));
              });
        });
  }

  auto write(std::span<char> buf) -> decltype(auto)
  {
    return exec::create<stdexec::set_value_t()>(
        [this, buf]<class Context>(Context& ctx) noexcept
        {
          handle.data = &ctx;
          uv_buf_t uv_buf = uv_buf_init(buf.data(), buf.size());
          uv_write(&std::get<0>(ctx.args),
                   reinterpret_cast<uv_stream_t*>(&handle),
                   &uv_buf,
                   1,
                   [](uv_write_t* req, int status)
                   {
                     // TODO: handle errors
                     auto ctx = static_cast<Context*>(req->handle->data);
                     stdexec::set_value(std::move(ctx->receiver));
                   });
        },
        uv_write_t {});
  }

  io_context::scheduler& sched;
  uv_tcp_t handle;
};

using accept_sender =
    decltype(std::declval<socket>().listen(std::declval<endpoint>()));
using connect_sender =
    decltype(std::declval<socket>().connect(std::declval<endpoint>()));
using read_sender = decltype(std::declval<socket>().read());
using write_sender =
    decltype(std::declval<socket>().write(std::declval<std::span<char>>()));

}  // namespace sawd::tcp
