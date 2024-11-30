#pragma once

#include <uv.h>
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
        ~operation() {
          uv_close(reinterpret_cast<uv_handle_t*>(&handle), nullptr);
        }

        auto start() noexcept -> void {
          uv_async_init(loop, &handle, +[](uv_async_t* h) {
              auto thiz = static_cast<operation*>(h->data);
              stdexec::set_value(static_cast<R&&>(thiz->r));
          });
          handle.data = this;
          uv_async_send(&handle);
        }

        uv_async_t handle;
        uv_loop_t* loop;
        R r;
      };

      template <typename R>
      friend auto tag_invoke(stdexec::connect_t, const schedule_sender& self, R r) -> operation<R> {
        return operation<R>{{}, self.ctx, static_cast<R&&>(r)};
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
    return uv_run(&loop_, UV_RUN_DEFAULT);
  }

  auto get_scheduler() -> scheduler {
    return scheduler{*this};
  }

  io_context() {
    uv_loop_init(&loop_);
  }

  ~io_context() {
    uv_loop_close(&loop_);
  }

  uv_loop_t* loop() {
    return &loop_;
  }

private:
  uv_loop_t loop_;
};

} // namespace sawd
