# API Reference - LogSystem MB

## ?? Índice

- [Visão Geral](#visão-geral)
- [Classes Principais](#classes-principais)
  - [LogSystem](#logsystem)
  - [LogManager](#logmanager)
- [Enumerations](#enumerations)
- [Structures](#structures)
- [Funções Públicas](#funções-públicas)
- [Exemplos Detalhados](#exemplos-detalhados)
- [Best Practices](#best-practices)

---

## Visão Geral

O LogSystem MB fornece uma API C++20 moderna e thread-safe para logging de alto desempenho. Todos os métodos são não-bloqueantes quando em modo assíncrono.

### Quick Start

```cpp
#include "LogSystem.h"

int main() {
    // Configurar níveis para arquivo
    pLog.EnableFileLevel(LogLevel::Info);
    pLog.EnableFileLevel(LogLevel::Error);
    
    // Logar mensagens
    pLog.Info("Aplicação iniciada");
    pLog.Error("Erro crítico", "Detalhes do erro");
    
    // Shutdown graceful
    pLog.Shutdown();
    return 0;
}
```

---

## Classes Principais

### LogSystem

Classe principal responsável por todo o sistema de logging.

#### Métodos Públicos

##### `void Initialize()`

Inicializa o sistema de logging.

**Descrição:**
- Carrega configurações do INI
- Abre arquivo de log inicial
- Executa cleanup de logs antigos

**Exemplo:**
```cpp
pLog.Initialize();
```

**Notas:**
- Chamado automaticamente no construtor
- Safe para chamar múltiplas vezes
- Thread-safe

---

##### `void SetTarget(TargetSide side, HWND editHandle)`

Define o handle do RichEdit para output visual.

**Parâmetros:**
- `side`: `TargetSide::Left` ou `TargetSide::Right`
- `editHandle`: Handle do controle RichEdit

**Descrição:**
Configura onde os logs serão exibidos visualmente. Logs gerais vão para `Left`, pacotes de rede para `Right`.

**Exemplo:**
```cpp
HWND hEditLog = CreateWindowExA(...);
pLog.SetTarget(TargetSide::Left, hEditLog);
```

**Exceções:**
- Loga warning se `editHandle` for NULL
- Safe para chamar antes da janela estar pronta

**Thread Safety:** ? Thread-safe (usa `targetMutex`)

---

##### `void Log(LogLevel level, const std::string& msg, const std::string& extra = "", unsigned int ip = 0)`

Método genérico de logging.

**Parâmetros:**
- `level`: Nível do log (Trace, Debug, Info, Warning, Error, Quest, Packets)
- `msg`: Mensagem principal
- `extra`: Contexto adicional (opcional)
- `ip`: Endereço IP em formato uint32 (opcional)

**Descrição:**
Registra uma mensagem de log com nível, timestamp e informações opcionais.

**Formato de saída:**
```
[2024-01-15 10:30:45] [INFO] Mensagem principal [Extra info] [IP:192.168.1.100]
```

**Exemplo:**
```cpp
// Simples
pLog.Log(LogLevel::Info, "Servidor iniciado");

// Com contexto
pLog.Log(LogLevel::Warning, "Memória alta", "80% utilizada");

// Com IP
unsigned int ip = (192 << 24) | (168 << 16) | (1 << 8) | 100;
pLog.Log(LogLevel::Error, "Falha na conexão", "Timeout", ip);
```

**Performance:**
- Async mode: ~2-5 ?s
- Sync mode: ~50-100 ?s

**Thread Safety:** ? Thread-safe

---

##### Métodos de Conveniência

Shortcuts para `Log()` com nível específico.

```cpp
void Trace(const std::string& msg, const std::string& extra = "", unsigned int ip = 0);
void Debug(const std::string& msg, const std::string& extra = "", unsigned int ip = 0);
void Info(const std::string& msg, const std::string& extra = "", unsigned int ip = 0);
void Warning(const std::string& msg, const std::string& extra = "", unsigned int ip = 0);
void Error(const std::string& msg, const std::string& extra = "", unsigned int ip = 0);
void Quest(const std::string& msg, const std::string& extra = "", unsigned int ip = 0);
void Packets(const std::string& msg, const std::string& extra = "", unsigned int ip = 0);
```

**Exemplos:**
```cpp
pLog.Trace("Função iniciada");
pLog.Debug("Valor da variável: " + std::to_string(x));
pLog.Info("Operação concluída");
pLog.Warning("Recurso baixo", "RAM: 90%");
pLog.Error("Falha crítica", "Disco cheio");
pLog.Quest("Quest completada", "QuestID: 1001");
pLog.Packets("Pacote recebido", "Size: 128 bytes");
```

**Cores no RichEdit:**
- `Trace`: Cinza médio
- `Debug`: Azul claro/ciano
- `Info`: Verde
- `Warning`: Amarelo
- `Error`: Vermelho
- `Quest`: Rosa
- `Packets`: Laranja

---

##### `void EnableFileLevel(LogLevel level)`

Habilita gravação de um nível específico em arquivo.

**Parâmetros:**
- `level`: Nível a habilitar

**Descrição:**
Por padrão, nenhum nível é gravado em arquivo. Use este método para escolher quais níveis persistir.

**Exemplo:**
```cpp
// Gravar apenas logs importantes
pLog.EnableFileLevel(LogLevel::Warning);
pLog.EnableFileLevel(LogLevel::Error);

// Gravar tudo (debug)
pLog.EnableFileLevel(LogLevel::Trace);
pLog.EnableFileLevel(LogLevel::Debug);
pLog.EnableFileLevel(LogLevel::Info);
pLog.EnableFileLevel(LogLevel::Warning);
pLog.EnableFileLevel(LogLevel::Error);
pLog.EnableFileLevel(LogLevel::Quest);
pLog.EnableFileLevel(LogLevel::Packets);
```

**Best Practice:**
```cpp
#ifdef _DEBUG
    pLog.EnableFileLevel(LogLevel::Trace);
    pLog.EnableFileLevel(LogLevel::Debug);
#endif
pLog.EnableFileLevel(LogLevel::Info);
pLog.EnableFileLevel(LogLevel::Warning);
pLog.EnableFileLevel(LogLevel::Error);
```

**Thread Safety:** ? Thread-safe (usa `logMutex`)

---

##### `void DisableFileLevel(LogLevel level)`

Desabilita gravação de um nível específico em arquivo.

**Parâmetros:**
- `level`: Nível a desabilitar

**Exemplo:**
```cpp
// Parar de gravar logs de debug em produção
pLog.DisableFileLevel(LogLevel::Trace);
pLog.DisableFileLevel(LogLevel::Debug);
```

**Thread Safety:** ? Thread-safe (usa `logMutex`)

---

##### `void CleanupOldLogs()`

Executa limpeza de logs antigos baseado em `retentionDays`.

**Descrição:**
- Busca arquivos de log com idade >= `retentionDays`
- Compacta em ZIP (se `compressMode` != "none")
- Faz upload para FTP (se `uploadBackup` = true)
- Deleta arquivos originais

**Exemplo:**
```cpp
// Forçar limpeza manual
pLog.CleanupOldLogs();
```

**Notas:**
- Chamado automaticamente em `Initialize()`
- Chamado automaticamente ao rotacionar data
- Operação pode demorar (compressão + FTP)

**Thread Safety:** ?? Não chamar concorrentemente

---

##### `void LoadConfig(const std::string& filename)`

Carrega configurações do arquivo INI.

**Parâmetros:**
- `filename`: Nome do arquivo (ex: "logconfig.ini")

**Descrição:**
Lê configurações do diretório `Config/`. Se o arquivo não existir, cria um com valores padrão.

**Exemplo:**
```cpp
pLog.LoadConfig("logconfig.ini");
```

**Configurações carregadas:**
```ini
[Log]
retentionDays=7
maxLogSize=1048576
compressMode=day
maxRichEditLines=10000
asyncLogging=true
headlessMode=false

[Backup]
uploadBackup=false
ftpServer=ftp.exemplo.com
ftpUser=usuario
ftpPass=senha_criptografada
ftpPath=/backup/
```

**Validação:**
- `retentionDays`: 1-365
- `maxLogSize`: 100KB-100MB
- `compressMode`: "none", "file", "day"
- `maxRichEditLines`: 100-100,000

**Thread Safety:** ?? Chamar apenas no início

---

##### `void SetMaxRichEditLines(int maxLines)`

Define número máximo de linhas no RichEdit.

**Parâmetros:**
- `maxLines`: Máximo de linhas (100-100,000)

**Descrição:**
Quando o RichEdit excede este limite, as linhas mais antigas são deletadas automaticamente.

**Exemplo:**
```cpp
pLog.SetMaxRichEditLines(5000); // Manter apenas 5000 linhas
```

**Notas:**
- Valores fora do range são clampados
- Padrão: 10,000 linhas
- Previne lentidão do RichEdit

**Thread Safety:** ? Thread-safe (usa `targetMutex`)

---

##### `void ClearRichEdit(TargetSide side)`

Limpa todo o conteúdo de um RichEdit.

**Parâmetros:**
- `side`: `TargetSide::Left` ou `TargetSide::Right`

**Descrição:**
Remove todas as linhas do RichEdit especificado.

**Exemplo:**
```cpp
// Limpar painel principal
pLog.ClearRichEdit(TargetSide::Left);

// Limpar painel de pacotes
pLog.ClearRichEdit(TargetSide::Right);
```

**Notas:**
- Safe se handle for NULL
- Define readonly = false temporariamente

**Thread Safety:** ? Thread-safe (usa `targetMutex`)

---

##### `template<typename... Args> std::string Format(const char* fmt, Args&&... args)`

Formata string usando C++20 `std::format`.

**Parâmetros:**
- `fmt`: String de formato
- `args`: Argumentos variádicos

**Retorno:** String formatada

**Exemplo:**
```cpp
std::string msg = pLog.Format("Player {} alcançou level {}", playerName, level);
pLog.Info(msg);

// Ou inline
pLog.Info(pLog.Format("Conexão de {} na porta {}", ip, port));
```

**Sintaxe do formato:**
```cpp
{}          // Placeholder automático
{0}         // Argumento por índice
{:.2f}      // Float com 2 decimais
{:04d}      // Int com padding zero (0042)
{:>10}      // Alinhado à direita
```

**Tipos suportados:**
- Integrais: `int`, `long`, `uint64_t`, etc.
- Float: `float`, `double`
- String: `std::string`, `const char*`
- Ponteiros: `void*`
- Custom types com `operator<<`

**Performance:** O(n) onde n = tamanho da string resultante

---

##### `void Shutdown()`

Para o worker thread gracefully e flush logs pendentes.

**Descrição:**
- Sinaliza `stopWorker = true`
- Aguarda worker thread terminar
- Processa mensagens restantes na fila
- Fecha arquivo de log

**Exemplo:**
```cpp
int main() {
    pLog.Info("App starting");
    
    // ... código ...
    
    pLog.Info("App closing");
    pLog.Shutdown();  // ?? IMPORTANTE antes de exit
    
    return 0;
}
```

**Notas:**
- **SEMPRE chamar antes de sair da aplicação**
- Previne perda de logs
- Safe para chamar múltiplas vezes

**Thread Safety:** ? Thread-safe

---

##### Performance Queries

```cpp
const PerformanceStats& GetStats() const;
bool IsAsyncEnabled() const;
bool IsHeadless() const;
std::string GetCompressMode() const;
```

**Exemplos:**
```cpp
const auto& stats = pLog.GetStats();
std::cout << "Total logs: " << stats.totalLogs.load() << "\n";
std::cout << "Uptime: " << stats.GetUptime() << " seconds\n";
std::cout << "Throughput: " << stats.GetLogsPerSecond() << " msg/s\n";

if (pLog.IsAsyncEnabled()) {
    std::cout << "Async mode active\n";
}

std::cout << "Compress mode: " << pLog.GetCompressMode() << "\n";
```

---

### LogManager

Singleton que gerencia a instância global do `LogSystem`.

#### Métodos Públicos

##### `static LogManager& Instance()`

Retorna a instância única do `LogManager`.

**Retorno:** Referência para o LogManager

**Exemplo:**
```cpp
auto& manager = LogManager::Instance();
auto& log = manager.GetLogInst();
log.Info("Mensagem");
```

**Notas:**
- Thread-safe (Meyer's Singleton)
- Inicialização lazy
- Destruição automática

---

##### `LogSystem& GetLogInst()`

Retorna a instância do `LogSystem`.

**Retorno:** Referência para o LogSystem

**Exemplo:**
```cpp
LogManager::Instance().GetLogInst().Info("Mensagem");
```

---

## Enumerations

### LogLevel

```cpp
enum class LogLevel {
    Trace,      // Debug detalhado
    Debug,      // Informações de debug
    Info,       // Informações gerais
    Warning,    // Avisos
    Error,      // Erros
    Quest,      // Sistema de quests (custom)
    Packets     // Tráfego de rede (custom)
};
```

**Uso:**
```cpp
pLog.Log(LogLevel::Info, "Mensagem");
```

---

### TargetSide

```cpp
enum class TargetSide {
    Left = 0,   // Painel esquerdo (logs gerais)
    Right = 1   // Painel direito (packets)
};
```

**Uso:**
```cpp
pLog.SetTarget(TargetSide::Left, hEditLog);
pLog.ClearRichEdit(TargetSide::Right);
```

---

## Structures

### PerformanceStats

Estatísticas de performance em tempo real.

```cpp
struct PerformanceStats {
    std::atomic<uint64_t> totalLogs;           // Total de logs
    std::atomic<uint64_t> logsPerLevel[7];     // Por nível
    std::atomic<uint64_t> bytesWritten;        // Bytes gravados
    std::atomic<uint64_t> filesRotated;        // Arquivos rotacionados
    std::atomic<uint64_t> compressionCount;    // Compressões realizadas
    std::atomic<uint64_t> queueFull;           // Eventos de fila cheia
    std::atomic<uint64_t> queuePeak;           // Pico da fila
    
    double GetUptime() const;
    double GetLogsPerSecond() const;
};
```

**Exemplo:**
```cpp
const auto& stats = pLog.GetStats();

std::cout << "=== Statistics ===\n";
std::cout << "Total: " << stats.totalLogs.load() << "\n";
std::cout << "Info logs: " << stats.logsPerLevel[(int)LogLevel::Info].load() << "\n";
std::cout << "Errors: " << stats.logsPerLevel[(int)LogLevel::Error].load() << "\n";
std::cout << "Bytes: " << stats.bytesWritten.load() << "\n";
std::cout << "Uptime: " << stats.GetUptime() << "s\n";
std::cout << "Rate: " << stats.GetLogsPerSecond() << " msg/s\n";
```

---

## Exemplos Detalhados

### Exemplo 1: Servidor HTTP

```cpp
class HttpServer {
private:
    void HandleRequest(const std::string& method, const std::string& path, unsigned int clientIP) {
        pLog.Info("HTTP Request", 
                  pLog.Format("{} {}", method, path), 
                  clientIP);
        
        try {
            auto response = ProcessRequest(method, path);
            pLog.Debug("Response sent", 
                       pLog.Format("Status: {}, Size: {}", response.status, response.size));
        }
        catch (const std::exception& e) {
            pLog.Error("Request failed", e.what(), clientIP);
        }
    }
};
```

### Exemplo 2: Game Server

```cpp
class GameServer {
private:
    void OnPlayerLogin(int playerID, const std::string& username, unsigned int ip) {
        pLog.Info("Player login", 
                  pLog.Format("ID: {}, Name: {}", playerID, username), 
                  ip);
        
        pLog.Quest("Welcome quest started", 
                   pLog.Format("Player: {}", username), 
                   ip);
    }
    
    void OnPacketReceived(int packetType, size_t size, unsigned int ip) {
        pLog.Packets("Packet received", 
                     pLog.Format("Type: 0x{:04X}, Size: {} bytes", packetType, size), 
                     ip);
    }
};
```

### Exemplo 3: Sistema de Monitoramento

```cpp
class SystemMonitor {
public:
    void MonitorResources() {
        while (running) {
            auto cpuUsage = GetCPUUsage();
            auto memUsage = GetMemoryUsage();
            
            if (cpuUsage > 80.0) {
                pLog.Warning("High CPU usage", 
                            pLog.Format("{:.1f}%", cpuUsage));
            }
            
            if (memUsage > 90.0) {
                pLog.Error("Critical memory usage", 
                          pLog.Format("{:.1f}%", memUsage));
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
};
```

---

## Best Practices

### ? DO's

```cpp
// 1. Shutdown gracefully
int main() {
    // ...
    pLog.Shutdown();
    return 0;
}

// 2. Use níveis apropriados
pLog.Trace("Entering function calculateSum()");      // Detalhe extremo
pLog.Debug("Variable x = " + std::to_string(x));     // Debug
pLog.Info("User logged in");                         // Info geral
pLog.Warning("Disk space low");                      // Aviso
pLog.Error("Database connection failed");            // Erro

// 3. Adicione contexto
pLog.Error("Connection failed", 
           pLog.Format("Host: {}, Port: {}, Error: {}", host, port, error));

// 4. Use Format() para tipos
pLog.Info(pLog.Format("Processing {} items in {:.2f}s", count, elapsed));

// 5. Configure níveis para arquivo
#ifdef _DEBUG
    pLog.EnableFileLevel(LogLevel::Debug);
#endif
pLog.EnableFileLevel(LogLevel::Info);
pLog.EnableFileLevel(LogLevel::Warning);
pLog.EnableFileLevel(LogLevel::Error);
```

### ? DON'Ts

```cpp
// 1. Não logue senhas ou dados sensíveis
pLog.Info("Login", "Password: " + password);  // ? NUNCA!

// 2. Não logue em loops apertados
for (int i = 0; i < 1000000; ++i) {
    pLog.Info("Processing " + std::to_string(i));  // ? Muito overhead
}

// 3. Não ignore thread safety
static std::string buffer;  // ? Shared state sem proteção
pLog.Info(buffer);

// 4. Não faça I/O pesado em callbacks de log
// (O sistema já é async, não adicione mais I/O)

// 5. Não esqueça de fazer Shutdown()
// ? Logs podem ser perdidos!
```

---

**Versão:** 2.0  
**Última atualização:** 2024-01-15  
**Autor:** Guilherme Candiotto
