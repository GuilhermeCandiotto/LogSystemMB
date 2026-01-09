@echo off
REM Build script para LogSystemMB
REM Uso: build.bat [Debug|Release] [x64|Win32]

setlocal

REM Configurações padrão
set CONFIG=Release
set PLATFORM=x64

REM Parse argumentos
if not "%1"=="" set CONFIG=%1
if not "%2"=="" set PLATFORM=%2

echo ========================================
echo   LogSystemMB Build Script
echo ========================================
echo Configuration: %CONFIG%
echo Platform: %PLATFORM%
echo ========================================
echo.

REM Verificar se MSBuild está disponível
where msbuild >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: MSBuild nao encontrado no PATH
    echo.
    echo Solucoes:
    echo 1. Instale Visual Studio 2019 ou superior
    echo 2. Execute este script do "Developer Command Prompt"
    echo 3. Adicione MSBuild ao PATH
    exit /b 1
)

REM Verificar se vcpkg está configurado
if not exist "vcpkg" (
    echo AVISO: vcpkg nao encontrado localmente
    echo Tentando usar vcpkg global...
    echo.
)

REM Build
echo [1/3] Limpando build anterior...
if exist "%PLATFORM%\%CONFIG%" (
    rmdir /s /q "%PLATFORM%\%CONFIG%"
)

echo [2/3] Compilando...
msbuild LogSystemMB.sln /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m /v:minimal

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo   BUILD FALHOU!
    echo ========================================
    exit /b 1
)

echo [3/3] Copiando arquivos necessarios...

REM Criar diretórios no output
if not exist "%PLATFORM%\%CONFIG%\Config" mkdir "%PLATFORM%\%CONFIG%\Config"
if not exist "%PLATFORM%\%CONFIG%\Log" mkdir "%PLATFORM%\%CONFIG%\Log"

REM Copiar DLLs do vcpkg se existir
if exist "vcpkg\installed\%PLATFORM%-windows\bin" (
    echo Copiando DLLs do vcpkg...
    copy "vcpkg\installed\%PLATFORM%-windows\bin\*.dll" "%PLATFORM%\%CONFIG%\" >nul 2>&1
)

echo.
echo ========================================
echo   BUILD COMPLETO!
echo ========================================
echo.
echo Executavel: %PLATFORM%\%CONFIG%\LogSystemMB.exe
echo.
echo Para executar:
echo   cd %PLATFORM%\%CONFIG%
echo   LogSystemMB.exe
echo.

REM Perguntar se quer executar
set /p RUN="Executar agora? (S/N): "
if /i "%RUN%"=="S" (
    cd "%PLATFORM%\%CONFIG%"
    start LogSystemMB.exe
    cd ..\..
)

endlocal
