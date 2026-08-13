#include "concurrency/task_tracker.hpp"
#include <algorithm>

namespace tachikoma::concurrency {

TaskTracker::TaskTracker(int max_threads) : max_threads_(max_threads) {}

TaskTracker::~TaskTracker() {
    cancel_all();
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

TaskId TaskTracker::submit(const std::string& label, std::function<uint64_t()> work) {
    TaskId id = ++next_id_;

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        tasks_[id] = {TaskState::Pending, label, std::nullopt};
    }

    // Launch directly as a detached thread (simpler than a thread pool for this use case)
    threads_.emplace_back(&TaskTracker::worker, this, id, label, std::move(work));

    return id;
}

void TaskTracker::worker(TaskId id, const std::string& label, std::function<uint64_t()> work) {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto it = tasks_.find(id);
        if (it != tasks_.end()) {
            it->second.state = TaskState::Running;
        }
    }

    running_count_.fetch_add(1, std::memory_order_relaxed);

    auto start = std::chrono::steady_clock::now();

    TaskResult result;
    result.id = id;
    result.label = label;

    if (cancelled_.load(std::memory_order_relaxed)) {
        result.state = TaskState::Cancelled;
        result.payload = 0;
    } else {
        try {
            result.payload = work();
            result.state = TaskState::Done;
        } catch (const std::exception&) {
            result.state = TaskState::Failed;
            result.payload = 0;
        }
    }

    auto end = std::chrono::steady_clock::now();
    result.elapsed = end - start;

    // Update task state
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto it = tasks_.find(id);
        if (it != tasks_.end()) {
            it->second.state = result.state;
            it->second.result = result;
        }
    }

    running_count_.fetch_sub(1, std::memory_order_relaxed);
    completed_count_.fetch_add(1, std::memory_order_relaxed);

    // Push to lock-free queue for the consumer thread
    result_queue_.push(std::move(result));
}

std::vector<QueuePayload> TaskTracker::drain_results() {
    return result_queue_.drain();
}

size_t TaskTracker::total_tasks() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return tasks_.size();
}

size_t TaskTracker::running_tasks() const {
    return running_count_.load(std::memory_order_relaxed);
}

size_t TaskTracker::completed_tasks() const {
    return completed_count_.load(std::memory_order_relaxed);
}

double TaskTracker::progress() const {
    size_t total = total_tasks();
    if (total == 0) return 1.0;
    size_t done = completed_tasks();
    return static_cast<double>(done) / static_cast<double>(total);
}

void TaskTracker::cancel_all() {
    cancelled_.store(true, std::memory_order_relaxed);
}

bool TaskTracker::all_done() const {
    size_t total = total_tasks();
    size_t done = completed_tasks();
    return total > 0 && done >= total;
}

std::vector<TaskId> TaskTracker::get_task_ids() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    std::vector<TaskId> ids;
    ids.reserve(tasks_.size());
    for (const auto& [id, _] : tasks_) {
        ids.push_back(id);
    }
    return ids;
}

TaskState TaskTracker::get_task_state(TaskId id) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto it = tasks_.find(id);
    if (it != tasks_.end()) {
        return it->second.state;
    }
    return TaskState::Failed;
}

std::optional<TaskResult> TaskTracker::get_task_result(TaskId id) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto it = tasks_.find(id);
    if (it != tasks_.end() && it->second.result.has_value()) {
        return it->second.result;
    }
    return std::nullopt;
}

} // namespace tachikoma::concurrency
