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

// Definição da macro para usar com CreateWindowExA
#define MSFTEDIT_CLASSA "RICHEDIT50W"

namespace WYD_Server {

    enum class LogLevel { Trace, Debug, Info, Warning, Error };

    class LogSystem {
    public:
        LogSystem();
        ~LogSystem();

        void SetTarget(HWND editHandle);

        // Método único de log
        void Log(LogLevel level, const std::string& msg, const std::string& extra = "", unsigned int ip = 0);

        // Wrappers inline simplificados
        inline void Trace(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Trace, msg, extra, ip); }
        inline void Debug(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Debug, msg, extra, ip); }
        inline void Info(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Info, msg, extra, ip); }
        inline void Warning(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Warning, msg, extra, ip); }
        inline void Error(const std::string& msg, const std::string& extra = "", unsigned int ip = 0) { Log(LogLevel::Error, msg, extra, ip); }

        void EnableFileLevel(LogLevel level);
        void DisableFileLevel(LogLevel level);

        void CleanupOldLogs();  											                    // compacta e limpa logs antigos
        void LoadConfig(const std::string& filename);											// lê configurações do logconfig.ini

        template<typename... Args>
        const std::string& Format(const char* fmt, Args... args) {
            buffer = std::format(fmt, args...);  // C++20
            return buffer;
        }

    private:
        HWND hEdit;                         // handle do RichEdit
        std::ofstream logFile;              // arquivo de log atual (wide-char)
        std::string currentDate;            // data corrente (YYYY-MM-DD)
        std::set<LogLevel> fileLevels;      // níveis que vão para arquivo
        int retentionDays;                  // dias de retenção dos logs
        std::string logDir;                 // diretório dos logs
        int fileIndex;                      // índice de rotação do arquivo (server_YYYY-MM-DD_X.log)
        size_t maxLogSize;                  // tamanho máximo em bytes antes de rotacionar

        // Configurações de compressão e backup
        std::string compressMode;           // modo de compressão: none, file, day
        bool uploadBackup;                  // habilita/desabilita upload para FTP
        std::string ftpServer;              // servidor FTP
        std::string ftpUser;                // usuário FTP
        std::string ftpPass;                // senha FTP
        std::string ftpPath;                // pasta remota no FTP

        std::string buffer;				    // buffer temporário global para formatação

        COLORREF GetColor(LogLevel level);                                                      // cor por nível
        std::string LevelToString(LogLevel level);                                              // string por nível
        std::string GetTimestamp();                                                             // timestamp atual
        std::string GetDate();                                                                  // data atual
        void AppendColoredText(const std::string& text, COLORREF color);                        // escreve no RichEdit
        void WriteToFile(const std::string& text);                                              // escreve no arquivo
        void OpenLogFile(); 														            // abre/rotaciona arquivo

        bool CompressDayLogs(const std::string& day, const std::vector<std::string>& files);    // compacta vários arquivos em um ZIP
        bool UploadToFTP(const std::string& localFile, const std::string& server,               // envia arquivo para FTP
            const std::string& user, const std::string& pass, const std::string& remotePath);
    };

    class LogManager {
    public:
        // Retorna a instância única
        static LogManager& Instance() {
            static LogManager instance; // criado na primeira chamada, destruído automaticamente no fim do programa
            return instance;
        }

        LogSystem& GetLogInst() { return logInst; }

        // Outras funções utilitárias podem ser adicionadas aqui
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