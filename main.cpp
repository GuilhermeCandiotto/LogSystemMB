#include <windows.h>
#include <richedit.h>
#include <sstream>
#include "LogSystem.h"

using namespace WYD_Server;

HWND hEditLog;
HWND hEditPackets;
HWND hEditServerInfo;
HWND hEditMessage;
HWND hButtonSendMsg;
HWND hButtonClearLog;
HWND hButtonTestLogs;

void UpdateServerInfo() {
    std::stringstream ss;
    ss << "=== INFORMAÇÕES DO SERVIDOR ===\r\n";
    ss << "Sistema de Log: Ativo\r\n";
    ss << "Versão: 2.0 (Thread-Safe)\r\n";
    ss << "Modo Assíncrono: Habilitado\r\n";
    ss << "Compressão: Configurada\r\n";
    ss << "Backup FTP: Configurado\r\n";
    ss << "\r\n";
    ss << "Status: Operacional ✓\r\n";
    ss << "Logs sendo gravados em: ./Log/\r\n";
    
    SetWindowTextA(hEditServerInfo, ss.str().c_str());
    
    // Aplicar cor de fundo ao ServerInfo também
    SendMessageA(hEditServerInfo, EM_SETBKGNDCOLOR, 0, RGB(20, 20, 20));
    
    CHARFORMAT2A cf{};
    cf.cbSize = sizeof(CHARFORMAT2A);
    cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
    cf.crTextColor = RGB(0, 255, 255);  // Ciano
    strcpy_s(cf.szFaceName, "Consolas");
    cf.yHeight = 200;
    
    SendMessageA(hEditServerInfo, EM_SETSEL, 0, -1);
    SendMessageA(hEditServerInfo, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageA(hEditServerInfo, EM_SETSEL, 0, 0);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            UINT dpi = GetDpiForWindow(hWnd);
            float scale = dpi / 96.0f;

            const int margin = static_cast<int>(10 * scale);
            const int spacing = static_cast<int>(10 * scale);
            const int buttonW = static_cast<int>(120 * scale);
            const int buttonH = static_cast<int>(30 * scale);

            RECT rc;
            GetClientRect(hWnd, &rc);
            int cx = rc.right - rc.left;
            int cy = rc.bottom - rc.top;

            int topH = (cy * 50 + 50) / 100; // 50%
            int midH = (cy * 40 + 50) / 100; // 40%

            // RichEdit LogSystem (esquerda)
            hEditLog = CreateWindowExA(0, MSFTEDIT_CLASSA, "",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                margin, margin, cx / 2 - spacing, topH,
                hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessageA(hEditLog, EM_SETBKGNDCOLOR, 0, RGB(20, 20, 20));

            // Área de Packets (direita)
            hEditPackets = CreateWindowExA(0, MSFTEDIT_CLASSA, "",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                cx / 2 + spacing, margin, cx / 2 - 2 * margin, topH,
                hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessageA(hEditPackets, EM_SETBKGNDCOLOR, 0, RGB(20, 20, 20));

            // Informações do servidor
            hEditServerInfo = CreateWindowExA(0, MSFTEDIT_CLASSA, "",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                margin, margin + topH + spacing, cx - 2 * margin, midH,
                hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessageA(hEditServerInfo, EM_SETBKGNDCOLOR, 0, RGB(20, 20, 20));

            int totalButtonW = 3 * buttonW + 2 * spacing;
            int msgW = cx - totalButtonW - 2 * margin - spacing;
            msgW = max(0, msgW);

            // Campo de mensagem
            hEditMessage = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                margin, cy - buttonH - margin, msgW, buttonH,
                hWnd, NULL, GetModuleHandle(NULL), NULL);

            // Botão Enviar Mensagem
            int btnX = margin + msgW + spacing;
            hButtonSendMsg = CreateWindowExA(0, "BUTTON", "Enviar Msg",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                btnX, cy - buttonH - margin, buttonW, buttonH,
                hWnd, (HMENU)(INT_PTR)2001, GetModuleHandle(NULL), NULL);

            // Botão Limpar Logs
            btnX += buttonW + spacing;
            hButtonClearLog = CreateWindowExA(0, "BUTTON", "Limpar Logs",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                btnX, cy - buttonH - margin, buttonW, buttonH,
                hWnd, (HMENU)(INT_PTR)2002, GetModuleHandle(NULL), NULL);

            // Botão Testar Logs
            btnX += buttonW + spacing;
            hButtonTestLogs = CreateWindowExA(0, "BUTTON", "Testar Logs",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                btnX, cy - buttonH - margin, buttonW, buttonH,
                hWnd, (HMENU)(INT_PTR)2003, GetModuleHandle(NULL), NULL);

			// Define quais tipos serão gravados no arquivo de log
            pLog.EnableFileLevel(LogLevel::Info);
            pLog.EnableFileLevel(LogLevel::Warning);
            pLog.EnableFileLevel(LogLevel::Error);
			pLog.EnableFileLevel(LogLevel::Quest);

            pLog.SetTarget(TargetSide::Left, hEditLog);
            pLog.SetTarget(TargetSide::Right, hEditPackets);

            // Atualizar informações do servidor
            UpdateServerInfo();

			// Teste inicial de logs
            pLog.Trace("Sistema de rastreamento inicializado");
            pLog.Debug("Modo debug ativado");
            pLog.Info("Servidor iniciado com sucesso");
            pLog.Warning("Sistema de logs thread-safe ativo");
			pLog.Quest("Sistema de quests carregado");
			pLog.Packets("Sistema de pacotes online");
        } break;

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
            case 2001: { // Enviar Mensagem
                char buffer[256];
                GetWindowTextA(hEditMessage, buffer, 256);
                
                if (strlen(buffer) > 0) {
                    pLog.Info("Mensagem do usuário", buffer);
                    SetWindowTextA(hEditMessage, "");  // Limpar campo
                }
                else {
                    pLog.Warning("Campo de mensagem vazio");
                }
                break;
            }

            case 2002: { // Limpar Logs
                pLog.ClearRichEdit(TargetSide::Left);
                pLog.Info("Log principal limpo pelo usuário");
                break;
            }

            case 2003: { // Testar Logs
                pLog.Info("Iniciando teste de logs...");
                
                // Simular IP
                std::string ipStr = "192.168.1.100";
                unsigned int a, b, c, d;
                char dot;

                std::stringstream ss(ipStr);
                ss >> a >> dot >> b >> dot >> c >> dot >> d;
                unsigned int testIp = (a << 24) | (b << 16) | (c << 8) | d;

                pLog.Trace("Teste de mensagem TRACE");
                pLog.Debug("Teste de mensagem DEBUG");
                pLog.Info("Teste de mensagem INFO", "contexto adicional", testIp);
                pLog.Warning("Teste de mensagem WARNING", "alerta de teste");
                pLog.Error("Teste de mensagem ERROR", "erro simulado", testIp);
                pLog.Quest("Quest aceita pelo jogador", "ID Quest: 1001", testIp);
                pLog.Packets("Pacote C2S_MOVE recebido", "Tamanho: 16 bytes", testIp);
                
                pLog.Info("Teste de logs concluído");
                break;
            }
            }
        } break;

        case WM_SIZE: {
            int cx = LOWORD(lParam);
            int cy = HIWORD(lParam);

            UINT dpi = GetDpiForWindow(hWnd);
            float scale = dpi / 96.0f;

            const int margin = static_cast<int>(10 * scale);
            const int spacing = static_cast<int>(10 * scale);
            const int buttonW = static_cast<int>(120 * scale);
            const int buttonH = static_cast<int>(30 * scale);

            int topH = (cy * 50 + 50) / 100;
            int midH = (cy * 40 + 50) / 100;

            topH = max(topH, buttonH);
            midH = max(midH, buttonH);

            // RichEdit LogSystem
            MoveWindow(hEditLog, margin, margin, max(0, cx / 2 - spacing), topH, TRUE);

            // Área de Packets
            MoveWindow(hEditPackets, cx / 2 + spacing, margin, max(0, cx / 2 - 2 * margin), topH, TRUE);

            // Informações do servidor
            MoveWindow(hEditServerInfo, margin, margin + topH + spacing, max(0, cx - 2 * margin), midH, TRUE);

            int totalButtonW = 3 * buttonW + 2 * spacing;
            int msgW = cx - totalButtonW - 2 * margin - spacing;
            msgW = max(0, msgW);

            // Campo de mensagem
            MoveWindow(hEditMessage, margin, cy - buttonH - margin, msgW, buttonH, TRUE);

            int btnX = margin + msgW + spacing;
            MoveWindow(hButtonSendMsg, btnX, cy - buttonH - margin, buttonW, buttonH, TRUE);

            btnX += buttonW + spacing;
            MoveWindow(hButtonClearLog, btnX, cy - buttonH - margin, buttonW, buttonH, TRUE);

            btnX += buttonW + spacing;
            MoveWindow(hButtonTestLogs, btnX, cy - buttonH - margin, buttonW, buttonH, TRUE);
        } break;

        case WM_CLOSE:
            // Shutdown log system BEFORE destroying window
            pLog.Info("Fechando aplicação...");
            pLog.Shutdown();
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {

    LoadLibraryA("Msftedit.dll");

    const char CLASS_NAME[] = "LogSystemMB";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Falha ao registrar classe da janela", "Erro", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hWnd = CreateWindowExA(
        0, CLASS_NAME, "LogSystem MB - Thread-Safe Edition",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1480, 768,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        MessageBoxA(NULL, "Falha ao criar janela", "Erro", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}