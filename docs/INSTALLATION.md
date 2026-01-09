# Guia de Instalação - LogSystem MB

## ?? Métodos de Instalação

### Opção 1: Visual Studio (Recomendado para Windows)

#### Pré-requisitos
- Visual Studio 2019 ou superior
- Windows SDK 10.0 ou superior
- vcpkg (gerenciador de pacotes)

#### Passos

1. **Clone o repositório**
```bash
git clone https://github.com/GuilhermeCandiotto/LogSystemMB.git
cd LogSystemMB
```

2. **Instale vcpkg** (se não tiver)
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

3. **Instale dependências**
```bash
.\vcpkg install zlib:x64-windows
.\vcpkg install minizip:x64-windows
```

4. **Abra a solution**
- Abra `LogSystemMB.sln` no Visual Studio
- Selecione configuração `Release` e plataforma `x64`
- Build (`Ctrl+Shift+B`)

5. **Execute**
```
.\x64\Release\LogSystemMB.exe
```

---

### Opção 2: CMake (Multi-plataforma)

#### Pré-requisitos
- CMake 3.20+
- Compilador C++20 (MSVC, GCC 10+, Clang 12+)
- ZLIB e Minizip

#### Passos

1. **Clone e configure**
```bash
git clone https://github.com/GuilhermeCandiotto/LogSystemMB.git
cd LogSystemMB
mkdir build && cd build
```

2. **Com vcpkg**
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

3. **Sem vcpkg** (dependências manuais)
```bash
cmake .. -DZLIB_ROOT=/path/to/zlib -DMINIZIP_ROOT=/path/to/minizip
cmake --build . --config Release
```

4. **Execute**
```bash
.\bin\Release\LogSystemMB.exe
```

---

### Opção 3: Build Manual

#### Windows (MSVC)

```batch
cl /std:c++20 /EHsc /MD /O2 ^
   /I"C:\path\to\zlib\include" ^
   /I"C:\path\to\minizip\include" ^
   main.cpp LogSystem.cpp ^
   /link ^
   /LIBPATH:"C:\path\to\zlib\lib" ^
   /LIBPATH:"C:\path\to\minizip\lib" ^
   zlib.lib minizip.lib wininet.lib crypt32.lib ^
   /SUBSYSTEM:WINDOWS
```

#### Linux (GCC) - Experimental

```bash
g++ -std=c++20 -O3 -pthread \
    -I/usr/include/minizip \
    main.cpp LogSystem.cpp \
    -lz -lminizip \
    -o LogSystemMB
```

**Nota:** GUI não funcionará em Linux (Win32 API). Use modo headless.

---

## ?? Configuração Pós-Instalação

### 1. Configurar arquivo INI

Ao executar pela primeira vez, o arquivo `Config/logconfig.ini` será criado automaticamente com valores padrão:

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
ftpServer=ftp.seuservidor.com
ftpUser=usuario
ftpPass=
ftpPath=/backup/
```

### 2. Configurar FTP (Opcional)

Se quiser backup automático:

1. Edite `Config/logconfig.ini`:
```ini
[Backup]
uploadBackup=true
ftpServer=ftp.exemplo.com
ftpUser=seu_usuario
ftpPass=sua_senha_em_texto_plano
ftpPath=/logs/backup/
```

2. Execute o programa (a senha será criptografada automaticamente)

3. Verifique o arquivo novamente - a senha estará em hexadecimal:
```ini
ftpPass=01a4f3e8b2c9...
```

---

## ?? Troubleshooting

### Problema: "MSVCP140.dll não encontrado"

**Causa:** Visual C++ Redistributable não instalado

**Solução:**
- Baixe e instale: https://aka.ms/vs/17/release/vc_redist.x64.exe
- Ou compile com `/MT` (static linking)

---

### Problema: "zlib.dll não encontrado"

**Causa:** DLL não está no PATH

**Solução 1** - Copiar DLLs para diretório do exe:
```batch
copy C:\path\to\vcpkg\installed\x64-windows\bin\zlib1.dll .\
copy C:\path\to\vcpkg\installed\x64-windows\bin\minizip.dll .\
```

**Solução 2** - Linkar estaticamente:
No Visual Studio:
- Propriedades ? C/C++ ? Code Generation ? Runtime Library ? `/MT`

---

### Problema: "Erro ao criar arquivo de configuração"

**Causa:** Sem permissão de escrita

**Solução:**
- Execute como Administrador (primeira vez)
- Ou crie `Config/` manualmente
- Verifique permissões da pasta

---

### Problema: Compilação falha com "std::format not found"

**Causa:** Compilador não suporta C++20 completo

**Solução:**
- Visual Studio: Use 2019 versão 16.10+ ou 2022
- GCC: Use GCC 10 ou superior
- Clang: Use Clang 12 ou superior
- Ou substitua `std::format` por `fmt` library

---

### Problema: "LNK2019: CryptProtectData não resolvido"

**Causa:** Falta biblioteca `crypt32.lib`

**Solução:**
Adicione ao linker:
```
crypt32.lib
```

No Visual Studio:
- Propriedades ? Linker ? Input ? Additional Dependencies ? Adicionar `crypt32.lib`

---

## ?? Verificar Instalação

Execute o seguinte teste:

```cpp
#include "LogSystem.h"

int main() {
    pLog.Info("Teste de instalação");
    pLog.Warning("Se você vê esta mensagem, instalação OK!");
    pLog.Shutdown();
    return 0;
}
```

Compile e execute. Se ver logs no console e arquivo `Log/server_YYYY-MM-DD_1.log` for criado, instalação está correta!

---

## ?? Próximos Passos

1. Leia [API_REFERENCE.md](API_REFERENCE.md) para documentação completa
2. Veja [EXAMPLES.md](../EXAMPLES.md) para exemplos práticos
3. Consulte [ARCHITECTURE.md](ARCHITECTURE.md) para detalhes internos
4. Leia [CONTRIBUTING.md](../CONTRIBUTING.md) se quiser contribuir

---

## ?? Dicas

### Performance Máxima
```ini
asyncLogging=true
maxLogSize=10485760  # 10MB
compressMode=day
```

### Debug Intensivo
```ini
asyncLogging=false  # Síncrono para debugging
maxLogSize=524288   # 512KB (rotação frequente)
```

### Servidor Headless
```ini
headlessMode=true
asyncLogging=true
```

---

**Precisa de ajuda?** Abra uma [issue no GitHub](https://github.com/GuilhermeCandiotto/LogSystemMB/issues)
