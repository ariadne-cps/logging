/***************************************************************************
 *            test_concurrency.cpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

#include <atomic>
#include <fstream>
#include <future>
#include <string>
#include <thread>
#include <vector>

#include "logging/logging.hpp"
#include "logging/thread_registry_interface.hpp"

using namespace Ariadne;

class ConcurrentThreadRegistry : public ThreadRegistryInterface {
  public:
    bool has_threads_registered() const override {
        return _registered.load(std::memory_order_acquire) != 0;
    }

    void register_thread() {
        _registered.fetch_add(1, std::memory_order_acq_rel);
    }

    void unregister_thread() {
        _registered.fetch_sub(1, std::memory_order_acq_rel);
    }

  private:
    std::atomic<unsigned int> _registered{0};
};

static unsigned int count_lines(std::string const& filename) {
    std::ifstream file(filename);
    unsigned int count = 0;
    std::string line;
    while (std::getline(file,line)) ++count;
    return count;
}

static bool file_contains(std::string const& filename, std::string const& text) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file,line)) {
        if (line.find(text) != std::string::npos) return true;
    }
    return false;
}

int main() {
    ConcurrentThreadRegistry registry;
    Logger::instance().attach_thread_registry(&registry);
    Logger::instance().configuration().set_theme(TT_THEME_NONE);
    Logger::instance().configuration().set_verbosity(1);
    Logger::instance().configuration().set_prints_level_on_change_only(false);
    Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::BEFORE);

    constexpr unsigned int NUM_THREADS = 12;
    constexpr unsigned int MESSAGES_PER_THREAD = 100;
    const std::string concurrent_filename = "concurrency.log";

    Logger::instance().use_nonblocking_scheduler();
    Logger::instance().redirect_to_file(concurrent_filename.c_str());

    std::promise<void> start_promise;
    std::shared_future<void> start_future(start_promise.get_future());
    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    for (unsigned int i=0; i<NUM_THREADS; ++i) {
        threads.emplace_back([i,&registry,start_future]() {
            registry.register_thread();
            Logger::instance().register_self_thread("worker-" + std::to_string(i), 1);
            start_future.wait();
            for (unsigned int j=0; j<MESSAGES_PER_THREAD; ++j) {
                LOGGING_PRINTLN("message-" << j)
            }
            Logger::instance().unregister_thread(std::this_thread::get_id());
            registry.unregister_thread();
        });
    }

    start_promise.set_value();
    for (auto& thread : threads) thread.join();

    if (registry.has_threads_registered()) return 1;

    // Switching scheduler terminates the nonblocking scheduler and therefore
    // deterministically waits until every queued message has been consumed.
    Logger::instance().use_blocking_scheduler();
    Logger::instance().redirect_to_console();

    if (count_lines(concurrent_filename) != NUM_THREADS * MESSAGES_PER_THREAD) return 2;

    const std::string reuse_filename = "thread_id_reuse.log";
    constexpr unsigned int REUSE_ATTEMPTS = 200;

    Logger::instance().use_nonblocking_scheduler();
    Logger::instance().redirect_to_file(reuse_filename.c_str());

    for (unsigned int i=0; i<REUSE_ATTEMPTS; ++i) {
        std::thread thread([i,&registry]() {
            registry.register_thread();
            const std::string name = "reuse-" + std::to_string(i);
            Logger::instance().register_self_thread(name,1);
            LOGGING_PRINTLN("reuse-message")
            Logger::instance().unregister_thread(std::this_thread::get_id());
            registry.unregister_thread();
        });
        thread.join();
    }

    Logger::instance().use_blocking_scheduler();
    Logger::instance().redirect_to_console();

    if (count_lines(reuse_filename) != REUSE_ATTEMPTS) return 3;
    for (unsigned int i=0; i<REUSE_ATTEMPTS; ++i) {
        if (!file_contains(reuse_filename,"reuse-" + std::to_string(i) + "@")) return 4;
    }

    const std::string configuration_filename = "configuration_race.log";
    constexpr unsigned int CONFIG_MESSAGES = 500;

    Logger::instance().use_nonblocking_scheduler();
    Logger::instance().redirect_to_file(configuration_filename.c_str());

    std::atomic<bool> producer_started{false};
    std::thread producer([&registry,&producer_started]() {
        registry.register_thread();
        Logger::instance().register_self_thread("config-producer",1);
        producer_started.store(true,std::memory_order_release);
        for (unsigned int i=0; i<CONFIG_MESSAGES; ++i) {
            LOGGING_PRINTLN("config-message-" << i << " true false + -")
        }
        Logger::instance().unregister_thread(std::this_thread::get_id());
        registry.unregister_thread();
    });

    while (!producer_started.load(std::memory_order_acquire)) std::this_thread::yield();

    for (unsigned int i=0; i<CONFIG_MESSAGES; ++i) {
        auto& configuration = Logger::instance().configuration();
        configuration.set_theme((i % 2 == 0) ? TT_THEME_DARK : TT_THEME_LIGHT);
        configuration.set_thread_name_printing_policy((i % 3 == 0) ? ThreadNamePrintingPolicy::BEFORE : ThreadNamePrintingPolicy::AFTER);
        configuration.set_prints_level_on_change_only(i % 2 == 0);
        configuration.set_indents_based_on_level(i % 2 != 0);
        configuration.set_handles_multiline_output(i % 2 == 0);
        configuration.set_discards_newlines_and_indentation(i % 2 != 0);
        configuration.add_custom_keyword((i % 2 == 0) ? "config-keyword-even" : "config-keyword-odd");
    }

    producer.join();
    Logger::instance().use_blocking_scheduler();
    Logger::instance().redirect_to_console();

    if (count_lines(configuration_filename) != CONFIG_MESSAGES) return 5;

    return 0;
}
