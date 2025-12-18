#include <windows.h>
#include <richedit.h>
#include "LogSystem.h"

using namespace WYD_Server;

HWND hEditLog;
HWND hEditPackets;
HWND hEditServerInfo;
HWND hEditMessage;
HWND hButtonSendMsg;
HWND hButtonFuture1;
HWND hButtonFuture2;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            UINT dpi = GetDpiForWindow(hWnd);
            float scale = dpi / 96.0f;

            const int margin = static_cast<int>(10 * scale);
            const int spacing = static_cast<int>(10 * scale);
            const int buttonW = static_cast<int>(100 * scale);
            const int buttonH = static_cast<int>(30 * scale);

            RECT rc;
            GetClientRect(hWnd, &rc);
            int cx = rc.right - rc.left;
            int cy = rc.bottom - rc.top;

            int topH = (cy * 50 + 50) / 100; // 50%
            int midH = (cy * 40 + 50) / 100; // 40%

            // RichEdit LogSystem
            hEditLog = CreateWindowExA(0, MSFTEDIT_CLASSA, "",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                margin, margin, cx / 2 - spacing, topH,
                hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessageA(hEditLog, EM_SETBKGNDCOLOR, 0, RGB(20, 20, 20));

            // Área de Packets
            hEditPackets = CreateWindowExA(0, MSFTEDIT_CLASSA, "",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                cx / 2 + spacing, margin, cx / 2 - 2 * margin, topH,
                hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessageA(hEditPackets, EM_SETBKGNDCOLOR, 0, RGB(20, 20, 20));

            // Informações do servidor
            hEditServerInfo = CreateWindowExA(0, MSFTEDIT_CLASSA, "",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                margin, margin + topH + spacing, cx - 2 * margin, midH,
                hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessageA(hEditServerInfo, EM_SETBKGNDCOLOR, 0, RGB(20, 20, 20));

            int totalButtonW = 3 * buttonW + 2 * spacing;
            int msgW = cx - totalButtonW - 2 * margin - spacing;
            msgW = max(0, msgW);

            // Campo de mensagem
            hEditMessage = CreateWindowExA(0, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                margin, cy - buttonH - margin, msgW, buttonH,
                hWnd, NULL, GetModuleHandle(NULL), NULL);

            // Botão Enviar
            int btnX = margin + msgW + spacing;
            hButtonSendMsg = CreateWindowExA(0, "BUTTON", "Enviar",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                btnX, cy - buttonH - margin, buttonW, buttonH,
                hWnd, (HMENU)(INT_PTR)2001, GetModuleHandle(NULL), NULL);

            // Botão Futuro 1
            btnX += buttonW + spacing;
            hButtonFuture1 = CreateWindowExA(0, "BUTTON", "Ação futura 1",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                btnX, cy - buttonH - margin, buttonW, buttonH,
                hWnd, (HMENU)(INT_PTR)2002, GetModuleHandle(NULL), NULL);

            // Botão Futuro 2
            btnX += buttonW + spacing;
            hButtonFuture2 = CreateWindowExA(0, "BUTTON", "Ação futura 2",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                btnX, cy - buttonH - margin, buttonW, buttonH,
                hWnd, (HMENU)(INT_PTR)2003, GetModuleHandle(NULL), NULL);

			// Define quais tipos seram gravados no arquivo de log
            pLog.EnableFileLevel(LogLevel::Info);
            pLog.EnableFileLevel(LogLevel::Warning);
            pLog.EnableFileLevel(LogLevel::Error);
			pLog.EnableFileLevel(LogLevel::Quest);

            pLog.SetTarget(TargetSide::Left, hEditLog);
            pLog.SetTarget(TargetSide::Right, hEditPackets);

			// Teste de logs simulação de mensagens de servidor
            pLog.Trace("Mensagem de rastreamento detalhado.");
            pLog.Debug("Mensagem de depuração.");
            pLog.Info("Servidor iniciado com sucesso.");
            pLog.Info("Testando outros parametros.");
            pLog.Warning("Conexão instável detectada.");
            pLog.Error("Falha crítica no subsistema.");
			pLog.Quest("Quest iniciada pelo jogador.");
			pLog.Packets("Pacote recebido do cliente.");
        } break;

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
            case 2002: // Limpar Logs
                SendMessageA(hEditLog, EM_SETREADONLY, FALSE, 0);
                SetWindowTextA(hEditLog, "");
                SendMessageA(hEditLog, EM_SETREADONLY, TRUE, 0);
                pLog.Info("Log apagado com sucesso.");
                break;

            case 2003: // Enviar Log
                std::string ipStr = "192.168.0.1";
                unsigned int a, b, c, d;
                char dot;

                std::stringstream ss(ipStr);
                ss >> a >> dot >> b >> dot >> c >> dot >> d;

                unsigned int Ip = (a << 24) | (b << 16) | (c << 8) | d;

                pLog.Info("Mensagem de teste enviada pelo botão.");
                pLog.Trace("Mensagem de rastreamento detalhado.");
                pLog.Debug("Mensagem de depuração.");
                pLog.Info("Servidor iniciado com sucesso.");
                pLog.Info("Testando outros parametros.", "Segunda String1", Ip);
                pLog.Info("Testando outros parametros.", "Segunda String2", Ip);
                pLog.Warning("Conexão instável detectada.");
                pLog.Error("Falha crítica no subsistema.");
				pLog.Quest("Quest iniciada pelo jogador.", "Detalhes da quest", Ip);
				pLog.Packets("Pacote recebido do cliente.", "Detalhes do pacote", Ip);
                break;
            }
        } break;

        case WM_SIZE: {
            int cx = LOWORD(lParam);
            int cy = HIWORD(lParam);

            UINT dpi = GetDpiForWindow(hWnd);
            float scale = dpi / 96.0f;

            const int margin = static_cast<int>(10 * scale);
            const int spacing = static_cast<int>(10 * scale);
            const int buttonW = static_cast<int>(100 * scale);
            const int buttonH = static_cast<int>(30 * scale);

            int topH = (cy * 50 + 50) / 100; // 50% da altura
            int midH = (cy * 40 + 50) / 100; // 40% da altura

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
            MoveWindow(hButtonFuture1, btnX, cy - buttonH - margin, buttonW, buttonH, TRUE);

            btnX += buttonW + spacing;
            MoveWindow(hButtonFuture2, btnX, cy - buttonH - margin, buttonW, buttonH, TRUE);
        } break;

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

    const char CLASS_NAME[] = "LogSystem";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassA(&wc);

    HWND hWnd = CreateWindowExA(
        0, CLASS_NAME, "TesteLogSystem",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1480, 768,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}