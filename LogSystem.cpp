#include "LogSystem.h"

using namespace WYD_Server;
namespace fs = std::filesystem;

LogSystem::LogSystem() : stopWorker(false), maxRichEditLines(10000), headlessMode(false), asyncLogging(true) {
    try {
        routing[(int)LogLevel::Trace] = TargetSide::Left;
        routing[(int)LogLevel::Debug] = TargetSide::Left;
        routing[(int)LogLevel::Info] = TargetSide::Left;
        routing[(int)LogLevel::Warning] = TargetSide::Left;
        routing[(int)LogLevel::Error] = TargetSide::Left;
        routing[(int)LogLevel::Quest] = TargetSide::Left;
        routing[(int)LogLevel::Packets] = TargetSide::Right;

		levelColors[(int)LogLevel::Trace] = RGB(128, 128, 128);
		levelColors[(int)LogLevel::Debug] = RGB(0, 200, 255);
		levelColors[(int)LogLevel::Info] = RGB(0, 255, 0);
		levelColors[(int)LogLevel::Warning] = RGB(255, 255, 0);
        levelColors[(int)LogLevel::Error] = RGB(255, 0, 0);
		levelColors[(int)LogLevel::Quest] = RGB(255, 105, 180);
		levelColors[(int)LogLevel::Packets] = RGB(255, 165, 0);

        targets[0] = nullptr;
        targets[1] = nullptr;

        fileIndex = 1;
        retentionDays = 7;
        maxLogSize = 1048576;
        compressMode = "day";
        uploadBackup = false;
        ftpServer = "";
        ftpUser = "";
        ftpPass = "";
        ftpPath = "/";

        logDir = "Log";
        fs::create_directories(logDir);

        Initialize();

        // Iniciar thread worker para async logging
        if (asyncLogging) {
            workerThread = std::thread(&LogSystem::WorkerThreadFunc, this);
        }
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
        Shutdown();
        if (logFile.is_open()) 
            logFile.close();
    }
    catch (...) { }
}

void LogSystem::Shutdown() {
    if (asyncLogging && workerThread.joinable()) {
        stopWorker.store(true, std::memory_order_release);
            
        // Wake up worker thread if sleeping
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
            
        // Wait for worker to finish (max 5 seconds)
        auto start = std::chrono::steady_clock::now();
        while (workerThread.joinable()) {
            if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
                // Force termination after timeout
                Warning("Worker thread timeout - forcing shutdown");
                break;
            }
                
            // Try to join
            if (workerThread.joinable()) {
                workerThread.join();
                break;
            }
        }
    }
        
    // Final flush
    if (logFile.is_open()) {
        logFile.flush();
    }
}

void LogSystem::Initialize() {
	LoadConfig("logconfig.ini");
	OpenLogFile();
	CleanupOldLogs();
}

void LogSystem::SetTarget(TargetSide side, HWND editHandle) {
    try {
        std::lock_guard<std::mutex> lock(targetMutex);
        targets[(int)side] = editHandle;

        if (!editHandle) {
            Warning("SetTarget chamado com handle nulo");
            return;
        }

        if (side == TargetSide::Left)
            Info("LogSystem inicializado com sucesso");
        else
            Packets("PacketSystem inicializado com sucesso");
    }
    catch (const std::exception& e) {
        Error(std::string("Exceção em SetTarget: ") + e.what());
    }
    catch (...) {
        Error("Erro desconhecido em SetTarget");
    }
}

void LogSystem::SetMaxRichEditLines(int maxLines) {
    std::lock_guard<std::mutex> lock(targetMutex);
    maxRichEditLines = maxLines;
}

void LogSystem::ClearRichEdit(TargetSide side) {
    std::lock_guard<std::mutex> lock(targetMutex);
    HWND target = targets[(int)side];
    if (target && IsWindow(target)) {
        SendMessageA(target, EM_SETREADONLY, FALSE, 0);
        SetWindowTextA(target, "");
        SendMessageA(target, EM_SETREADONLY, TRUE, 0);
    }
}

// =======================
// Thread worker para processamento assíncrono - agora com lock-free queue
// =======================
void LogSystem::WorkerThreadFunc() {
    LogMessage msg;
        
    while (!stopWorker.load(std::memory_order_acquire)) {
        bool processedAny = false;
            
        // Try lock-free queue first
        while (lockFreeQueue.TryPop(msg)) {
            ProcessLogMessage(msg);
            processedAny = true;
                
            // Update queue peak
            size_t currentSize = lockFreeQueue.Size();
            uint64_t peak = stats.queuePeak.load(std::memory_order_relaxed);
            while (currentSize > peak && 
                    !stats.queuePeak.compare_exchange_weak(peak, currentSize, std::memory_order_relaxed));
                
            // Check stop flag periodically
            if (stopWorker.load(std::memory_order_acquire)) {
                break;
            }
        }
            
        // Sleep briefly if queue empty and not stopping
        if (!processedAny && !stopWorker.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    // Processar mensagens restantes antes de sair (max 100ms)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    while (lockFreeQueue.TryPop(msg) && std::chrono::steady_clock::now() < deadline) {
        ProcessLogMessage(msg);
    }
        
    // Final flush do arquivo
    if (logFile.is_open()) {
        logFile.flush();
    }
}

void LogSystem::ProcessLogMessage(const LogMessage& msg) {
    TargetSide side = routing[(int)msg.level];
        
    {
        std::lock_guard<std::mutex> lock(targetMutex);
        HWND target = targets[(int)side];

        if (target && IsWindow(target) && !headlessMode) {
            AppendColoredText(target, msg.fullText, GetColor(msg.level));
            TrimRichEdit(target);
        }
    }

    // Rotação de arquivo por data
    std::string today = GetDate();
    if (today != currentDate) {
        std::lock_guard<std::mutex> lock(fileMutex);
        if (today != currentDate) {  // Double-check após adquirir lock
            currentDate = today;
            fileIndex = 1;  // Reset index quando muda o dia
            OpenLogFile();
            CleanupOldLogs();
        }
    }

    // Saída para arquivo
    if (fileLevels.find(msg.level) != fileLevels.end()) {
        std::lock_guard<std::mutex> lock(fileMutex);
        WriteToFile(msg.fullText);
    }
}

void LogSystem::TrimRichEdit(HWND target) {
    if (!target || !IsWindow(target)) return;

    int lineCount = (int)SendMessageA(target, EM_GETLINECOUNT, 0, 0);
        
    if (lineCount > maxRichEditLines) {
        int linesToDelete = lineCount - maxRichEditLines;
        int charIndex = (int)SendMessageA(target, EM_LINEINDEX, linesToDelete, 0);
            
        SendMessageA(target, EM_SETREADONLY, FALSE, 0);
        SendMessageA(target, EM_SETSEL, 0, charIndex);
        SendMessageA(target, EM_REPLACESEL, FALSE, (LPARAM)"");
        SendMessageA(target, EM_SETREADONLY, TRUE, 0);
    }
}

// =======================
// Função única de log - agora com otimizações de performance
// =======================
void LogSystem::Log(LogLevel level, const std::string& msg, const std::string& extra, unsigned int ip) {
    std::string prefix = LevelToString(level);
    std::string timestamp = timestampCache.Get(); // Use cached timestamp

    // Use string pool to reduce allocations
    std::string* poolStr = stringPool.Acquire();
    std::stringstream ss;
    ss << timestamp << " " << prefix << " " << msg;

    if (!extra.empty()) {
        ss << " [" << extra << "]";
    }

    if (ip != 0) {
        char ipStr[32];
        sprintf_s(ipStr, "%u.%u.%u.%u",
            (ip >> 24) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF,
            ip & 0xFF
        );
        ss << " [IP:" << ipStr << "]";
    }

    *poolStr = ss.str();
    std::string fullMsg = *poolStr;
    stringPool.Release(poolStr);
        
    // Record statistics
    stats.RecordLog(level, fullMsg.size());

    if (asyncLogging) {
        LogMessage logMsg;
        logMsg.level = level;
        logMsg.message = msg;
        logMsg.extra = extra;
        logMsg.ip = ip;
        logMsg.timestamp = timestamp;
        logMsg.fullText = fullMsg + "\r\n";

        // Try lock-free queue first
        if (!lockFreeQueue.TryPush(std::move(logMsg))) {
            // Queue full, record stat and process synchronously
            stats.queueFull.fetch_add(1, std::memory_order_relaxed);
            ProcessLogMessage(logMsg);
        }
    }
    else {
        // Modo síncrono (fallback)
        LogMessage logMsg;
        logMsg.level = level;
        logMsg.fullText = fullMsg + "\r\n";
        ProcessLogMessage(logMsg);
    }
}

// =======================
// Funções para habilitar/desabilitar níveis de log para arquivo
// =======================
void LogSystem::EnableFileLevel(LogLevel level) { 
    std::lock_guard<std::mutex> lock(logMutex);
    fileLevels.insert(level); 
}
    
void LogSystem::DisableFileLevel(LogLevel level) { 
    std::lock_guard<std::mutex> lock(logMutex);
    fileLevels.erase(level); 
}

// =======================
// Função para abrir o arquivo de log com base na data atual e índice de rotação
// =======================
void LogSystem::OpenLogFile() {
    currentDate = GetDate();
    std::string filename = logDir + "/server_" + currentDate + "_" + std::to_string(fileIndex) + ".log";

    if (logFile.is_open()) logFile.close();
    logFile.open(filename, std::ios::app);
        
    if (!logFile.is_open()) {
        // Tentar criar diretório se não existir
        fs::create_directories(logDir);
        logFile.open(filename, std::ios::app);
    }
}

// =======================
// Função para fazer a limpeza e compactação dos logs antigos podendo enviar para FTP
// =======================
void LogSystem::CleanupOldLogs() {
    try {
        auto now = std::chrono::system_clock::now();
        std::map<std::string, std::vector<std::string>> filesByDay;

        for (const auto& entry : fs::directory_iterator(logDir)) {
            if (!fs::is_regular_file(entry.path())) continue;

            auto name = entry.path().filename().string();
                
            // Corrigir regex para corresponder ao formato real: server_YYYY-MM-DD_X.log
            std::regex re(R"(server_(\d{4}-\d{2}-\d{2})_\d+\.log)");
            std::smatch match;
                
            if (std::regex_match(name, match, re)) {
                std::string dayStr = match[1].str();

                std::tm tm = {};
                int year, month, day;
                if (sscanf_s(dayStr.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
                    Warning("Falha ao parsear data do arquivo: " + name);
                    continue;
                }
                    
                tm.tm_year = year - 1900;
                tm.tm_mon = month - 1;
                tm.tm_mday = day;
                    
                auto fileDate = std::chrono::system_clock::from_time_t(std::mktime(&tm));
                auto ageDays = std::chrono::duration_cast<std::chrono::hours>(now - fileDate).count() / 24;

                if (ageDays >= retentionDays) {
                    filesByDay[dayStr].push_back(entry.path().string());
                }
            }
        }

        for (auto& [day, files] : filesByDay) {
            if (compressMode == "none") {
                // Apenas deletar arquivos antigos
                for (auto& f : files) {
                    try {
                        fs::remove(f);
                        Info("Arquivo excluído: " + f);
                    }
                    catch (const std::exception& e) {
                        Error("Falha ao excluir arquivo: " + f + " - " + e.what());
                    }
                }
            }
            else if (compressMode == "file") {
                // Compactar cada arquivo individualmente
                for (auto& f : files) {
                    if (CompressFile(f)) {
                        stats.compressionCount.fetch_add(1, std::memory_order_relaxed);
                        Info("Compactação concluída: " + f + ".zip");
                        try {
                            fs::remove(f);
                            Info("Arquivo excluído após compactação: " + f);
                        }
                        catch (const std::exception& e) {
                            Error("Falha ao excluir arquivo: " + f + " - " + e.what());
                        }

                        if (uploadBackup) {
                            std::string localZip = f + ".zip";
                            std::string remoteZip = ftpPath + fs::path(localZip).filename().string();
                            if (UploadToFTP(localZip, ftpServer, ftpUser, ftpPass, remoteZip)) {
                                Info("Upload concluído para FTP: " + remoteZip);
                            }
                            else {
                                Error("Falha no upload para FTP: " + remoteZip);
                            }
                        }
                    }
                }
            }
            else if (compressMode == "day") {
                // Compactar todos os arquivos do dia em um único ZIP
                Info("Iniciando compactação dos logs do dia " + day);

                if (CompressDayLogs(day, files)) {
                    stats.compressionCount.fetch_add(1, std::memory_order_relaxed);
                    Info("Compactação concluída: logpack_" + day + ".zip");

                    for (auto& f : files) {
                        try {
                            fs::remove(f);
                            Info("Arquivo excluído após compactação: " + f);
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
    }
    catch (const std::exception& e) {
        Error("Exceção em CleanupOldLogs: " + std::string(e.what()));
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
            ini << "# Exemplo: 1048576 = 1 MB, 5242880 = 5 MB\n";
            ini << "maxLogSize=1048576\n\n";

            ini << "# Modo de compressão dos logs antigos:\n";
            ini << "# none = não compacta, apenas deleta\n";
            ini << "# file = compacta cada arquivo individualmente\n";
            ini << "# day  = compacta todos os arquivos de um mesmo dia em um único .zip\n";
            ini << "compressMode=day\n\n";

            ini << "# Número máximo de linhas no RichEdit antes de limpar as antigas\n";
            ini << "maxRichEditLines=10000\n\n";

            ini << "# Habilitar logging assíncrono (recomendado)\n";
            ini << "asyncLogging=true\n\n";

            ini << "# Modo headless (sem GUI, apenas arquivo)\n";
            ini << "headlessMode=false\n\n";

            ini << "[Backup]\n";
            ini << "# Define se os arquivos compactados devem ser enviados para FTP\n";
            ini << "# true = habilita envio\n";
            ini << "# false = desabilita envio (padrão)\n";
            ini << "uploadBackup=false\n\n";

            ini << "# Endereço do servidor FTP\n";
            ini << "ftpServer=ftp.meuservidor.com\n\n";

            ini << "# Usuário para login no FTP\n";
            ini << "ftpUser=usuario\n\n";

            ini << "# Senha do usuário FTP (será criptografada automaticamente)\n";
            ini << "ftpPass=\n\n";

            ini << "# Pasta remota no servidor FTP onde os arquivos serão armazenhados\n";
            ini << "# Exemplo: /backup/\n";
            ini << "ftpPath=/backup/\n";

            ini.close();
            Info("Arquivo de configuração criado: " + configFile.string());
        }

        char tempbuffer[512];

        // Validação de retentionDays
        GetPrivateProfileString("Log", "retentionDays", "7", tempbuffer, 512, configFile.string().c_str());
        retentionDays = atoi(tempbuffer);
        if (retentionDays < 1) retentionDays = 1;
        if (retentionDays > 365) retentionDays = 365;

        // Validação de maxLogSize
        GetPrivateProfileString("Log", "maxLogSize", "1048576", tempbuffer, 512, configFile.string().c_str());
        maxLogSize = atoi(tempbuffer);
        if (maxLogSize < 102400) maxLogSize = 102400;  // Mínimo 100KB
        if (maxLogSize > 104857600) maxLogSize = 104857600;  // Máximo 100MB

        // Validação de compressMode
        GetPrivateProfileString("Log", "compressMode", "day", tempbuffer, 512, configFile.string().c_str());
        compressMode = tempbuffer;
        if (compressMode != "none" && compressMode != "file" && compressMode != "day") {
            Warning("compressMode inválido '" + compressMode + "', usando 'day'");
            compressMode = "day";
        }

        // maxRichEditLines
        GetPrivateProfileString("Log", "maxRichEditLines", "10000", tempbuffer, 512, configFile.string().c_str());
        maxRichEditLines = atoi(tempbuffer);
        if (maxRichEditLines < 100) maxRichEditLines = 100;
        if (maxRichEditLines > 100000) maxRichEditLines = 100000;

        // asyncLogging
        GetPrivateProfileString("Log", "asyncLogging", "true", tempbuffer, 512, configFile.string().c_str());
        asyncLogging = (_stricmp(tempbuffer, "true") == 0);

        // headlessMode
        GetPrivateProfileString("Log", "headlessMode", "false", tempbuffer, 512, configFile.string().c_str());
        headlessMode = (_stricmp(tempbuffer, "true") == 0);

        GetPrivateProfileString("Backup", "uploadBackup", "false", tempbuffer, 512, configFile.string().c_str());
        uploadBackup = (_stricmp(tempbuffer, "true") == 0);

        GetPrivateProfileString("Backup", "ftpServer", "", tempbuffer, 512, configFile.string().c_str());
        ftpServer = tempbuffer;

        GetPrivateProfileString("Backup", "ftpUser", "", tempbuffer, 512, configFile.string().c_str());
        ftpUser = tempbuffer;

        GetPrivateProfileString("Backup", "ftpPass", "", tempbuffer, 512, configFile.string().c_str());
        std::string encryptedPass = tempbuffer;
            
        // Descriptografar senha se não estiver vazia
        if (!encryptedPass.empty()) {
            ftpPass = DecryptPassword(encryptedPass);
        }

        GetPrivateProfileString("Backup", "ftpPath", "/", tempbuffer, 512, configFile.string().c_str());
        ftpPath = tempbuffer;
            
        // Garantir que ftpPath termina com /
        if (!ftpPath.empty() && ftpPath.back() != '/') {
            ftpPath += '/';
        }

        Info("Configurações carregadas: retention=" + std::to_string(retentionDays) + 
                "d, maxSize=" + std::to_string(maxLogSize) + 
                "b, compress=" + compressMode);
    }
    catch (const std::exception& e) {
        Error(std::string("Exceção ao carregar configuração: ") + e.what());
        // Valores padrão seguros
        retentionDays = 7;
        maxLogSize = 1048576;
        compressMode = "day";
        uploadBackup = false;
        maxRichEditLines = 10000;
        asyncLogging = true;
        headlessMode = false;
    }
}

// =======================
// Criptografia de senha usando Windows DPAPI
// =======================
std::string LogSystem::EncryptPassword(const std::string& password) {
    if (password.empty()) return "";

    DATA_BLOB dataIn;
    DATA_BLOB dataOut;
        
    dataIn.pbData = (BYTE*)password.data();
    dataIn.cbData = (DWORD)password.size();

    if (CryptProtectData(&dataIn, L"LogSystemFTP", nullptr, nullptr, nullptr, 0, &dataOut)) {
        std::string encrypted((char*)dataOut.pbData, dataOut.cbData);
        LocalFree(dataOut.pbData);
            
        // Converter para hex para armazenar no INI
        std::stringstream ss;
        for (unsigned char c : encrypted) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }
        return ss.str();
    }
        
    Error("Falha ao criptografar senha FTP");
    return "";
}

std::string LogSystem::DecryptPassword(const std::string& encrypted) {
    if (encrypted.empty()) return "";

    try {
        // Converter de hex
        std::vector<unsigned char> bytes;
        for (size_t i = 0; i < encrypted.length(); i += 2) {
            std::string byteStr = encrypted.substr(i, 2);
            bytes.push_back((unsigned char)std::stoul(byteStr, nullptr, 16));
        }

        DATA_BLOB dataIn;
        DATA_BLOB dataOut;
            
        dataIn.pbData = bytes.data();
        dataIn.cbData = (DWORD)bytes.size();

        if (CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut)) {
            std::string decrypted((char*)dataOut.pbData, dataOut.cbData);
            LocalFree(dataOut.pbData);
            return decrypted;
        }
    }
    catch (...) {
        Error("Falha ao descriptografar senha FTP");
    }
        
    return "";
}

// =======================
// Função para obter a cor associada a cada nível de log
// =======================
COLORREF LogSystem::GetColor(LogLevel level) {
    return levelColors[(int)level];
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
    case LogLevel::Quest:   return "[QUEST]";
    case LogLevel::Packets: return "[PACKETS]";
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
// Função para adicionar texto colorido ao controle RichEdit
// =======================
void LogSystem::AppendColoredText(HWND target, const std::string& text, COLORREF textColor) {
    if (!target || !IsWindow(target)) {
        return;
    }

    CHARRANGE cr{ -1, -1 };
    SendMessageA(target, EM_EXSETSEL, 0, (LPARAM)&cr);

    CHARFORMAT2A cf{};
    cf.cbSize = sizeof(CHARFORMAT2A);
    cf.dwMask = CFM_COLOR | CFM_BACKCOLOR | CFM_FACE | CFM_SIZE;
    cf.crTextColor = textColor;
    cf.crBackColor = RGB(20, 20, 20);
    strcpy_s(cf.szFaceName, "Consolas");
    cf.yHeight = 200;

    SendMessageA(target, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageA(target, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());

    SendMessageA(target, EM_SETSEL, -1, -1);
    SendMessageA(target, EM_SCROLLCARET, 0, 0);
}

// =======================
// Função para escrever os logs em arquivo com rotação baseada em tamanho - otimizado
// =======================
void LogSystem::WriteToFile(const std::string& text) {
    if (!logFile.is_open()) {
        OpenLogFile();
    }

    std::error_code ec;
    std::string filename = logDir + "/server_" + currentDate + "_" + std::to_string(fileIndex) + ".log";
    auto size = fs::file_size(filename, ec);

    if (!ec && size >= maxLogSize) {
        fileIndex++;
        stats.filesRotated.fetch_add(1, std::memory_order_relaxed);
        OpenLogFile();
    }

    if (logFile.is_open()) {
        logFile << text << "\n";  // Usar \n ao invés de std::endl
            
        // Flush periódico usando atomic counter (thread-safe)
        int currentCount = flushCounter.fetch_add(1, std::memory_order_relaxed);
        if (currentCount >= 10) {
            logFile.flush();
            flushCounter.store(0, std::memory_order_relaxed);
        }
    }
}

// =======================
// Função para compactar um único arquivo
// =======================
bool LogSystem::CompressFile(const std::string& file) {
    try {
        std::string zipName = file + ".zip";

        zipFile zf = zipOpen(zipName.c_str(), APPEND_STATUS_CREATE);
        if (!zf) {
            Error("Falha ao criar ZIP: " + zipName);
            return false;
        }

        zip_fileinfo zi = {};

        if (zipOpenNewFileInZip(zf, fs::path(file).filename().string().c_str(), &zi,
            nullptr, 0, nullptr, 0, nullptr,
            Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK) {
            Error("Falha ao adicionar arquivo ao ZIP: " + file);
            zipClose(zf, nullptr);
            return false;
        }

        std::ifstream in(file, std::ios::binary);
        if (!in.is_open()) {
            Error("Não foi possível abrir arquivo para compactação: " + file);
            zipCloseFileInZip(zf);
            zipClose(zf, nullptr);
            return false;
        }

        const size_t chunkSize = 1024 * 1024;
        std::vector<char> buffer(chunkSize);
        while (in.read(buffer.data(), buffer.size()) || in.gcount() > 0) {
            zipWriteInFileInZip(zf, buffer.data(), static_cast<unsigned int>(in.gcount()));
        }

        zipCloseFileInZip(zf);
        zipClose(zf, nullptr);
            
        return true;
    }
    catch (const std::exception& e) {
        Error("Exceção em CompressFile: " + std::string(e.what()));
        return false;
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
                zipCloseFileInZip(zf);
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
        return true;
    }
    catch (const std::exception& e) {
        Error("Exceção em CompressDayLogs: " + std::string(e.what()));
        return false;
    }
}

// =======================
// Função para upload dos logs antigos via FTP
// =======================
bool LogSystem::UploadToFTP(const std::string& localFile, const std::string& server,
    const std::string& user, const std::string& pass, const std::string& remotePath) {
        
    HINTERNET hInternet = nullptr;
    HINTERNET hFtp = nullptr;
    bool result = false;

    try {
        hInternet = InternetOpenA("LogUploader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            Error("Falha ao inicializar InternetOpen");
            return false;
        }

        hFtp = InternetConnectA(hInternet, server.c_str(), INTERNET_DEFAULT_FTP_PORT,
            user.c_str(), pass.c_str(), INTERNET_SERVICE_FTP, 0, 0);
            
        if (!hFtp) {
            Error("Falha ao conectar ao servidor FTP: " + server);
            InternetCloseHandle(hInternet);
            return false;
        }

        result = FtpPutFileA(hFtp, localFile.c_str(), remotePath.c_str(), FTP_TRANSFER_TYPE_BINARY, 0);
            
        if (result) {
            Info("Upload concluído: " + remotePath);
        }
        else {
            DWORD error = GetLastError();
            Error("Falha no upload para FTP: " + remotePath + " (erro: " + std::to_string(error) + ")");
        }
    }
    catch (const std::exception& e) {
        Error("Exceção em UploadToFTP: " + std::string(e.what()));
        result = false;
    }

    // Cleanup
    if (hFtp) InternetCloseHandle(hFtp);
    if (hInternet) InternetCloseHandle(hInternet);

    return result;
}
