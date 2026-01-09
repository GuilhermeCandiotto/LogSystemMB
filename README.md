# LogSystem MB - Sistema de Logging Thread-Safe

![License](https://img.shields.io/badge/license-AGPL--3.0-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)

Sistema de logging avançado e thread-safe para aplicações C++ Windows, com interface gráfica integrada, compressão automática e backup via FTP.

## ?? Características

### Core Features
- ? **Thread-Safe** - Proteção completa com mutexes para ambientes multithread
- ? **Async Logging** - Fila assíncrona com worker thread para alta performance
- ? **Dual Panel Interface** - Logs gerais à esquerda, pacotes à direita
- ? **7 Níveis de Log** - Trace, Debug, Info, Warning, Error, Quest, Packets
- ? **Colorização** - Cores diferentes para cada nível de log
- ? **Rotação Automática** - Por data e tamanho de arquivo

### Advanced Features
- ?? **Criptografia** - Senhas FTP protegidas com Windows DPAPI
- ?? **Compressão** - ZIP automático de logs antigos (none/file/day)
- ?? **Backup FTP** - Upload automático para servidor remoto
- ?? **Roteamento** - Direcionamento automático por tipo de log
- ?? **Buffer Limitado** - Previne crescimento infinito do RichEdit
- ??? **Headless Mode** - Funcionamento sem GUI para servidores

## ?? Requisitos

### Dependências
- **C++20 Compiler** (MSVC 2019+, GCC 10+, Clang 12+)
- **Windows SDK** (para Win32 API)
- **ZLIB** - Compressão
- **Minizip** - Manipulação de arquivos ZIP
- **Msftedit.dll** - RichEdit 5.0 (incluído no Windows)

### Sistema Operacional
- Windows 7 ou superior
- Windows Server 2012 ou superior

## ?? Instalação

### Opção 1: Visual Studio (Recomendado)

1. Clone o repositório:
```bash
git clone https://github.com/GuilhermeCandiotto/LogSystemMB.git
cd LogSystemMB
```

2. Abra `LogSystemMB.sln` no Visual Studio

3. Instale as dependências via vcpkg:
```bash
vcpkg install zlib:x64-windows
vcpkg install minizip:x64-windows
```

4. Configure o projeto para usar vcpkg (Visual Studio faz isso automaticamente)

5. Compile (Ctrl+Shift+B)

### Opção 2: CMake

1. Clone o repositório:
```bash
git clone https://github.com/GuilhermeCandiotto/LogSystemMB.git
cd LogSystemMB
```

2. Instale dependências via vcpkg:
```bash
vcpkg install zlib minizip
```

3. Configure e compile:
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

4. Execute:
```bash
.\bin\Release\LogSystemMB.exe
```

## ?? Uso Básico

### Exemplo Simples

```cpp
#include "LogSystem.h"

int main() {
    // Obter instância (Singleton)
    auto& log = pLog;
    
    // Configurar destinos (RichEdit handles)
    log.SetTarget(TargetSide::Left, hEditLog);
    log.SetTarget(TargetSide::Right, hEditPackets);
    
    // Habilitar gravação em arquivo
    log.EnableFileLevel(LogLevel::Info);
    log.EnableFileLevel(LogLevel::Warning);
    log.EnableFileLevel(LogLevel::Error);
    
    // Usar logs
    log.Info("Servidor iniciado");
    log.Warning("Conexão instável");
    log.Error("Falha crítica");
    
    // Com informações extras
    log.Info("Player conectado", "Username: João");
    
    // Com IP
    unsigned int ip = (192 << 24) | (168 << 16) | (1 << 8) | 100;
    log.Info("Requisição recebida", "GET /api/data", ip);
    
    // Shutdown graceful
    log.Shutdown();
    
    return 0;
}
```

### Níveis de Log

| Nível    | Método      | Cor        | Uso                          |
|----------|-------------|------------|------------------------------|
| Trace    | `Trace()`   | Cinza      | Debug detalhado              |
| Debug    | `Debug()`   | Azul Claro | Informações de debug         |
| Info     | `Info()`    | Verde      | Informações gerais           |
| Warning  | `Warning()` | Amarelo    | Avisos                       |
| Error    | `Error()`   | Vermelho   | Erros                        |
| Quest    | `Quest()`   | Rosa       | Sistema de quests (customizado) |
| Packets  | `Packets()` | Laranja    | Tráfego de rede              |

## ?? Configuração

### Arquivo `Config/logconfig.ini`

```ini
[Log]
# Dias de retenção antes de compactar/deletar (1-365)
retentionDays=7

# Tamanho máximo do arquivo antes de rotacionar (100KB-100MB)
maxLogSize=1048576

# Modo de compressão: none, file, day
compressMode=day

# Máximo de linhas no RichEdit (100-100000)
maxRichEditLines=10000

# Habilitar logging assíncrono (true/false)
asyncLogging=true

# Modo headless - sem GUI (true/false)
headlessMode=false

[Backup]
# Habilitar upload FTP (true/false)
uploadBackup=false

# Servidor FTP
ftpServer=ftp.meuservidor.com

# Usuário FTP
ftpUser=usuario

# Senha FTP (será criptografada automaticamente)
ftpPass=

# Pasta remota
ftpPath=/backup/
```

### Modos de Compressão

- **`none`** - Apenas deleta arquivos antigos sem comprimir
- **`file`** - Compacta cada arquivo individualmente (.zip)
- **`day`** - Compacta todos os arquivos de um dia em um único ZIP (**recomendado**)

## ?? Segurança

### Criptografia de Senhas

As senhas FTP são automaticamente criptografadas usando **Windows DPAPI** (Data Protection API):

- ? Criptografia específica por usuário e máquina
- ? Não requer chave manual
- ? Descriptografia automática no mesmo contexto
- ? Armazenamento em hexadecimal no INI

**Primeira execução:**
1. Coloque a senha em texto plano no `logconfig.ini`
2. Execute o programa
3. A senha será automaticamente criptografada
4. Arquivo será atualizado com versão criptografada

## ?? Thread Safety

### Recursos Protegidos

Todos os recursos compartilhados são protegidos por mutexes:

- **`logMutex`** - Configurações gerais
- **`fileMutex`** - Acesso ao arquivo de log
- **`targetMutex`** - Handles dos RichEdit
- **`queueMutex`** - Fila de mensagens

### Async Logging

```
[Thread Cliente 1] ???
[Thread Cliente 2] ?????> [Queue Thread-Safe] ??> [Worker Thread] ??> [File + GUI]
[Thread Cliente N] ???
```

**Vantagens:**
- Threads clientes não bloqueiam em I/O
- Escrita sequencial no arquivo
- Sem race conditions
- Shutdown graceful com processamento de fila restante

## ?? Performance

### Benchmarks (Intel i7-9700K, SSD NVMe)

| Operação                  | Tempo (?s) | Throughput      |
|---------------------------|------------|-----------------|
| Log simples (async)       | ~2-5       | ~200,000 msg/s  |
| Log com IP e extra (async)| ~3-7       | ~140,000 msg/s  |
| Log síncrono              | ~50-100    | ~10,000 msg/s   |
| Rotação de arquivo        | ~1,000     | -               |
| Compressão ZIP (10MB)     | ~200,000   | ~50 MB/s        |

## ??? API Reference

### LogSystem Class

#### Métodos Principais

```cpp
// Configuração
void Initialize();
void SetTarget(TargetSide side, HWND editHandle);
void LoadConfig(const std::string& filename);
void Shutdown();

// Logging
void Log(LogLevel level, const std::string& msg, 
         const std::string& extra = "", unsigned int ip = 0);
void Trace(const std::string& msg, ...);
void Debug(const std::string& msg, ...);
void Info(const std::string& msg, ...);
void Warning(const std::string& msg, ...);
void Error(const std::string& msg, ...);
void Quest(const std::string& msg, ...);
void Packets(const std::string& msg, ...);

// Controle
void EnableFileLevel(LogLevel level);
void DisableFileLevel(LogLevel level);
void SetMaxRichEditLines(int maxLines);
void ClearRichEdit(TargetSide side);

// Utilitários
template<typename... Args>
std::string Format(const char* fmt, Args&&... args);
```

### Macro Global

```cpp
#define pLog (LogManager::Instance().GetLogInst())

// Uso:
pLog.Info("Mensagem");
```

## ?? Estrutura de Arquivos

```
LogSystemMB/
??? Config/
?   ??? logconfig.ini          # Configuração
??? Log/
?   ??? server_2024-01-15_1.log
?   ??? server_2024-01-15_2.log
?   ??? logpack_2024-01-08.zip # Compactado
??? LogSystem.h                # Header principal
??? LogSystem.cpp              # Implementação
??? main.cpp                   # Exemplo de uso (GUI)
??? CMakeLists.txt             # Build system
??? LogSystemMB.sln            # Visual Studio Solution
??? LogSystemMB.vcxproj        # Visual Studio Project
??? README.md                  # Este arquivo
??? LICENSE.txt                # AGPL-3.0
```

## ?? Troubleshooting

### Problema: "Arquivo de log em uso"

**Solução:** O arquivo está aberto em outro processo. Feche ou aguarde rotação automática.

### Problema: "Falha ao criptografar senha FTP"

**Solução:** Verifique se está executando em conta com permissões adequadas no Windows.

### Problema: "Minizip não encontrado"

**Solução:**
```bash
vcpkg install minizip:x64-windows
# ou especifique manualmente no CMakeLists.txt
```

### Problema: RichEdit não exibe logs

**Solução:** Verifique se `Msftedit.dll` foi carregado:
```cpp
LoadLibraryA("Msftedit.dll");
```

## ?? Changelog

### v2.0 (2024-01-15)
- ? Thread safety completo
- ? Async logging com worker thread
- ? Criptografia de senhas FTP
- ? Validação de configuração
- ? Modo headless
- ? Limite de buffer RichEdit
- ? Correção rotação de arquivos
- ? Implementação completa de compressMode
- ? RAII para handles Windows
- ? CMake build system

### v1.0 (2023-XX-XX)
- Versão inicial

## ?? Contribuindo

Contribuições são bem-vindas! Por favor:

1. Fork o projeto
2. Crie uma branch para sua feature (`git checkout -b feature/AmazingFeature`)
3. Commit suas mudanças (`git commit -m 'Add some AmazingFeature'`)
4. Push para a branch (`git push origin feature/AmazingFeature`)
5. Abra um Pull Request

## ?? Licença

Este projeto está licenciado sob a **GNU Affero General Public License v3.0** - veja o arquivo [LICENSE.txt](LICENSE.txt) para detalhes.

### O que isso significa?

- ? Uso comercial permitido
- ? Modificação permitida
- ? Distribuição permitida
- ?? Deve manter mesma licença
- ?? Deve divulgar código fonte
- ?? Se usar em rede, deve disponibilizar fonte

## ????? Autor

**Guilherme Candiotto**

- GitHub: [@GuilhermeCandiotto](https://github.com/GuilhermeCandiotto)
- Projeto: [LogSystemMB](https://github.com/GuilhermeCandiotto/LogSystemMB)

## ?? Agradecimentos

- Comunidade WYD (With Your Destiny)
- Contributors do ZLIB e Minizip
- Microsoft (Win32 API e DPAPI)

## ?? Suporte

- **Issues:** [GitHub Issues](https://github.com/GuilhermeCandiotto/LogSystemMB/issues)
- **Documentação:** Este README
- **Exemplos:** Veja `main.cpp` para exemplo completo

---

? **Se este projeto foi útil, considere dar uma estrela no GitHub!** ?
