#pragma once
#include "io_context.hpp"

namespace sawd {

template <typename P>
struct socket {
  using handle_type = P;

  socket(io_context::scheduler& sched) : handle{sched.ctx.loop_->resource<handle_type>()} {}
  virtual ~socket() = default;

  struct accept_sender {
    tcp->on<uvw::listen_event>([](const uvw::listen_event &, uvw::tcp_handle &srv) {
        std::shared_ptr<uvw::tcp_handle> client = srv.parent().resource<uvw::tcp_handle>();

        client->on<uvw::close_event>([ptr = srv.shared_from_this()](const uvw::close_event &, uvw::tcp_handle &) { ptr->close(); });
        client->on<uvw::end_event>([](const uvw::end_event &, uvw::tcp_handle &client) { client.close(); });

        srv.accept(*client);
        client->read();
    });

    tcp->bind("127.0.0.1", 4242);
    tcp->listen();
  };

  struct write_sender {
  };

  struct read_sender {
  };


  auto async_write(std::string_view data) -> void {
  }

  auto async_read() -> void {
  }

  std::shared_ptr<handle_type> handle{};
};

struct tcp_socket : public socket<uvw::tcp_handle> {
};

struct udp_socket : public socket<uvw::udp_handle> {
};

} // namespace sawd
