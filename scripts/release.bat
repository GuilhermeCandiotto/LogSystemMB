@echo off
REM Release script para LogSystemMB
REM Gera pacote de distribuição completo

setlocal enabledelayedexpansion

echo ========================================
echo   LogSystemMB Release Packager
echo ========================================
echo.

REM Versão (ler de algum lugar ou pedir)
set /p VERSION="Digite a versao (ex: 2.0.0): "
if "%VERSION%"=="" (
    echo ERRO: Versao nao pode ser vazia
    exit /b 1
)

set RELEASE_DIR=Release\LogSystemMB-v%VERSION%
set RELEASE_ZIP=LogSystemMB-v%VERSION%-windows-x64.zip

echo Versao: %VERSION%
echo Diretorio: %RELEASE_DIR%
echo.

REM Limpar release anterior
if exist "Release" (
    echo [1/7] Limpando release anterior...
    rmdir /s /q "Release"
)

REM Criar estrutura de diretórios
echo [2/7] Criando estrutura de diretorios...
mkdir "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%\bin"
mkdir "%RELEASE_DIR%\docs"
mkdir "%RELEASE_DIR%\Config"
mkdir "%RELEASE_DIR%\examples"

REM Build Release x64
echo [3/7] Compilando versao Release x64...
call scripts\build.bat Release x64
if %ERRORLEVEL% NEQ 0 (
    echo ERRO: Falha na compilacao
    exit /b 1
)

REM Copiar executáveis e DLLs
echo [4/7] Copiando executaveis e DLLs...
copy "x64\Release\LogSystemMB.exe" "%RELEASE_DIR%\bin\" >nul
copy "x64\Release\*.dll" "%RELEASE_DIR%\bin\" >nul 2>&1

REM Build benchmark (opcional)
if exist "benchmark.cpp" (
    echo Compilando benchmark...
    REM TODO: Adicionar compilação do benchmark
    REM copy "x64\Release\benchmark.exe" "%RELEASE_DIR%\bin\" >nul 2>&1
)

REM Copiar documentação
echo [5/7] Copiando documentacao...
copy "README.md" "%RELEASE_DIR%\" >nul
copy "CHANGELOG.md" "%RELEASE_DIR%\" >nul
copy "CONTRIBUTING.md" "%RELEASE_DIR%\" >nul
copy "CODE_OF_CONDUCT.md" "%RELEASE_DIR%\" >nul
copy "LICENSE.txt" "%RELEASE_DIR%\" >nul
copy "EXAMPLES.md" "%RELEASE_DIR%\examples\" >nul

if exist "docs" (
    copy "docs\*.md" "%RELEASE_DIR%\docs\" >nul
)

REM Criar arquivo de configuração padrão
echo [6/7] Criando arquivo de configuracao padrao...
(
echo [Log]
echo retentionDays=7
echo maxLogSize=1048576
echo compressMode=day
echo maxRichEditLines=10000
echo asyncLogging=true
echo headlessMode=false
echo.
echo [Backup]
echo uploadBackup=false
echo ftpServer=ftp.example.com
echo ftpUser=username
echo ftpPass=
echo ftpPath=/backup/
) > "%RELEASE_DIR%\Config\logconfig.ini"

REM Criar README de instalação rápida
echo [7/7] Criando README de instalacao...
(
echo LogSystemMB v%VERSION%
echo =======================================
echo.
echo INSTALACAO RAPIDA:
echo.
echo 1. Descompacte este arquivo em uma pasta
echo 2. Execute bin\LogSystemMB.exe
echo 3. Configuracoes estao em Config\logconfig.ini
echo.
echo DOCUMENTACAO:
echo.
echo - README.md: Visao geral do projeto
echo - CHANGELOG.md: Historico de versoes
echo - docs\API_REFERENCE.md: Referencia completa da API
echo - docs\ARCHITECTURE.md: Arquitetura interna
echo - docs\INSTALLATION.md: Guia de instalacao detalhado
echo - examples\EXAMPLES.md: Exemplos de uso
echo.
echo SUPORTE:
echo.
echo - Issues: https://github.com/GuilhermeCandiotto/LogSystemMB/issues
echo - Documentacao: https://github.com/GuilhermeCandiotto/LogSystemMB
echo.
echo LICENCA: GNU AGPL-3.0
echo.
) > "%RELEASE_DIR%\QUICK_START.txt"

REM Compactar
echo.
echo Criando arquivo ZIP...
powershell -Command "Compress-Archive -Path '%RELEASE_DIR%' -DestinationPath 'Release\%RELEASE_ZIP%' -Force"

if %ERRORLEVEL% EQ 0 (
    echo.
    echo ========================================
    echo   RELEASE CRIADO COM SUCESSO!
    echo ========================================
    echo.
    echo Arquivo: Release\%RELEASE_ZIP%
    echo Tamanho: 
    dir "Release\%RELEASE_ZIP%" | find "%RELEASE_ZIP%"
    echo.
    echo Para distribuir:
    echo 1. Teste o pacote em uma maquina limpa
    echo 2. Crie release no GitHub
    echo 3. Faca upload do ZIP
    echo 4. Atualize CHANGELOG.md
    echo.
) else (
    echo.
    echo ERRO: Falha ao criar arquivo ZIP
    exit /b 1
)

REM Perguntar se quer abrir pasta
set /p OPEN="Abrir pasta Release? (S/N): "
if /i "%OPEN%"=="S" (
    explorer "Release"
)

endlocal
