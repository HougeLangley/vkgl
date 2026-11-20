#include "submit-fence.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

using vkgl_fence::Result;
using vkgl_fence::State;

#define CHECK(expr) do { if (!(expr)) { \
    std::fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #expr); \
    std::exit(1); \
} } while (0)

struct Backend {
    int creates = 0;
    int destroys = 0;
    int flushes = 0;
    int finishes = 0;
    int sleeps = 0;
    bool null_sync = false;
    bool fail_waits = false;
    bool signal_blocking_wait = false;
    void *poll_signaled = nullptr;
    std::vector<uint64_t> timeouts;

    void *create_sync() {
        ++creates;
        return null_sync ? nullptr : reinterpret_cast<void *>(static_cast<uintptr_t>(creates));
    }
    void destroy_sync(void *handle) {
        CHECK(handle != nullptr);
        ++destroys;
    }
    void flush_commands() { ++flushes; }
    void finish_commands() { ++finishes; }
    void sleep_for(uint64_t timeout) {
        timeouts.push_back(timeout);
        ++sleeps;
    }
    Result wait_sync(void *handle, uint64_t timeout) {
        timeouts.push_back(timeout);
        if (fail_waits) return Result::Error;
        if (timeout == 0 && handle == poll_signaled) return Result::Signaled;
        if (timeout != 0 && signal_blocking_wait) return Result::Signaled;
        return Result::Timeout;
    }
};

struct Clock {
    uint64_t now = 100;
    uint64_t now_ns() const { return now; }
};

void empty_wait_is_signaled() {
    Backend backend;
    Clock clock;
    CHECK(vkgl_fence::wait(nullptr, 0, true, 0, backend, clock) == Result::Signaled);
}

void fallback_finishes_and_signals() {
    Backend backend;
    backend.null_sync = true;
    State state;
    vkgl_fence::submit(state, backend);
    CHECK(backend.finishes == 1);
    CHECK(state.signaled);
    CHECK(vkgl_fence::observe(state, backend) == Result::Signaled);
}

void submit_flush_and_host_reset() {
    Backend backend;
    State state;
    vkgl_fence::submit(state, backend);
    CHECK(backend.creates == 1 && backend.flushes == 1);
    CHECK(state.sync != nullptr && !state.signaled);
    vkgl_fence::signal_host(state, backend);
    CHECK(backend.destroys == 1);
    CHECK(state.sync == nullptr && state.signaled);
    vkgl_fence::reset(state, backend);
    CHECK(!state.signaled);
}

void zero_timeout_does_not_sleep() {
    Backend backend;
    Clock clock;
    State state;
    State *states[] = {&state};
    CHECK(vkgl_fence::wait(states, 1, true, 0, backend, clock) == Result::Timeout);
    CHECK(backend.sleeps == 0);
}

void wait_any_polls_every_fence() {
    Backend backend;
    State first;
    State second;
    vkgl_fence::submit(first, backend);
    vkgl_fence::submit(second, backend);
    backend.poll_signaled = second.sync;
    State *states[] = {&first, &second};
    Clock clock;
    CHECK(vkgl_fence::wait(states, 2, false, 0, backend, clock) == Result::Signaled);
    CHECK(!first.signaled && second.signaled);
}

void wait_all_uses_remaining_timeout() {
    Backend backend;
    State host;
    host.signaled = true;
    State gpu;
    vkgl_fence::submit(gpu, backend);
    backend.signal_blocking_wait = true;
    State *states[] = {&host, &gpu};
    Clock clock;
    CHECK(vkgl_fence::wait(states, 2, true, 50, backend, clock) == Result::Signaled);
    CHECK(backend.timeouts.size() == 2);
    CHECK(backend.timeouts[0] == 0 && backend.timeouts[1] == 50);
}

void wait_error_is_not_a_signal() {
    Backend backend;
    State state;
    vkgl_fence::submit(state, backend);
    backend.fail_waits = true;
    State *states[] = {&state};
    Clock clock;
    CHECK(vkgl_fence::wait(states, 1, true, 0, backend, clock) == Result::Error);
    CHECK(!state.signaled);
}

} // namespace

int main() {
    empty_wait_is_signaled();
    fallback_finishes_and_signals();
    submit_flush_and_host_reset();
    zero_timeout_does_not_sleep();
    wait_any_polls_every_fence();
    wait_all_uses_remaining_timeout();
    wait_error_is_not_a_signal();
    std::puts("PASS host-fence cases=7");
    return 0;
}
