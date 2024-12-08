#pragma once

#include <stdexec/execution.hpp>
#include <uv.h>

namespace sawd
{

class io_context
{
public:
  struct scheduler
  {
    struct schedule_sender
    {
      using sender_concept = stdexec::sender_t;
      using completion_signatures =
          stdexec::completion_signatures<stdexec::set_value_t()>;

      template<typename R>
      struct operation
      {
        ~operation()
        {
          uv_close(reinterpret_cast<uv_handle_t*>(&handle), nullptr);
        }

        auto start() noexcept -> void
        {
          uv_async_init(ctx.loop(),
                        &handle,
                        [](uv_async_t* h)
                        {
                          auto thiz = static_cast<operation*>(h->data);
                          stdexec::set_value(static_cast<R&&>(thiz->r));
                        });
          handle.data = this;
          uv_async_send(&handle);
        }

        R r;
        io_context& ctx;
        uv_async_t handle;
      };

      struct env
      {
        io_context& ctx;

        template<class CPO>
        auto query(stdexec::get_completion_scheduler_t<CPO>) const noexcept -> scheduler
        {
          return ctx.get_scheduler();
        }
      };

      auto get_env() const noexcept -> env { return {ctx}; }

      template<typename R>
      friend auto tag_invoke(stdexec::connect_t,
                             const schedule_sender& self,
                             R r) -> operation<R>
      {
        return {static_cast<R&&>(r), self.ctx};
      }

      io_context& ctx;
    };

    auto operator==(const scheduler& other) const noexcept -> bool
    {
      return &ctx == &other.ctx;
    }
    auto schedule() const noexcept -> schedule_sender
    {
      return {ctx};
    }
    io_context& ctx;
  };


  static_assert(
      stdexec::tag_invocable<stdexec::get_env_t,
                             decltype(std::declval<scheduler>().schedule())>);
  static_assert(stdexec::scheduler<scheduler>);
  static_assert(stdexec::sender<scheduler::schedule_sender>);
  static_assert(
    requires {
    {
      stdexec::tag_invoke(stdexec::__detail::_GetComplSched(), std::declval<scheduler::schedule_sender::env>())
    } -> stdexec::same_as<stdexec::__decay_t<scheduler>>;
   });
  static_assert(stdexec::__has_schedule<scheduler>  //
                && stdexec::__sender_has_completion_scheduler<scheduler> //
                && stdexec::equality_comparable<stdexec::__decay_t<scheduler>>  //
                && stdexec::copy_constructible<stdexec::__decay_t<scheduler>>);

  auto run() -> int { return uv_run(&loop_, UV_RUN_DEFAULT); }

  auto get_scheduler() noexcept -> scheduler { return {*this}; }

  io_context() { uv_loop_init(&loop_); }

  ~io_context() { uv_loop_close(&loop_); }

  uv_loop_t* loop() { return &loop_; }

private:
  uv_loop_t loop_;
};

}  // namespace sawd
