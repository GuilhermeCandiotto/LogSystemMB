#ifndef __LOGSYSTEM_H__
#define __LOGSYSTEM_H__

#include <windows.h>
#include <string>
#include <set>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <richedit.h>
#include <zlib.h>
#include <minizip/zip.h>
#include <regex>
#include <map>
#include <wininet.h>
#include <vector>
#include <format>   // C++20
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <array>

// Definição da macro para usar com CreateWindowExA
#define MSFTEDIT_CLASSA "RICHEDIT50W"

namespace WYD_Server {

    static constexpr int TARGET_COUNT = 2;
	static constexpr int LOG_LEVEL_COUNT = 7;
    static constexpr size_t LOCK_FREE_QUEUE_SIZE = 8192; // Must be power of 2
    static constexpr size_t STRING_POOL_SIZE = 32;
    
    enum class LogLevel { Trace, Debug, Info, Warning, Error, Quest, Packets };
    enum class TargetSide { Left = 0, Right = 1 };

    // Estrutura para mensagem de log na fila
    struct LogMessage {
        LogLevel level;
        std::string message;
        std::string extra;
        unsigned int ip;
        std::string timestamp;
        std::string fullText;
        
        LogMessage() = default;
        LogMessage(const LogMessage&) = default;
        LogMessage& operator=(const LogMessage&) = default;
        LogMessage(LogMessage&&) noexcept = default;
        LogMessage& operator=(LogMessage&&) noexcept = default;
    };

    // Lock-Free Queue simplificada (SPSC - Single Producer Single Consumer)
    template<typename T, size_t Size>
    class LockFreeQueue {
    private:
        std::array<T, Size> buffer;
        std::atomic<size_t> writeIndex{0};
        std::atomic<size_t> readIndex{0};
        
        static constexpr size_t Mask = Size - 1;
        static_assert((Size & Mask) == 0, "Size must be power of 2");
        
    public:
        bool TryPush(T&& item) {
            size_t currentWrite = writeIndex.load(std::memory_order_relaxed);
            size_t nextWrite = (currentWrite + 1) & Mask;
            
            if (nextWrite == readIndex.load(std::memory_order_acquire)) {
                return false; // Queue full
            }
            
            buffer[currentWrite] = std::move(item);
            writeIndex.store(nextWrite, std::memory_order_release);
            return true;
        }
        
        bool TryPop(T& item) {
            size_t currentRead = readIndex.load(std::memory_order_relaxed);
            
            if (currentRead == writeIndex.load(std::memory_order_acquire)) {
                return false; // Queue empty
            }
            
            item = std::move(buffer[currentRead]);
            readIndex.store((currentRead + 1) & Mask, std::memory_order_release);
            return true;
        }
        
        bool IsEmpty() const {
            return readIndex.load(std::memory_order_acquire) == 
                   writeIndex.load(std::memory_order_acquire);
        }
        
        size_t Size() const {
            size_t write = writeIndex.load(std::memory_order_acquire);
            size_t read = readIndex.load(std::memory_order_acquire);
            return (write - read) & Mask;
        }
    };

    // String Pool para reduzir alocações
    class StringPool {
    private:
        struct PoolEntry {
            std::string buffer;
            std::atomic<bool> inUse{false};
        };
        
        std::array<PoolEntry, STRING_POOL_SIZE> pool;
        std::atomic<size_t> nextIndex{0};
        
    public:
        std::string* Acquire() {
            // Try to find free entry
            for (size_t i = 0; i < STRING_POOL_SIZE; ++i) {
                size_t idx = (nextIndex.fetch_add(1, std::memory_order_relaxed) % STRING_POOL_SIZE);
                bool expected = false;
                if (pool[idx].inUse.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
                    pool[idx].buffer.clear();
                    return &pool[idx].buffer;
                }
            }
            // All busy, return new allocation
            return new std::string();
        }
        
        void Release(std::string* str) {
            // Check if from pool
            for (auto& entry : pool) {
                if (&entry.buffer == str) {
                    entry.inUse.store(false, std::memory_order_release);
                    return;
                }
            }
            // Was allocated, delete it
            delete str;
        }
    };

    // Timestamp cache para evitar chamadas repetidas
    class TimestampCache {
    private:
        std::atomic<std::chrono::system_clock::time_point> lastUpdate;
        std::atomic<bool> updateFlag{false};
        std::string cached;
        std::mutex cacheMutex;
        
    public:
        std::string Get() {
            auto now = std::chrono::system_clock::now();
            auto last = lastUpdate.load(std::memory_order_relaxed);
            
            // Update cache if older than 1 second
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last).count() >= 1) {
                bool expected = false;
                if (updateFlag.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
                    auto t = std::chrono::system_clock::to_time_t(now);
                    std::tm tm;
                    localtime_s(&tm, &t);
                    
                    std::ostringstream oss;
                    oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "]";
                    
                    {
                        std::lock_guard<std::mutex> lock(cacheMutex);
                        cached = oss.str();
                    }
                    
                    lastUpdate.store(now, std::memory_order_release);
                    updateFlag.store(false, std::memory_order_release);
                }
            }
            
            std::lock_guard<std::mutex> lock(cacheMutex);
            return cached;
        }
    };

    // Performance statistics
    struct PerformanceStats {
        std::atomic<uint64_t> totalLogs{0};
        std::atomic<uint64_t> logsPerLevel[LOG_LEVEL_COUNT]{};
        std::atomic<uint64_t> bytesWritten{0};
        std::atomic<uint64_t> filesRotated{0};
        std::atomic<uint64_t> compressionCount{0};
        std::atomic<uint64_t> queueFull{0};
        std::atomic<uint64_t> queuePeak{0};
        std::chrono::steady_clock::time_point startTime;
        
        PerformanceStats() : startTime(std::chrono::steady_clock::now()) {}
        
        void RecordLog(LogLevel level, size_t bytes) {
            totalLogs.fetch_add(1, std::memory_order_relaxed);
            logsPerLevel[(int)level].fetch_add(1, std::memory_order_relaxed);
            bytesWritten.fetch_add(bytes, std::memory_order_relaxed);
        }
        
        double GetUptime() const {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration<double>(now - startTime).count();
        }
        
        double GetLogsPerSecond() const {
            double uptime = GetUptime();
            return uptime > 0 ? totalLogs.load(std::memory_order_relaxed) / uptime : 0;
        }
    };

    // RAII wrapper para handles do Windows
    class WinHandle {
    public:
        WinHandle(HANDLE h = nullptr) : handle(h) {}
        ~WinHandle() { if (handle && handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
        WinHandle(const WinHandle&) = delete;
        WinHandle& operator=(const WinHandle&) = delete;
        WinHandle(WinHandle&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
        WinHandle& operator=(WinHandle&& other) noexcept {
            if (this != &other) {
                if (handle && handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
                handle = other.handle;
                other.handle = nullptr;
            }
            return *this;
        }
        operator HANDLE() const { return handle; }
        HANDLE get() const { return handle; }
    private:
        HANDLE handle;
    };

    class LogSystem {
    public:
        LogSystem();
        ~LogSystem();

        void Initialize();
		void SetTarget(TargetSide side, HWND editHandle);
        void Log(LogLevel level, const std::string& msg, const std::string& extra = "", unsigned int ip = 0);

        inline void Trace(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Trace, msg, extra, ip); }
        inline void Debug(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Debug, msg, extra, ip); }
        inline void Info(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Info, msg, extra, ip); }
        inline void Warning(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Warning, msg, extra, ip); }
        inline void Error(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Error, msg, extra, ip); }
        inline void Quest(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Quest, msg, extra, ip); }
        inline void Packets(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Packets, msg, extra, ip); }

		void EnableFileLevel(LogLevel level);
		void DisableFileLevel(LogLevel level);

        void CleanupOldLogs();
        void LoadConfig(const std::string& filename);

        void SetMaxRichEditLines(int maxLines);
        void ClearRichEdit(TargetSide side);

		template<typename... Args>
        std::string Format(const char* fmt, Args&&... args) {
            return std::format(fmt, std::forward<Args>(args)...);
        }

        void Shutdown();
        
        // Performance queries
        const PerformanceStats& GetStats() const { return stats; }
        bool IsAsyncEnabled() const { return asyncLogging; }
        bool IsHeadless() const { return headlessMode; }
        std::string GetCompressMode() const { return compressMode; }

    private:
		HWND targets[TARGET_COUNT] = { nullptr, nullptr };
		TargetSide routing[LOG_LEVEL_COUNT];
        COLORREF levelColors[LOG_LEVEL_COUNT];

        std::ofstream logFile;
        std::string currentDate;
        std::set<LogLevel> fileLevels;
        int retentionDays;
        std::string logDir;
        int fileIndex;
        size_t maxLogSize;

        std::string compressMode;
        bool uploadBackup;
        std::string ftpServer;
        std::string ftpUser;
        std::string ftpPass;
        std::string ftpPath;

        int maxRichEditLines;
        bool headlessMode;
        bool asyncLogging;

        // Thread safety
        std::mutex logMutex;
        std::mutex fileMutex;
        std::mutex targetMutex;
        
        // Lock-free async logging
        LockFreeQueue<LogMessage, LOCK_FREE_QUEUE_SIZE> lockFreeQueue;
        std::condition_variable queueCV;
        std::thread workerThread;
        std::atomic<bool> stopWorker;
        
        // Performance optimizations
        StringPool stringPool;
        TimestampCache timestampCache;
        PerformanceStats stats;
        std::atomic<int> flushCounter{0};

        COLORREF GetColor(LogLevel level);
        std::string LevelToString(LogLevel level);
        std::string GetTimestamp();
        std::string GetDate();
        void AppendColoredText(HWND target, const std::string& text, COLORREF textColor);
        void WriteToFile(const std::string& text);
        void OpenLogFile();

        bool CompressFile(const std::string& file);
        bool CompressDayLogs(const std::string& day, const std::vector<std::string>& files);
        bool UploadToFTP(const std::string& localFile, const std::string& server,
            const std::string& user, const std::string& pass, const std::string& remotePath);

        void WorkerThreadFunc();
        void ProcessLogMessage(const LogMessage& msg);
        void TrimRichEdit(HWND target);

        std::string EncryptPassword(const std::string& password);
        std::string DecryptPassword(const std::string& encrypted);
    };

    class LogManager {
    public:
        static LogManager& Instance() {
            static LogManager instance;
            return instance;
        }

        LogSystem& GetLogInst() { return logInst; }

    private:
        LogManager() = default;
        ~LogManager() = default;

        LogManager(const LogManager&) = delete;
        LogManager& operator=(const LogManager&) = delete;

        LogSystem logInst;
    };
} // namespace WYD_Server

#define pLog (LogManager::Instance().GetLogInst())
#endif // __LOGSYSTEM_H__