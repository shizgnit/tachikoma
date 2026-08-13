#pragma once

#include "concurrency/spsc_queue.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace tachikoma::concurrency {

/// Unique identifier for a background task.
using TaskId = uint64_t;

/// Possible states of a tracked task.
enum class TaskState {
    Pending,
    Running,
    Done,
    Failed,
    Cancelled
};

/// Result returned by a background task.
struct TaskResult {
    TaskId id;
    TaskState state;
    std::string label;       // Human-readable label (e.g. folder path)
    uint64_t payload;        // Generic numeric payload (e.g. total_size)
    std::chrono::steady_clock::duration elapsed;
};

/// Payload pushed to the lock-free queue when a task completes.
using QueuePayload = TaskResult;

/// Tracks background tasks: submits work, runs threads, and pushes results
/// to a lock-free SPSC queue for the consumer (main/UI) thread to drain.
class TaskTracker {
public:
    explicit TaskTracker(int max_threads = 4);
    ~TaskTracker();

    // Non-copyable
    TaskTracker(const TaskTracker&) = delete;
    TaskTracker& operator=(const TaskTracker&) = delete;

    /// Submit a task. Returns the assigned TaskId.
    /// The task function runs in a background thread.
    TaskId submit(const std::string& label, std::function<uint64_t()> work);

    /// Drain completed results from the lock-free queue (call from consumer thread).
    std::vector<QueuePayload> drain_results();

    /// Total number of tasks submitted.
    size_t total_tasks() const;

    /// Number of tasks currently running.
    size_t running_tasks() const;

    /// Number of tasks completed (done + failed + cancelled).
    size_t completed_tasks() const;

    /// Overall progress as a fraction [0.0, 1.0].
    double progress() const;

    /// Cancel all pending/running tasks.
    void cancel_all();

    /// Check if all tasks have finished.
    bool all_done() const;

    /// Get the list of known task IDs.
    std::vector<TaskId> get_task_ids() const;

    /// Get state of a specific task.
    TaskState get_task_state(TaskId id) const;

    /// Get result of a specific task (if available).
    std::optional<TaskResult> get_task_result(TaskId id) const;

private:
    void worker(TaskId id, const std::string& label, std::function<uint64_t()> work);

    mutable std::mutex state_mutex_;
    std::atomic<TaskId> next_id_{0};
    std::atomic<size_t> running_count_{0};
    std::atomic<size_t> completed_count_{0};
    std::atomic<bool> cancelled_{false};

    int max_threads_;
    std::vector<std::thread> threads_;

    // Lock-free queue: producer (worker threads) -> consumer (main thread)
    SPSCQueue<QueuePayload, 4096> result_queue_;

    // Task state map (protected by state_mutex_)
    struct TaskEntry {
        TaskState state;
        std::string label;
        std::optional<TaskResult> result;
    };
    std::unordered_map<TaskId, TaskEntry> tasks_;
};

} // namespace tachikoma::concurrency
