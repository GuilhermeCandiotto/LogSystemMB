#include "LogSystem.h"

namespace WYD_Server {

    namespace fs = std::filesystem;

    LogSystem::LogSystem() {
        try {
            hEdit = nullptr;
            fileIndex = 1;
            retentionDays = 7;
            maxLogSize = 1048576; // 1 MB
            compressMode = "day";
            uploadBackup = false;
            ftpServer = "";
            ftpUser = "";
            ftpPass = "";
            ftpPath = "/";

            logDir = "Log";
            fs::create_directories(logDir);

            LoadConfig("logconfig.ini");
            OpenLogFile();
        }
        catch (...) {
            retentionDays = 7;
            maxLogSize = 1048576;
            compressMode = "day";
            uploadBackup = false;
        }
    }

    LogSystem::~LogSystem() {
        try {
            if (logFile.is_open()) logFile.close();
        }
        catch (...) { }
    }

    void LogSystem::SetTarget(HWND editHandle) {
        try {
            hEdit = editHandle;
            if (!hEdit) {
                Warning("SetTarget chamado com handle nulo");
            }

            LoadConfig("logconfig.ini");
            OpenLogFile();
            CleanupOldLogs();

            Info("LogSystem inicializado com sucesso");
        }
        catch (const std::exception& e) {
            Error(std::string("Exceção em SetTarget: ") + e.what());
        }
        catch (...) {
            Error("Erro desconhecido em SetTarget");
        }
    }

    // =======================
    // Função única de log que trata todos os níveis e destinos
    // =======================
    void LogSystem::Log(LogLevel level, const std::string& msg, const std::string& extra, unsigned int ip) {
        std::string prefix = LevelToString(level);
        std::string timestamp = GetTimestamp();

        std::stringstream ss;
        ss << timestamp << " " << prefix << " ";
        ss << msg << " ";

        if (!extra.empty()) {
            ss << "[" << extra << "] ";
        }

        if (ip != 0) {
            char ipStr[32];
            sprintf_s(ipStr, "%u.%u.%u.%u",
                (ip >> 24) & 0xFF,
                (ip >> 16) & 0xFF,
                (ip >> 8) & 0xFF,
                ip & 0xFF
            );
            ss << "[IP:" << ipStr << "] ";
        }

        std::string fullMsg = ss.str();

        // Saída para RichEdit
        if (hEdit && IsWindow(hEdit)) {
            AppendColoredText(fullMsg + "\r\n", GetColor(level));
        }

        // Rotação de arquivo por data
        std::string today = GetDate();
        if (today != currentDate) {
            OpenLogFile();
            CleanupOldLogs();
        }

        // Saída para arquivo
        if (logFile.is_open() && fileLevels.find(level) != fileLevels.end()) {
            logFile << fullMsg << "\n";
        }
    }

    // =======================
    // Funções para habilitar/desabilitar níveis de log para arquivo
    // =======================
    void LogSystem::EnableFileLevel(LogLevel level) { fileLevels.insert(level); }
    void LogSystem::DisableFileLevel(LogLevel level) { fileLevels.erase(level); }

    // =======================
    // Função para abrir o arquivo de log com base na data atual e índice de rotação
    // =======================
    void LogSystem::OpenLogFile() {
        currentDate = GetDate();
        std::string filename = logDir + "/server_log_" + currentDate + "_" + std::to_string(fileIndex) + ".log";

        if (logFile.is_open()) logFile.close();
        logFile.open(filename, std::ios::app);
    }

    // =======================
    // Função para fazer a limpeza e compactação dos logs antigos podendo enviar para FTP
    // =======================
    void LogSystem::CleanupOldLogs() {
        auto now = std::chrono::system_clock::now();
        std::map<std::string, std::vector<std::string>> filesByDay;

        for (const auto& entry : fs::directory_iterator(logDir)) {
            auto name = entry.path().filename().string();
            if (!fs::is_regular_file(entry.path())) continue;

            std::regex re(R"(server_log_(\d{4}-\d{2}-\d{2})_\d+\.log)");
            std::smatch match;
            if (std::regex_match(name, match, re)) {
                std::string dayStr = match[1].str();

                std::tm tm = {};
                sscanf_s(dayStr.c_str(), "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
                tm.tm_year -= 1900;
                tm.tm_mon -= 1;
                auto fileDate = std::chrono::system_clock::from_time_t(std::mktime(&tm));

                auto ageDays = std::chrono::duration_cast<std::chrono::hours>(now - fileDate).count() / 24;

                if (ageDays >= retentionDays) {
                    filesByDay[dayStr].push_back(entry.path().string());
                }
            }
        }

        for (auto& [day, files] : filesByDay) {
            Info("Iniciando compactação dos logs do dia " + day);

            if (CompressDayLogs(day, files)) {
                Info("Compactação concluída: logpack_" + day + ".zip");

                for (auto& f : files) {
                    try {
                        std::ifstream test(f, std::ios::in | std::ios::binary);
                        if (test.is_open()) {
                            test.close();
                            fs::remove(f);
                            Info("Arquivo excluído após compactação: " + f);
                        }
                        else {
                            Warning("Arquivo ignorado (em uso): " + f);
                        }
                    }
                    catch (const std::exception& e) {
                        Error("Falha ao excluir arquivo: " + f + " - " + e.what());
                    }
                }

                if (uploadBackup) {
                    std::string localZip = logDir + "/logpack_" + day + ".zip";
                    std::string remoteZip = ftpPath + "logpack_" + day + ".zip";

                    if (UploadToFTP(localZip, ftpServer, ftpUser, ftpPass, remoteZip)) {
                        Info("Upload concluído para FTP: " + remoteZip);
                    }
                    else {
                        Error("Falha no upload para FTP: " + remoteZip);
                    }
                }
            }
            else {
                Error("Falha ao compactar logs do dia " + day);
            }
        }
    }

    // =======================
    // Função para carregar as configurações do arquivo INI de log se nao existir cria um novo
    // =======================
    void LogSystem::LoadConfig(const std::string& filename) {
        try {
            fs::path configDir = fs::current_path() / "Config";
            if (!fs::exists(configDir)) {
                fs::create_directory(configDir);
            }

            fs::path configFile = configDir / filename;
            if (!fs::exists(configFile)) {
                std::ofstream ini(configFile);
                if (!ini.is_open()) {
                    Error("Não foi possível criar o arquivo de configuração: " + configFile.string());
                    return;
                }

                ini << "[Log]\n";
                ini << "# Quantos dias os arquivos de log devem ser mantidos antes de serem compactados/excluídos\n";
                ini << "retentionDays=7\n\n";

                ini << "# Tamanho máximo (em bytes) de cada arquivo de log antes de rotacionar\n";
                ini << "# Exemplo: 1048576 = 1 MB\n";
                ini << "maxLogSize=1048576\n\n";

                ini << "# Modo de compressão dos logs antigos:\n";
                ini << "# none = não compacta\n";
                ini << "# file = compacta cada arquivo individualmente\n";
                ini << "# day  = compacta todos os arquivos de um mesmo dia em um único .zip\n";
                ini << "compressMode=day\n\n";

                ini << "[Backup]\n";
                ini << "# Define se os arquivos compactados devem ser enviados para FTP\n";
                ini << "# true = habilita envio\n";
                ini << "# false = desabilita envio(padrão)\n";
                ini << "uploadBackup=false\n\n";

                ini << "# Endereço do servidor FTP\n";
                ini << "ftpServer=ftp.meuservidor.com\n\n";

                ini << "# Usuário para login no FTP\n";
                ini << "ftpUser=usuario\n\n";

                ini << "# Senha do usuário FTP\n";
                ini << "ftpPass=senha\n\n";

                ini << "# Pasta remota no servidor FTP onde os arquivos serão armazenados\n";
                ini << "# Exemplo: /backup/\n";
                ini << "ftpPath=/backup/\n";

                ini.close();
            }

            char tempbuffer[256];

            GetPrivateProfileString("Log", "retentionDays", "7", tempbuffer, 256, configFile.string().c_str());
            retentionDays = atoi(tempbuffer);

            GetPrivateProfileString("Log", "maxLogSize", "1048576", tempbuffer, 256, configFile.string().c_str());
            maxLogSize = atoi(tempbuffer);

            GetPrivateProfileString("Log", "compressMode", "day", tempbuffer, 256, configFile.string().c_str());
            compressMode = tempbuffer;

            GetPrivateProfileString("Backup", "uploadBackup", "false", tempbuffer, 256, configFile.string().c_str());
            uploadBackup = (_stricmp(tempbuffer, "true") == 0);

            GetPrivateProfileString("Backup", "ftpServer", "", tempbuffer, 256, configFile.string().c_str());
            ftpServer = tempbuffer;

            GetPrivateProfileString("Backup", "ftpUser", "", tempbuffer, 256, configFile.string().c_str());
            ftpUser = tempbuffer;

            GetPrivateProfileString("Backup", "ftpPass", "", tempbuffer, 256, configFile.string().c_str());
            ftpPass = tempbuffer;

            GetPrivateProfileString("Backup", "ftpPath", "/", tempbuffer, 256, configFile.string().c_str());
            ftpPath = tempbuffer;

            Info("Configurações carregadas com sucesso: " + configFile.string());
        }
        catch (const std::exception& e) {
            Error(std::string("Exceção ao carregar configuração: ") + e.what());
            retentionDays = 7;
            maxLogSize = 1048576;
            compressMode = "day";
            uploadBackup = false;
            ftpServer = "";
            ftpUser = "";
            ftpPass = "";
            ftpPath = "/";
        }
        catch (...) {
            Error("Erro desconhecido ao carregar configuração.");
            retentionDays = 7;
            maxLogSize = 1048576;
            compressMode = "day";
            uploadBackup = false;
            ftpServer = "";
            ftpUser = "";
            ftpPass = "";
            ftpPath = "/";
        }
    }

    // =======================
    // Função para obter a cor associada a cada nível de log
    // =======================
    COLORREF LogSystem::GetColor(LogLevel level) {
        switch (level) {
        case LogLevel::Trace:   return RGB(128, 128, 128);      //cinza médio
        case LogLevel::Debug:   return RGB(0, 200, 255);        //azul claro/ciano
        case LogLevel::Info:    return RGB(0, 255, 0);          //verde
        case LogLevel::Warning: return RGB(255, 255, 0);        //amarelo
        case LogLevel::Error:   return RGB(255, 0, 0);          //vermelho
        }
        return RGB(200, 200, 200);                              //cinza claro
    }

    // =======================
    // Função para converter o nível de log em string
    // =======================
    std::string LogSystem::LevelToString(LogLevel level) {
        switch (level) {
        case LogLevel::Trace:   return "[TRACE]";
        case LogLevel::Debug:   return "[DEBUG]";
        case LogLevel::Info:    return "[INFO]";
        case LogLevel::Warning: return "[WARN]";
        case LogLevel::Error:   return "[ERROR]";
        }
        return "[UNKNOWN]";
    }

    // =======================
    // Função para retornar o timestamp atual no formato [YYYY-MM-DD HH:MM:SS]
    // =======================
    std::string LogSystem::GetTimestamp() {
        auto t = std::time(nullptr);
        std::tm tm;
        localtime_s(&tm, &t);
        std::ostringstream oss;
        oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "]";
        return oss.str();
    }

    // =======================
    // Função para retornar a data atual no formato YYYY-MM-DD
    // =======================
    std::string LogSystem::GetDate() {
        auto t = std::time(nullptr);
        std::tm tm;
        localtime_s(&tm, &t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d");
        return oss.str();
    }

    // =======================
    // Função para adicionar texto colorido ao controle RichEdit e mostrar o log na interface
    // =======================
    void LogSystem::AppendColoredText(const std::string& text, COLORREF textColor) {
        if (!hEdit || !IsWindow(hEdit)) {
            WriteToFile(text);
            return;
        }

        CHARRANGE cr{ -1, -1 };
        SendMessageA(hEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

        CHARFORMAT2A cf{};
        cf.cbSize = sizeof(CHARFORMAT2A);
        cf.dwMask = CFM_COLOR | CFM_BACKCOLOR | CFM_FACE | CFM_SIZE;
        cf.crTextColor = textColor;
        cf.crBackColor = RGB(20, 20, 20);
        strcpy_s(cf.szFaceName, "Consolas");
        cf.yHeight = 200;

        SendMessageA(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
        SendMessageA(hEdit, EM_SCROLL, SB_BOTTOM, 0);
    }

    // =======================
    // Função para escrever os logs em um  arquivo de texto com rotação baseada em tamanho (5 MB por padrão)
    // =======================
    void LogSystem::WriteToFile(const std::string& text) {
        if (!logFile.is_open()) OpenLogFile();

        std::error_code ec;
        std::string filename = logDir + "/server_log_" + currentDate + "_" + std::to_string(fileIndex) + ".log";
        auto size = fs::file_size(filename, ec);

        if (!ec && size >= maxLogSize) {
            fileIndex++;
            OpenLogFile();
        }

        if (logFile.is_open()) {
            logFile << GetTimestamp() << " " << text << std::endl;
        }
    }

    // =======================
    // Função para compactar vários arquivos de um mesmo dia em um único ZIP
    // =======================
    bool LogSystem::CompressDayLogs(const std::string& day, const std::vector<std::string>& files) {
        try {
            std::string zipName = logDir + "/logpack_" + day + ".zip";

            zipFile zf = zipOpen(zipName.c_str(), APPEND_STATUS_CREATE);
            if (!zf) {
                Error("Falha ao criar ZIP: " + zipName);
                return false;
            }

            for (const auto& file : files) {
                zip_fileinfo zi = {};

                if (zipOpenNewFileInZip(zf, fs::path(file).filename().string().c_str(), &zi,
                    nullptr, 0, nullptr, 0, nullptr,
                    Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK) {
                    Warning("Falha ao adicionar arquivo ao ZIP: " + file);
                    continue;
                }

                std::ifstream in(file, std::ios::binary);
                if (!in.is_open()) {
                    Warning("Não foi possível abrir arquivo para compactação: " + file);
                    continue;
                }

                const size_t chunkSize = 1024 * 1024;
                std::vector<char> buffer(chunkSize);
                while (in.read(buffer.data(), buffer.size()) || in.gcount() > 0) {
                    zipWriteInFileInZip(zf, buffer.data(), static_cast<unsigned int>(in.gcount()));
                }

                zipCloseFileInZip(zf);
            }

            zipClose(zf, nullptr);
            Info("Compactação concluída: " + zipName);
            return true;
        }
        catch (...) {
            Error("Exceção em CompressDayLogs");
            return false;
        }
    }

    // =======================
    // Função para upload dos logs antigos via FTP
    // =======================
    bool LogSystem::UploadToFTP(const std::string& localFile, const std::string& server,
        const std::string& user, const std::string& pass, const std::string& remotePath) {
        try {
            HINTERNET hInternet = InternetOpenA("LogUploader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (!hInternet) {
                Error("Falha ao inicializar InternetOpen");
                return false;
            }

            HINTERNET hFtp = InternetConnectA(hInternet, server.c_str(), INTERNET_DEFAULT_FTP_PORT,
                user.c_str(), pass.c_str(), INTERNET_SERVICE_FTP, 0, 0);
            if (!hFtp) {
                Error("Falha ao conectar ao servidor FTP: " + server);
                InternetCloseHandle(hInternet);
                return false;
            }

            bool result = FtpPutFileA(hFtp, localFile.c_str(), remotePath.c_str(), FTP_TRANSFER_TYPE_BINARY, 0);
            if (result) {
                Info("Upload concluído: " + remotePath);
            }
            else {
                Error("Falha no upload para FTP: " + remotePath);
            }

            InternetCloseHandle(hFtp);
            InternetCloseHandle(hInternet);

            return result;
        }
        catch (...) {
            Error("Exceção em UploadToFTP");
            return false;
        }
    }
}