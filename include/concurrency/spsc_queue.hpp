#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>
#include <type_traits>

namespace tachikoma::concurrency {

/// Lock-free Single-Producer Single-Consumer (SPSC) ring buffer queue.
/// Thread-safe without mutexes; uses atomics for head/tail indices.
///
/// Template Parameters:
///   T          - element type (must be trivially copyable or movable)
///   Capacity   - power-of-two ring buffer capacity (default 1024)
template <typename T, size_t Capacity = 1024>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert(Capacity >= 2, "Capacity must be >= 2");

public:
    explicit SPSCQueue() : buffer_(Capacity), head_(0), tail_(0) {}

    ~SPSCQueue() = default;

    // Non-copyable, non-movable
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    /// Push an item from the producer thread. Returns false if queue is full.
    bool push(T item) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) & (Capacity - 1);

        // Check if queue is full
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }

        buffer_[current_tail] = std::move(item);
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    /// Pop an item from the consumer thread. Returns nullopt if queue is empty.
    std::optional<T> pop() {
        size_t current_head = head_.load(std::memory_order_relaxed);

        // Check if queue is empty
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt;
        }

        T item = std::move(buffer_[current_head]);
        head_.store((current_head + 1) & (Capacity - 1), std::memory_order_release);
        return item;
    }

    /// Try to pop multiple items (batch drain). Returns drained items.
    std::vector<T> drain() {
        std::vector<T> items;
        while (true) {
            auto item = pop();
            if (!item.has_value()) break;
            items.push_back(std::move(item.value()));
        }
        return items;
    }

    /// Check if the queue appears empty (approximate; no synchronization barrier).
    bool empty() const {
        size_t current_head = head_.load(std::memory_order_relaxed);
        return current_head == tail_.load(std::memory_order_relaxed);
    }

    /// Approximate number of items in the queue.
    size_t size_approx() const {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_relaxed);
        return (tail - head) & (Capacity - 1);
    }

private:
    std::vector<T> buffer_;
    alignas(64) std::atomic<size_t> head_;  // Cache-line aligned to avoid false sharing
    alignas(64) std::atomic<size_t> tail_;
};

} // namespace tachikoma::concurrency
