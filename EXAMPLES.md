# Exemplos de Uso - LogSystem MB

Este documento contém exemplos práticos de uso do LogSystem.

## ?? Exemplo 1: Aplicação Console Simples

```cpp
#include "LogSystem.h"
#include <iostream>

int main() {
    // Configurar modo headless (sem GUI)
    // Edite Config/logconfig.ini e defina: headlessMode=true
    
    // Habilitar níveis de log para arquivo
    pLog.EnableFileLevel(LogLevel::Info);
    pLog.EnableFileLevel(LogLevel::Warning);
    pLog.EnableFileLevel(LogLevel::Error);
    
    // Usar logs
    pLog.Info("Aplicação iniciada");
    
    // Simular processamento
    for (int i = 0; i < 10; i++) {
        pLog.Info("Processando item", "ID: " + std::to_string(i));
        
        if (i == 5) {
            pLog.Warning("Item suspeito detectado", "ID: 5");
        }
    }
    
    pLog.Info("Processamento concluído");
    pLog.Shutdown();
    
    return 0;
}
```

## ?? Exemplo 2: Servidor de Rede

```cpp
#include "LogSystem.h"
#include <thread>
#include <vector>

// Converter IP string para uint32
unsigned int StringToIP(const std::string& ipStr) {
    unsigned int a, b, c, d;
    char dot;
    std::stringstream ss(ipStr);
    ss >> a >> dot >> b >> dot >> c >> dot >> d;
    return (a << 24) | (b << 16) | (c << 8) | d;
}

void HandleClient(int clientId, const std::string& clientIP) {
    unsigned int ip = StringToIP(clientIP);
    
    pLog.Info("Cliente conectado", "ID: " + std::to_string(clientId), ip);
    
    // Simular tráfego de rede
    for (int i = 0; i < 5; i++) {
        pLog.Packets("Pacote recebido", 
                     "Tipo: DATA, Tamanho: " + std::to_string(128 + i), 
                     ip);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    pLog.Info("Cliente desconectado", "ID: " + std::to_string(clientId), ip);
}

int main() {
    pLog.EnableFileLevel(LogLevel::Info);
    pLog.EnableFileLevel(LogLevel::Packets);
    pLog.EnableFileLevel(LogLevel::Error);
    
    pLog.Info("Servidor de rede iniciado");
    
    // Simular múltiplos clientes (thread-safe!)
    std::vector<std::thread> threads;
    
    threads.emplace_back(HandleClient, 1, "192.168.1.10");
    threads.emplace_back(HandleClient, 2, "192.168.1.20");
    threads.emplace_back(HandleClient, 3, "192.168.1.30");
    
    for (auto& t : threads) {
        t.join();
    }
    
    pLog.Info("Servidor finalizado");
    pLog.Shutdown();
    
    return 0;
}
```

## ?? Exemplo 3: Sistema de Quests (MMORPG)

```cpp
#include "LogSystem.h"

struct Player {
    int id;
    std::string name;
    std::string ip;
};

void StartQuest(const Player& player, int questId) {
    unsigned int ip = StringToIP(player.ip);
    
    pLog.Quest("Quest iniciada", 
               pLog.Format("Player: %s, QuestID: %d", player.name.c_str(), questId),
               ip);
}

void CompleteQuest(const Player& player, int questId, int reward) {
    unsigned int ip = StringToIP(player.ip);
    
    pLog.Quest("Quest completada",
               pLog.Format("Player: %s, QuestID: %d, Reward: %d gold", 
                          player.name.c_str(), questId, reward),
               ip);
}

void FailQuest(const Player& player, int questId, const std::string& reason) {
    unsigned int ip = StringToIP(player.ip);
    
    pLog.Warning("Quest falhou",
                 pLog.Format("Player: %s, QuestID: %d, Razão: %s",
                            player.name.c_str(), questId, reason.c_str()),
                 ip);
}

int main() {
    pLog.EnableFileLevel(LogLevel::Quest);
    pLog.EnableFileLevel(LogLevel::Warning);
    
    Player player1 = {1, "João", "192.168.1.100"};
    Player player2 = {2, "Maria", "192.168.1.101"};
    
    StartQuest(player1, 1001);
    StartQuest(player2, 1002);
    
    CompleteQuest(player1, 1001, 500);
    FailQuest(player2, 1002, "Timeout expirado");
    
    pLog.Shutdown();
    return 0;
}
```

## ?? Exemplo 4: Debug Avançado com Trace

```cpp
#include "LogSystem.h"

void ComplexFunction(int param1, const std::string& param2) {
    pLog.Trace("ComplexFunction iniciada", 
               pLog.Format("param1=%d, param2=%s", param1, param2.c_str()));
    
    // Passo 1
    pLog.Trace("Passo 1: Validação");
    if (param1 < 0) {
        pLog.Error("Parâmetro inválido", "param1 deve ser >= 0");
        return;
    }
    
    // Passo 2
    pLog.Trace("Passo 2: Processamento");
    // ... processamento ...
    
    // Passo 3
    pLog.Trace("Passo 3: Finalização");
    pLog.Info("ComplexFunction concluída com sucesso");
}

int main() {
    // Habilitar apenas durante debug
    pLog.EnableFileLevel(LogLevel::Trace);
    pLog.EnableFileLevel(LogLevel::Debug);
    pLog.EnableFileLevel(LogLevel::Info);
    pLog.EnableFileLevel(LogLevel::Error);
    
    pLog.Debug("Modo debug ativado");
    
    ComplexFunction(42, "teste");
    ComplexFunction(-1, "erro");
    
    pLog.Shutdown();
    return 0;
}
```

## ?? Exemplo 5: Monitoramento de Performance

```cpp
#include "LogSystem.h"
#include <chrono>

class PerformanceTimer {
    std::chrono::steady_clock::time_point start;
    std::string operation;
    
public:
    PerformanceTimer(const std::string& op) : operation(op) {
        start = std::chrono::steady_clock::now();
        pLog.Debug("Iniciando", operation);
    }
    
    ~PerformanceTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        if (duration.count() > 1000) {
            pLog.Warning("Operação lenta", 
                        pLog.Format("%s levou %lld ms", operation.c_str(), duration.count()));
        } else {
            pLog.Debug("Operação concluída",
                      pLog.Format("%s em %lld ms", operation.c_str(), duration.count()));
        }
    }
};

void SlowOperation() {
    PerformanceTimer timer("SlowOperation");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

void FastOperation() {
    PerformanceTimer timer("FastOperation");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main() {
    pLog.EnableFileLevel(LogLevel::Debug);
    pLog.EnableFileLevel(LogLevel::Warning);
    
    FastOperation();
    SlowOperation();
    
    pLog.Shutdown();
    return 0;
}
```

## ?? Exemplo 6: Gerenciamento Manual de Rotação

```cpp
#include "LogSystem.h"

int main() {
    pLog.EnableFileLevel(LogLevel::Info);
    
    pLog.Info("Gerando muitos logs para testar rotação...");
    
    // Gerar logs grandes para forçar rotação
    for (int i = 0; i < 10000; i++) {
        pLog.Info("Log de teste número " + std::to_string(i),
                  "Conteúdo adicional para aumentar tamanho do log. " 
                  "Esta é uma linha longa para simular logs reais de produção.");
        
        if (i % 1000 == 0) {
            pLog.Info("Progresso", std::to_string(i) + " logs gerados");
        }
    }
    
    pLog.Info("Geração de logs concluída");
    
    // Forçar limpeza de logs antigos (se configurado)
    pLog.CleanupOldLogs();
    
    pLog.Shutdown();
    return 0;
}
```

## ?? Exemplo 7: GUI com Botões Customizados

```cpp
// Adicionar em main.cpp após criar a janela

case WM_COMMAND: {
    switch (LOWORD(wParam)) {
        case ID_BUTTON_EXPORT: {
            // Exportar logs para arquivo específico
            pLog.Info("Exportando logs...");
            
            // Implementar lógica de export
            std::ofstream export_file("export_" + pLog.GetDate() + ".txt");
            // ... copiar conteúdo do RichEdit ...
            
            pLog.Info("Logs exportados com sucesso");
            MessageBoxA(hWnd, "Logs exportados!", "Sucesso", MB_OK);
            break;
        }
        
        case ID_BUTTON_FILTER: {
            // Filtrar apenas erros
            pLog.ClearRichEdit(TargetSide::Left);
            pLog.Info("Mostrando apenas erros...");
            
            // Aqui você implementaria a lógica de filtro
            // lendo do arquivo e mostrando apenas ERROR
            
            break;
        }
        
        case ID_BUTTON_STATS: {
            // Mostrar estatísticas
            int errorCount = 0;
            int warningCount = 0;
            int infoCount = 0;
            
            // Contar logs (implementar leitura do arquivo)
            
            std::string stats = pLog.Format(
                "Estatísticas:\nErros: %d\nAvisos: %d\nInfo: %d",
                errorCount, warningCount, infoCount
            );
            
            MessageBoxA(hWnd, stats.c_str(), "Estatísticas", MB_OK);
            break;
        }
    }
    break;
}
```

## ?? Exemplo 8: Configuração de FTP

```cpp
#include "LogSystem.h"

int main() {
    // Primeira execução: configurar FTP
    // 1. Edite Config/logconfig.ini:
    /*
    [Backup]
    uploadBackup=true
    ftpServer=ftp.meuservidor.com
    ftpUser=meususuario
    ftpPass=minhasenha123  # Será criptografada automaticamente!
    ftpPath=/backups/logs/
    */
    
    // 2. Execute o programa
    pLog.Info("Testando configuração FTP...");
    
    // O LogSystem irá:
    // - Detectar senha em texto plano
    // - Criptografar usando DPAPI
    // - Salvar versão criptografada no INI
    
    // 3. Logs antigos serão automaticamente enviados para FTP
    pLog.CleanupOldLogs();
    
    pLog.Info("Configuração FTP concluída");
    pLog.Shutdown();
    
    return 0;
}
```

## ?? Dicas de Uso

### 1. Níveis de Log por Ambiente

```cpp
#ifdef _DEBUG
    // Modo Debug: tudo
    pLog.EnableFileLevel(LogLevel::Trace);
    pLog.EnableFileLevel(LogLevel::Debug);
#endif
    // Sempre: Info, Warning, Error
    pLog.EnableFileLevel(LogLevel::Info);
    pLog.EnableFileLevel(LogLevel::Warning);
    pLog.EnableFileLevel(LogLevel::Error);
```

### 2. Formatação Avançada

```cpp
// Usar std::format (C++20)
pLog.Info(pLog.Format("Player {}: Level {} -> {}", 
                      playerName, oldLevel, newLevel));

// Com tipos diferentes
pLog.Info(pLog.Format("Conexão: {}:{} (timeout: {}ms)",
                      serverIP, port, timeout));
```

### 3. Contexto Adicional

```cpp
// Sempre adicione contexto relevante
pLog.Error("Falha na conexão", 
           pLog.Format("Host: %s, Port: %d, Erro: %s",
                      host.c_str(), port, strerror(errno)));
```

### 4. IPs em Logs de Rede

```cpp
// Sempre logue IPs em operações de rede
unsigned int clientIP = GetClientIP();
pLog.Info("Requisição HTTP", "GET /api/users", clientIP);
pLog.Packets("Pacote TCP", "SYN", clientIP);
```

---

Para mais exemplos, consulte `main.cpp` no repositório!
