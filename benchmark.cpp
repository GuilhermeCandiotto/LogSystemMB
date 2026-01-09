#include "LogSystem.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <chrono>

using namespace WYD_Server;
using namespace std::chrono;

// Benchmark configuration
struct BenchmarkConfig {
    int numThreads = 4;
    int messagesPerThread = 10000;
    bool enableFileLogging = true;
    bool enableGUI = false;
};

// Benchmark results
struct BenchmarkResults {
    double totalTimeSeconds;
    uint64_t totalMessages;
    double messagesPerSecond;
    double avgLatencyMicros;
    uint64_t queueFullCount;
    uint64_t queuePeakSize;
};

class LogBenchmark {
public:
    static BenchmarkResults RunSimpleLogging(const BenchmarkConfig& config) {
        std::cout << "\n=== Simple Logging Benchmark ===\n";
        std::cout << "Threads: " << config.numThreads << "\n";
        std::cout << "Messages per thread: " << config.messagesPerThread << "\n\n";

        // Setup
        if (config.enableFileLogging) {
            pLog.EnableFileLevel(LogLevel::Info);
        }

        auto start = high_resolution_clock::now();

        // Run benchmark
        std::vector<std::thread> threads;
        for (int t = 0; t < config.numThreads; ++t) {
            threads.emplace_back([&config, t]() {
                for (int i = 0; i < config.messagesPerThread; ++i) {
                    pLog.Info("Test message from thread " + std::to_string(t), 
                             "iteration " + std::to_string(i));
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        auto end = high_resolution_clock::now();
        
        // Wait for queue to drain
        std::this_thread::sleep_for(milliseconds(100));

        // Calculate results
        BenchmarkResults results;
        const auto& stats = pLog.GetStats();
        
        results.totalTimeSeconds = duration<double>(end - start).count();
        results.totalMessages = stats.totalLogs.load();
        results.messagesPerSecond = results.totalMessages / results.totalTimeSeconds;
        results.avgLatencyMicros = (results.totalTimeSeconds * 1000000.0) / results.totalMessages;
        results.queueFullCount = stats.queueFull.load();
        results.queuePeakSize = stats.queuePeak.load();

        return results;
    }

    static BenchmarkResults RunComplexLogging(const BenchmarkConfig& config) {
        std::cout << "\n=== Complex Logging Benchmark (with IP and extras) ===\n";
        std::cout << "Threads: " << config.numThreads << "\n";
        std::cout << "Messages per thread: " << config.messagesPerThread << "\n\n";

        if (config.enableFileLogging) {
            pLog.EnableFileLevel(LogLevel::Info);
            pLog.EnableFileLevel(LogLevel::Warning);
            pLog.EnableFileLevel(LogLevel::Error);
        }

        auto start = high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (int t = 0; t < config.numThreads; ++t) {
            threads.emplace_back([&config, t]() {
                unsigned int ip = (192 << 24) | (168 << 16) | (t << 8) | 1;
                
                for (int i = 0; i < config.messagesPerThread; ++i) {
                    switch (i % 3) {
                        case 0:
                            pLog.Info("Complex log message", 
                                     "Thread: " + std::to_string(t) + ", Iter: " + std::to_string(i), ip);
                            break;
                        case 1:
                            pLog.Warning("Warning message", "Context data", ip);
                            break;
                        case 2:
                            pLog.Error("Error simulation", "Error details", ip);
                            break;
                    }
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        auto end = high_resolution_clock::now();
        std::this_thread::sleep_for(milliseconds(100));

        BenchmarkResults results;
        const auto& stats = pLog.GetStats();
        
        results.totalTimeSeconds = duration<double>(end - start).count();
        results.totalMessages = stats.totalLogs.load();
        results.messagesPerSecond = results.totalMessages / results.totalTimeSeconds;
        results.avgLatencyMicros = (results.totalTimeSeconds * 1000000.0) / results.totalMessages;
        results.queueFullCount = stats.queueFull.load();
        results.queuePeakSize = stats.queuePeak.load();

        return results;
    }

    static BenchmarkResults RunStressTest(const BenchmarkConfig& config) {
        std::cout << "\n=== Stress Test (Maximum Throughput) ===\n";
        std::cout << "Threads: " << config.numThreads << "\n";
        std::cout << "Messages per thread: " << config.messagesPerThread << "\n\n";

        if (config.enableFileLogging) {
            pLog.EnableFileLevel(LogLevel::Info);
        }

        auto start = high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (int t = 0; t < config.numThreads; ++t) {
            threads.emplace_back([&config]() {
                for (int i = 0; i < config.messagesPerThread; ++i) {
                    pLog.Info("Stress test message");
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        auto end = high_resolution_clock::now();
        std::this_thread::sleep_for(milliseconds(200));

        BenchmarkResults results;
        const auto& stats = pLog.GetStats();
        
        results.totalTimeSeconds = duration<double>(end - start).count();
        results.totalMessages = stats.totalLogs.load();
        results.messagesPerSecond = results.totalMessages / results.totalTimeSeconds;
        results.avgLatencyMicros = (results.totalTimeSeconds * 1000000.0) / results.totalMessages;
        results.queueFullCount = stats.queueFull.load();
        results.queuePeakSize = stats.queuePeak.load();

        return results;
    }

    static void PrintResults(const std::string& testName, const BenchmarkResults& results) {
        std::cout << "\n--- " << testName << " Results ---\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Total time: " << results.totalTimeSeconds << " seconds\n";
        std::cout << "Total messages: " << results.totalMessages << "\n";
        std::cout << "Throughput: " << results.messagesPerSecond << " msg/s\n";
        std::cout << "Average latency: " << results.avgLatencyMicros << " ?s\n";
        std::cout << "Queue full events: " << results.queueFullCount << "\n";
        std::cout << "Queue peak size: " << results.queuePeakSize << "\n";
        std::cout << "-------------------------------\n";
    }

    static void PrintStatistics() {
        const auto& stats = pLog.GetStats();
        
        std::cout << "\n=== Overall Statistics ===\n";
        std::cout << "Uptime: " << std::fixed << std::setprecision(2) << stats.GetUptime() << " seconds\n";
        std::cout << "Total logs: " << stats.totalLogs.load() << "\n";
        std::cout << "Logs/second: " << stats.GetLogsPerSecond() << "\n";
        std::cout << "Bytes written: " << stats.bytesWritten.load() << "\n";
        std::cout << "Files rotated: " << stats.filesRotated.load() << "\n";
        std::cout << "Compressions: " << stats.compressionCount.load() << "\n\n";
        
        std::cout << "Logs by level:\n";
        const char* levelNames[] = {"Trace", "Debug", "Info", "Warning", "Error", "Quest", "Packets"};
        for (int i = 0; i < LOG_LEVEL_COUNT; ++i) {
            uint64_t count = stats.logsPerLevel[i].load();
            if (count > 0) {
                std::cout << "  " << levelNames[i] << ": " << count << "\n";
            }
        }
        std::cout << "==========================\n";
    }
};

int main() {
    std::cout << "LogSystem Performance Benchmark\n";
    std::cout << "================================\n";

    BenchmarkConfig config;
    config.numThreads = 4;
    config.messagesPerThread = 10000;
    config.enableFileLogging = true;
    config.enableGUI = false;

    // Test 1: Simple logging
    auto results1 = LogBenchmark::RunSimpleLogging(config);
    LogBenchmark::PrintResults("Simple Logging", results1);

    std::this_thread::sleep_for(seconds(1));

    // Test 2: Complex logging
    auto results2 = LogBenchmark::RunComplexLogging(config);
    LogBenchmark::PrintResults("Complex Logging", results2);

    std::this_thread::sleep_for(seconds(1));

    // Test 3: Stress test
    config.messagesPerThread = 25000;
    auto results3 = LogBenchmark::RunStressTest(config);
    LogBenchmark::PrintResults("Stress Test", results3);

    // Print overall statistics
    LogBenchmark::PrintStatistics();

    // Shutdown
    pLog.Shutdown();

    std::cout << "\nBenchmark completed. Press Enter to exit...";
    std::cin.get();

    return 0;
}
