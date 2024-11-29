#pragma once

#include <uvw.hpp>
#include <stdexec/execution.hpp>

namespace sawd {

class io_context {
public:

  struct scheduler {
    struct schedule_sender {
      using sender_concept = stdexec::sender_t;
      using completion_signatures = stdexec::completion_signatures<stdexec::set_value_t()>;

      template <typename R>
      struct operation {
        auto start() noexcept -> void {
          auto async = ctx.loop_->resource<uvw::async_handle>();
          async->on<uvw::async_event>([this] (const uvw::async_event&, uvw::async_handle& handle) {
            stdexec::set_value(static_cast<R&&>(r));
            handle.close();
          });
          async->send();
        }

        io_context& ctx;
        R r;
      };

      template <typename R>
      friend auto tag_invoke(stdexec::connect_t, const schedule_sender& self, R r) -> operation<R> {
        return operation<R>{self.ctx, static_cast<R&&>(r)};
      }

      io_context& ctx;
    };

    // struct 

    static_assert(stdexec::sender<schedule_sender>);
    auto schedule() -> schedule_sender {
        return schedule_sender{ctx};
    }

    io_context& ctx;
  };

  auto run() -> int {
    return loop_->run();
  }

  auto get_scheduler() -> scheduler {
    return scheduler{*this};
  }

  std::shared_ptr<uvw::loop> loop_{uvw::loop::get_default()};
};

} // namespace sawd
