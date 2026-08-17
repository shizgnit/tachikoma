#include <gtest/gtest.h>
#include "concurrency/spsc_queue.hpp"
#include "concurrency/task_tracker.hpp"
#include "filesystem/size_estimator.hpp"
#include "filesystem/scanner.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <numeric>
#include <vector>

namespace fs = std::filesystem;
using namespace tachikoma::concurrency;
using namespace tachikoma::filesystem;

// ============================================================
// SPSC Queue Tests
// ============================================================

TEST(SPSCQueueTest, PushAndPop) {
    SPSCQueue<int, 16> q;
    EXPECT_TRUE(q.push(42));
    auto val = q.pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 42);
}

TEST(SPSCQueueTest, EmptyQueue) {
    SPSCQueue<int, 16> q;
    EXPECT_TRUE(q.pop().has_value() == false);
    EXPECT_TRUE(q.empty());
}

TEST(SPSCQueueTest, MultipleItems) {
    SPSCQueue<int, 16> q;
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(q.push(i));
    }

    for (int i = 0; i < 10; ++i) {
        auto val = q.pop();
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(val.value(), i);
    }
    EXPECT_TRUE(q.empty());
}

TEST(SPSCQueueTest, FullQueue) {
    SPSCQueue<int, 4> q;
    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_TRUE(q.push(3));
    // Capacity is 4, but one slot is wasted to distinguish full from empty
    EXPECT_FALSE(q.push(4));
}

TEST(SPSCQueueTest, Drain) {
    SPSCQueue<int, 16> q;
    q.push(10);
    q.push(20);
    q.push(30);

    auto items = q.drain();
    EXPECT_EQ(items.size(), 3u);
    EXPECT_EQ(items[0], 10);
    EXPECT_EQ(items[1], 20);
    EXPECT_EQ(items[2], 30);
    EXPECT_TRUE(q.empty());
}

TEST(SPSCQueueTest, SizeApprox) {
    SPSCQueue<int, 16> q;
    EXPECT_EQ(q.size_approx(), 0u);
    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_EQ(q.size_approx(), 3u);
}

TEST(SPSCQueueTest, ThreadSafeProducerConsumer) {
    SPSCQueue<int, 256> q;
    std::atomic<bool> done{false};
    std::vector<int> consumed;
    std::mutex mtx;

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < 100; ++i) {
            while (!q.push(i)) {
                std::this_thread::yield();
            }
        }
        done.store(true);
    });

    // Consumer thread (main)
    int count = 0;
    while (count < 100) {
        auto val = q.pop();
        if (val.has_value()) {
            consumed.push_back(val.value());
            ++count;
        } else {
            std::this_thread::yield();
        }
    }

    producer.join();

    // Verify FIFO order
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(consumed[i], i);
    }
}

// ============================================================
// Task Tracker Tests
// ============================================================

TEST(TaskTrackerTest, SubmitAndComplete) {
    TaskTracker tracker(2);

    auto id = tracker.submit("test", []() -> uint64_t {
        return 42;
    });

    EXPECT_GT(id, 0ULL);

    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto results = tracker.drain_results();
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].id, id);
    EXPECT_EQ(results[0].state, TaskState::Done);
    EXPECT_EQ(results[0].payload, 42ULL);
}

TEST(TaskTrackerTest, MultipleTasks) {
    TaskTracker tracker(4);

    for (int i = 0; i < 5; ++i) {
        tracker.submit("task_" + std::to_string(i), [i]() -> uint64_t {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return static_cast<uint64_t>(i * 100);
        });
    }

    EXPECT_EQ(tracker.total_tasks(), 5u);

    // Wait for all to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_TRUE(tracker.all_done());
    EXPECT_EQ(tracker.completed_tasks(), 5u);
    EXPECT_EQ(tracker.running_tasks(), 0u);
    EXPECT_DOUBLE_EQ(tracker.progress(), 1.0);
}

TEST(TaskTrackerTest, TaskFailure) {
    TaskTracker tracker(2);

    tracker.submit("failing_task", []() -> uint64_t {
        throw std::runtime_error("intentional failure");
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto results = tracker.drain_results();
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].state, TaskState::Failed);
    EXPECT_EQ(results[0].payload, 0ULL);
}

TEST(TaskTrackerTest, ProgressTracking) {
    TaskTracker tracker(2);

    tracker.submit("slow_1", []() -> uint64_t {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 1;
    });
    tracker.submit("slow_2", []() -> uint64_t {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 2;
    });

    EXPECT_EQ(tracker.total_tasks(), 2u);
    EXPECT_DOUBLE_EQ(tracker.progress(), 0.0);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_DOUBLE_EQ(tracker.progress(), 1.0);

    tracker.drain_results();
}

TEST(TaskTrackerTest, CancelAll) {
    TaskTracker tracker(2);

    tracker.submit("long_task", []() -> uint64_t {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        return 999;
    });

    tracker.cancel_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // New tasks after cancel should be cancelled immediately
    auto id = tracker.submit("cancelled", []() -> uint64_t {
        return 42;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto results = tracker.drain_results();

    bool found_cancelled = false;
    for (const auto& r : results) {
        if (r.id == id) {
            found_cancelled = true;
            EXPECT_EQ(r.state, TaskState::Cancelled);
        }
    }
    EXPECT_TRUE(found_cancelled);
}

TEST(TaskTrackerTest, TaskStateQuery) {
    TaskTracker tracker(2);

    auto id = tracker.submit("state_test", []() -> uint64_t {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 77;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    tracker.drain_results();

    EXPECT_EQ(tracker.get_task_state(id), TaskState::Done);

    auto result = tracker.get_task_result(id);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->payload, 77ULL);
}

TEST(TaskTrackerTest, ResetClearsStateForRescan) {
    TaskTracker tracker(2);

    // First "scan": one task that completes.
    tracker.submit("first", []() -> uint64_t { return 1; });
    for (int i = 0; i < 200 && !(tracker.total_tasks() > 0 && tracker.all_done()); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_TRUE(tracker.all_done());
    EXPECT_EQ(tracker.completed_tasks(), 1u);

    // Reset for a second "scan" (what /scan and F5 do before resubmitting).
    tracker.reset();
    EXPECT_EQ(tracker.total_tasks(), 0u);
    EXPECT_EQ(tracker.completed_tasks(), 0u);
    EXPECT_FALSE(tracker.all_done());   // fresh state: no tasks yet
    EXPECT_TRUE(tracker.drain_results().empty());

    // Tracker is fully reusable afterwards.
    tracker.submit("second_a", []() -> uint64_t { return 10; });
    tracker.submit("second_b", []() -> uint64_t { return 20; });
    for (int i = 0; i < 200 && !tracker.all_done(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_EQ(tracker.total_tasks(), 2u);
    EXPECT_TRUE(tracker.all_done());
    // Counters reflect only the second scan, not cumulative totals.
    EXPECT_EQ(tracker.completed_tasks(), 2u);
}

// ============================================================
// Size Estimator Tests
// ============================================================

TEST(SizeEstimatorTest, ComputeTotalSize) {
    fs::path tmp = fs::temp_directory_path() / "tachikoma_size_test";
    fs::create_directories(tmp / "sub");

    std::ofstream(tmp / "f1.txt") << "12345";       // 5 bytes
    std::ofstream(tmp / "sub" / "f2.txt") << "1234567890";  // 10 bytes

    uint64_t total = SizeEstimator::compute_total_size(tmp.string());
    EXPECT_EQ(total, 15ULL);

    fs::remove_all(tmp);
}

TEST(SizeEstimatorTest, ComputeTotalSizeEmpty) {
    fs::path tmp = fs::temp_directory_path() / "tachikoma_size_empty";
    fs::create_directories(tmp);

    uint64_t total = SizeEstimator::compute_total_size(tmp.string());
    EXPECT_EQ(total, 0ULL);

    fs::remove_all(tmp);
}

TEST(SizeEstimatorTest, ComputeTotalSizeNonExistent) {
    uint64_t total = SizeEstimator::compute_total_size("/nonexistent/path/xyz");
    EXPECT_EQ(total, 0ULL);
}

TEST(SizeEstimatorTest, SortBySize) {
    std::vector<Entry> entries;

    Entry dir1;
    dir1.name = "small_dir";
    dir1.type = Entry::Type::Directory;
    dir1.total_size = 100;

    Entry dir2;
    dir2.name = "large_dir";
    dir2.type = Entry::Type::Directory;
    dir2.total_size = 10000;

    Entry file1;
    file1.name = "medium_file";
    file1.type = Entry::Type::File;
    file1.size = 5000;

    Entry file2;
    file2.name = "tiny_file";
    file2.type = Entry::Type::File;
    file2.size = 10;

    entries.push_back(dir1);
    entries.push_back(dir2);
    entries.push_back(file1);
    entries.push_back(file2);

    SizeEstimator::sort_by_size(entries);

    // Directories first, sorted by size desc
    EXPECT_EQ(entries[0].name, "large_dir");
    EXPECT_EQ(entries[1].name, "small_dir");
    // Then files, sorted by size desc
    EXPECT_EQ(entries[2].name, "medium_file");
    EXPECT_EQ(entries[3].name, "tiny_file");
}

TEST(SizeEstimatorTest, SubmitAndApplyResults) {
    TaskTracker tracker(2);
    SizeEstimator estimator(tracker);

    fs::path tmp = fs::temp_directory_path() / "tachikoma_estimator_test";
    fs::create_directories(tmp / "dir_a");
    fs::create_directories(tmp / "dir_b");

    // dir_a has more data
    for (int i = 0; i < 10; ++i) {
        std::ofstream(tmp / "dir_a" / ("f" + std::to_string(i) + ".txt"))
            << std::string(100, 'x');  // 100 bytes each = 1000 total
    }
    std::ofstream(tmp / "dir_b" / "small.txt") << "hi";  // 2 bytes

    auto entries = Scanner::list_directory(tmp.string());

    // Get mutable copies for the estimator
    std::vector<Entry> mutable_entries = entries;

    size_t submitted = estimator.submit_directory_tasks(mutable_entries);
    EXPECT_GT(submitted, 0u);

    // Wait for tasks
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Drain and apply
    auto results = tracker.drain_results();
    estimator.apply_results(results, mutable_entries);

    // Find dir_a and dir_b
    uint64_t dir_a_size = 0;
    uint64_t dir_b_size = 0;
    for (const auto& e : mutable_entries) {
        if (e.name == "dir_a") dir_a_size = e.total_size;
        if (e.name == "dir_b") dir_b_size = e.total_size;
    }

    EXPECT_GT(dir_a_size, dir_b_size);
    EXPECT_GT(dir_a_size, 0ULL);
    EXPECT_GT(dir_b_size, 0ULL);

    fs::remove_all(tmp);
}

// ============================================================
// Integration: Lock-free Queue + TaskTracker + SizeEstimator
// ============================================================

TEST(IntegrationTest, FullPipeline) {
    TaskTracker tracker(4);
    SizeEstimator estimator(tracker);

    fs::path tmp = fs::temp_directory_path() / "tachikoma_pipeline";
    fs::create_directories(tmp / "big");
    fs::create_directories(tmp / "medium");
    fs::create_directories(tmp / "small");

    // Create different sizes
    for (int i = 0; i < 20; ++i) {
        std::ofstream(tmp / "big" / ("f" + std::to_string(i) + ".bin"))
            << std::string(500, 'B');
    }
    for (int i = 0; i < 10; ++i) {
        std::ofstream(tmp / "medium" / ("f" + std::to_string(i) + ".txt"))
            << std::string(100, 'M');
    }
    std::ofstream(tmp / "small" / "tiny.txt") << "S";

    auto entries = Scanner::list_directory(tmp.string());
    std::vector<Entry> mutable_entries = entries;

    // Submit
    size_t n = estimator.submit_directory_tasks(mutable_entries);
    EXPECT_EQ(n, 3u);

    // Wait and drain in a loop (simulates main loop polling)
    int iterations = 0;
    while (!tracker.all_done() && iterations < 50) {
        auto results = tracker.drain_results();
        estimator.apply_results(results, mutable_entries);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ++iterations;
    }

    EXPECT_TRUE(tracker.all_done());

    // Sort by size and verify ordering
    SizeEstimator::sort_by_size(mutable_entries);

    // First should be "big" (largest)
    bool found_big_first = false;
    bool found_small_last = false;
    if (!mutable_entries.empty() && mutable_entries[0].type == Entry::Type::Directory) {
        found_big_first = (mutable_entries[0].name == "big");
    }
    // Last directory should be "small"
    for (int i = static_cast<int>(mutable_entries.size()) - 1; i >= 0; --i) {
        if (mutable_entries[i].type == Entry::Type::Directory) {
            found_small_last = (mutable_entries[i].name == "small");
            break;
        }
    }

    EXPECT_TRUE(found_big_first);
    EXPECT_TRUE(found_small_last);

    fs::remove_all(tmp);
}
