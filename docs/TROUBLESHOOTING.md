# Troubleshooting - LogSystem MB

## ?? Problemas Comuns e Soluções

### 1. Processo não fecha completamente (fica no Task Manager)

**Sintoma:**
- Ao clicar no X da janela, a interface fecha mas o processo continua em execução
- No Gerenciador de Tarefas, o processo permanece listado

**Causa:**
- Worker thread não foi finalizada corretamente
- `Shutdown()` não foi chamado antes de fechar a aplicação

**Solução:**

? **Versão 2.0.1+**: Problema já corrigido. O sistema agora:
- Detecta `WM_CLOSE` antes de destruir a janela
- Chama `Shutdown()` automaticamente
- Worker thread tem timeout de 5 segundos
- Flush final garantido

Se ainda assim ocorrer:

1. **Verificar código de fechamento:**
```cpp
case WM_CLOSE:
    pLog.Info("Fechando aplicação...");
    pLog.Shutdown();  // ?? OBRIGATÓRIO
    DestroyWindow(hWnd);
    return 0;

case WM_DESTROY:
    PostQuitMessage(0);
    break;
```

2. **Modo Debug - Verificar thread:**
- Abra Visual Studio ? Debug ? Windows ? Threads
- Verifique se há thread "WorkerThreadFunc" ativa após fechar
- Se sim, verifique se `stopWorker` está sendo setado

3. **Forçar término (último recurso):**
```cpp
// Em caso extremo, após 5s de timeout
if (workerThread.joinable()) {
    workerThread.detach();  // Libera thread (pode perder logs pendentes)
}
```

---

### 2. Logs não aparecem na interface

**Sintoma:**
- Logs gravados em arquivo mas não exibidos no RichEdit

**Causas possíveis:**

**A. Msftedit.dll não carregada**

Solução:
```cpp
LoadLibraryA("Msftedit.dll");  // Adicionar no início de WinMain
```

**B. SetTarget não chamado**

Solução:
```cpp
pLog.SetTarget(TargetSide::Left, hEditLog);
pLog.SetTarget(TargetSide::Right, hEditPackets);
```

**C. Modo headless ativo**

Verificar `Config/logconfig.ini`:
```ini
headlessMode=false  # Deve estar false para GUI
```

**D. Handle inválido**

Verificar se RichEdit foi criado corretamente:
```cpp
if (!hEditLog) {
    MessageBoxA(NULL, "Falha ao criar RichEdit", "Erro", MB_OK);
}
```

---

### 3. Performance muito baixa / Travamentos

**Sintoma:**
- Interface trava ao gerar muitos logs
- Lentidão extrema

**Soluções:**

**A. Ativar modo assíncrono**

```ini
[Log]
asyncLogging=true  # Essencial para performance
```

**B. Limitar linhas do RichEdit**

```ini
maxRichEditLines=5000  # Reduzir se travar
```

ou via código:
```cpp
pLog.SetMaxRichEditLines(5000);
```

**C. Desabilitar níveis desnecessários**

```cpp
// Em produção, desabilitar Trace e Debug
pLog.DisableFileLevel(LogLevel::Trace);
pLog.DisableFileLevel(LogLevel::Debug);
```

**D. Modo headless para servidores**

```ini
headlessMode=true  # Desabilita GUI, só arquivo
```

---

### 4. Arquivo de log em uso / Não consegue abrir

**Sintoma:**
```
ERROR: Falha ao abrir arquivo de log
```

**Causas:**

**A. Arquivo aberto em outro programa**
- Notepad++, Sublime, etc. com lock exclusivo

Solução: Feche o editor ou use modo somente leitura

**B. Antivírus/Backup bloqueando**

Solução: Adicione pasta `Log/` às exceções

**C. Permissões insuficientes**

Solução: Execute como Administrador (primeira vez)

**D. Caminho inválido**

Verificar:
```cpp
fs::create_directories(logDir);  // Garante que pasta existe
```

---

### 5. Senha FTP não criptografa

**Sintoma:**
- Senha permanece em texto plano no INI

**Solução:**

1. **Verificar permissões de usuário:**
   - DPAPI requer usuário Windows válido
   - Não funciona com conta de serviço/sistema sem perfil

2. **Testar criptografia manual:**
```cpp
std::string encrypted = pLog.EncryptPassword("teste123");
if (encrypted.empty()) {
    // Erro na criptografia
}
```

3. **Fallback manual:**
Se DPAPI não funcionar, considere usar biblioteca externa (OpenSSL, etc.)

---

### 6. Compressão falha / ZIPs corrompidos

**Sintoma:**
```
ERROR: Falha ao criar ZIP
```

**Soluções:**

**A. Verificar minizip instalado**
```bash
vcpkg list | findstr minizip
```

**B. Espaço em disco**
- Compressão requer espaço temporário
- Verificar se há espaço suficiente

**C. Arquivo em uso**
- Log ainda sendo escrito
- Aguardar rotação completa

**D. Testar manualmente:**
```cpp
bool result = pLog.CompressFile("Log/server_2024-01-15_1.log");
if (!result) {
    // Verificar logs de erro
}
```

---

### 7. Upload FTP falha

**Sintoma:**
```
ERROR: Falha no upload para FTP
```

**Diagnóstico:**

```cpp
// Testar conexão FTP manualmente
HINTERNET hInternet = InternetOpenA("Test", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
HINTERNET hFtp = InternetConnectA(hInternet, "ftp.exemplo.com", 
                                   INTERNET_DEFAULT_FTP_PORT,
                                   "user", "pass", INTERNET_SERVICE_FTP, 0, 0);
if (!hFtp) {
    DWORD err = GetLastError();
    // err contém o código do erro
}
```

**Soluções:**

**A. Firewall bloqueando**
- Verificar se porta 21 está aberta
- Adicionar exceção ao Firewall do Windows

**B. Credenciais inválidas**
- Testar com FileZilla primeiro
- Verificar se senha foi descriptografada corretamente

**C. Modo passivo**
```cpp
// Se necessário, usar modo passivo:
InternetSetOptionA(hFtp, INTERNET_OPTION_FTP_PASV, &pasv, sizeof(pasv));
```

**D. Timeout**
```cpp
DWORD timeout = 30000; // 30 segundos
InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
```

---

### 8. Memory Leak / Uso crescente de RAM

**Sintoma:**
- Memória cresce continuamente
- Após horas, GB de RAM usado

**Diagnóstico:**

```cpp
const auto& stats = pLog.GetStats();
std::cout << "Queue peak: " << stats.queuePeak.load() << "\n";
std::cout << "Queue full: " << stats.queueFull.load() << "\n";
```

Se `queueFull` está alto ? fila não está sendo processada

**Soluções:**

**A. String pool overflow**
- String pool tem apenas 32 slots
- Com muitas threads, pode falhar

Aumentar:
```cpp
static constexpr size_t STRING_POOL_SIZE = 64;  // Era 32
```

**B. Queue overflow**
```cpp
static constexpr size_t LOCK_FREE_QUEUE_SIZE = 16384;  // Era 8192
```

**C. RichEdit não trimming**
- Verificar se `maxRichEditLines` está configurado
- `TrimRichEdit()` deve ser chamado regularmente

---

### 9. Crashes / Access Violations

**Sintomas comuns:**

**A. Crash em `AppendColoredText`**

Causa: Handle inválido

Solução:
```cpp
if (target && IsWindow(target)) {
    // Só então acessar
}
```

**B. Crash em `Shutdown()`**

Causa: Double-shutdown

Solução:
```cpp
static bool shutdownCalled = false;
if (shutdownCalled) return;
shutdownCalled = true;
```

**C. Crash em destrutor**

Causa: Worker thread ainda rodando

Solução: Garantir `Shutdown()` antes do destrutor

---

### 10. Compilação falha

**A. "std::format not found"**

Solução: Compilador precisa suportar C++20 completo
```
MSVC 2019 16.10+ ou MSVC 2022
GCC 10+
Clang 12+
```

Alternativa: Usar `fmt` library
```bash
vcpkg install fmt
```

**B. "LNK2019: CryptProtectData"**

Solução: Adicionar `crypt32.lib` ao linker

**C. "minizip.h not found"**

Solução:
```bash
vcpkg install minizip:x64-windows
```

---

## ?? Ferramentas de Diagnóstico

### 1. Habilitar logs detalhados

```cpp
pLog.EnableFileLevel(LogLevel::Trace);
pLog.EnableFileLevel(LogLevel::Debug);
```

### 2. Verificar estatísticas

```cpp
const auto& stats = pLog.GetStats();
std::cout << "Total logs: " << stats.totalLogs.load() << "\n";
std::cout << "Queue full events: " << stats.queueFull.load() << "\n";
std::cout << "Queue peak: " << stats.queuePeak.load() << "\n";
std::cout << "Files rotated: " << stats.filesRotated.load() << "\n";
```

### 3. Profiling

Use Visual Studio Diagnostic Tools:
- Debug ? Performance Profiler
- Selecione "CPU Usage"
- Identifique hotspots

### 4. Thread debugging

```
Debug ? Windows ? Threads
```
Verifique:
- Número de threads ativas
- Worker thread em execução
- Deadlocks (threads bloqueadas)

---

## ?? Suporte

Se nenhuma solução acima resolver:

1. **Colete informações:**
   - Versão do Windows
   - Versão do compilador
   - Logs de erro completos
   - Código de reprodução mínimo

2. **Abra issue:**
   https://github.com/GuilhermeCandiotto/LogSystemMB/issues

3. **Inclua:**
   - Descrição detalhada
   - Passos para reproduzir
   - Logs relevantes
   - Configuração INI

---

**Última atualização:** 2024-01-15  
**Versão:** 2.0.1
