#pragma once
#include <span>
#include <memory>
#include <string>

#include "io_context.hpp"
#include "uv.h"

namespace sawd::tcp
{

struct endpoint
{
  std::string ip;
  int port;
};

struct socket;

template<typename R>
struct connect_operation
{
  void start() noexcept
  {
    handle->data = this;
    struct sockaddr_in dest;
    uv_ip4_addr(ep.ip.c_str(), ep.port, &dest);

    uv_tcp_connect(&req,
                   handle,
                   (const struct sockaddr*)&dest,
                   [](uv_connect_t* req, int status)
                   {
                     // TODO: handle errors
                     auto thiz =
                         static_cast<connect_operation*>(req->handle->data);
                     stdexec::set_value(static_cast<R&&>(thiz->r));
                   });
  }
  R r;
  uv_tcp_t* handle;
  io_context::scheduler& sched;
  endpoint ep;
  uv_connect_t req;
};

template<typename R>
struct accept_operation
{
  void start() noexcept
  {
    handle->data = this;
    sockaddr_in addr {};
    uv_ip4_addr(ep.ip.c_str(), ep.port, &addr);
    uv_tcp_bind(handle, (const struct sockaddr*)&addr, 0);
    uv_listen(reinterpret_cast<uv_stream_t*>(handle),
              4096,
              [](uv_stream_t* h, int status)
              {
                auto thiz = static_cast<accept_operation*>(h->data);
                // TODO: handle errors
                // if (status < 0) {
                //   stdexec::set_error(
                //       static_cast<R&&>(thiz->r),
                //       std::error_code {status, std::system_category()});
                //   return;
                // }

                auto sock = std::make_unique<socket>(thiz->sched);
                uv_accept(reinterpret_cast<uv_stream_t*>(thiz->handle),
                          reinterpret_cast<uv_stream_t*>(&sock->handle));
                stdexec::set_value(static_cast<R&&>(thiz->r), std::move(sock));
              });
  }
  R r;
  uv_tcp_t* handle;
  io_context::scheduler& sched;
  endpoint ep;
};

template<template<typename> typename T, typename... Sig>
struct socket_sender
{
  using sender_concept = stdexec::sender_t;
  using completion_signatures = stdexec::completion_signatures<Sig...>;

  template<typename R>
  friend auto tag_invoke(stdexec::connect_t,
                         const socket_sender& self,
                         R r) -> T<R>
  {
    return T<R> {static_cast<R&&>(r), self.handle, self.sched, self.ep};
  }

  uv_tcp_t* handle;
  io_context::scheduler& sched;
  endpoint ep;
};
using accept_sender =
    socket_sender<accept_operation,
                  stdexec::set_value_t(std::unique_ptr<socket>)>;
using connect_sender = socket_sender<connect_operation, stdexec::set_value_t()>;

struct write_sender
{
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t()>;

  template<typename R>
  struct operation
  {
    void start() noexcept
    {
      handle->data = this;
      uv_buf_t uv_buf = uv_buf_init(buf.data(), buf.size());
      uv_write(&req,
               reinterpret_cast<uv_stream_t*>(handle),
               &uv_buf,
               1,
               [](uv_write_t* req, int status)
               {
                 // TODO: handle errors
                 auto thiz = static_cast<operation*>(req->handle->data);
                 stdexec::set_value(static_cast<R&&>(thiz->r));
               });
    };

    R r;
    uv_tcp_t* handle;
    io_context::scheduler& sched;
    std::span<char> buf;
    uv_write_t req;
  };

  template<typename R>
  friend auto tag_invoke(stdexec::connect_t,
                         const write_sender& self,
                         R r) -> operation<R>
  {
    return operation<R> {
        static_cast<R&&>(r), self.handle, self.sched, self.buf};
  }
  uv_tcp_t* handle;
  io_context::scheduler& sched;
  std::span<char> buf;
};

struct read_sender
{
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(
                                         std::unique_ptr<char[]>, size_t),
                                     stdexec::set_stopped_t()>;

  template<typename R>
  struct operation
  {
    void start() noexcept
    {
      handle->data = this;
      uv_read_start(
          reinterpret_cast<uv_stream_t*>(handle),
          [](uv_handle_t* h, size_t len, uv_buf_t* uv_buf)
          {
            uv_buf->base = new char[len];
            uv_buf->len = len;
          },
          [](uv_stream_t* h, ssize_t nread, const uv_buf_t* uv_buf)
          {
            auto thiz = static_cast<operation*>(h->data);
            auto buf = std::unique_ptr<char[]>(uv_buf->base);
            if (nread == UV_EOF) {
              stdexec::set_stopped(static_cast<R&&>(thiz->r));
              return;
            }
            stdexec::set_value(static_cast<R&&>(thiz->r),
                               std::move(buf),
                               static_cast<size_t>(nread));
          });
    };

    R r;
    uv_tcp_t* handle;
    io_context::scheduler& sched;
  };

  template<typename R>
  friend auto tag_invoke(stdexec::connect_t,
                         const read_sender& self,
                         R r) -> operation<R>
  {
    return operation<R> {static_cast<R&&>(r), self.handle, self.sched};
  }
  uv_tcp_t* handle;
  io_context::scheduler& sched;
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

  auto listen(const endpoint& ep) -> accept_sender
  {
    return accept_sender {&handle, sched, ep};
  }

  auto connect(const endpoint& ep) -> connect_sender
  {
    return connect_sender {&handle, sched, ep};
  }

  auto read() -> read_sender { return read_sender {&handle, sched}; }

  auto write(std::span<char> buf) -> write_sender
  {
    return write_sender {&handle, sched, buf};
  }

  io_context::scheduler& sched;
  uv_tcp_t handle;
};

}  // namespace sawd::tcp
