#pragma once
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

    uv_tcp_connect(
        &req,
        handle,
        (const struct sockaddr*)&dest,
        [](uv_connect_t* req, int status)
        {
        // TODO: handle errors
        auto thiz = static_cast<connect_operation*>(req->handle->data);
        stdexec::set_value(static_cast<R&&>(thiz->r));
        });
  }
  uv_tcp_t* handle;
  io_context::scheduler& sched;
  endpoint ep;
  R r;
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
    uv_listen((uv_stream_t*)handle,
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
                uv_accept((uv_stream_t*)thiz->handle,
                          (uv_stream_t*)sock->handle);
                stdexec::set_value(static_cast<R&&>(thiz->r), std::move(sock));
              });
  }
  uv_tcp_t* handle;
  io_context::scheduler& sched;
  endpoint ep;
  R r;
};

template<template <typename> typename T, typename... Sig>
struct socket_sender
{
  using sender_concept = stdexec::sender_t;
  using completion_signatures = stdexec::completion_signatures<Sig...>;

  template<typename R>
  friend auto tag_invoke(stdexec::connect_t,
                         const socket_sender& self,
                         R r) -> T<R>
  {
    return T<R> {self.handle, self.sched, self.ep, static_cast<R&&>(r)};
  }

  uv_tcp_t* handle;
  io_context::scheduler& sched;
  endpoint ep;
};

struct write_sender
{
};

struct read_sender
{
};

struct socket
{
  socket(io_context::scheduler& sched)
      : sched{sched}
  {
    uv_tcp_init(sched.ctx.loop(), &handle);
  }

  virtual ~socket()
  {
    uv_close(reinterpret_cast<uv_handle_t*>(&handle), nullptr);
  }

  using accept_sender = socket_sender<accept_operation, stdexec::set_value_t(std::unique_ptr<socket>&&)>;
  using connect_sender = socket_sender<accept_operation>;

  auto listen(const endpoint& ep) -> accept_sender
  {
    return accept_sender {&handle, sched, ep};
  }

  auto connect(const endpoint& ep) -> connect_sender
  {
    return connect_sender {&handle, sched, ep};
  }

  io_context::scheduler& sched;
  uv_tcp_t handle;
};

}  // namespace sawd::tcp
