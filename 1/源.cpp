#include <windows.h>
#include <gdiplus.h>
#include <vector>
#pragma comment(lib, "gdiplus.lib")

#include <shellapi.h>
#define ID_TRAY_APP_ICON 5000 // ����ͼ���ID
#define ID_TRAY_EXIT      5001 // �˳��˵�ID
#define ID_TRAY_GREET     5002 // ���к��˵���ID

using namespace Gdiplus;
ULONG_PTR gdiplusToken;  // ��������Ϊȫ�ֱ���
POINT offset; // ����ƫ��
static POINT showPos; // ��ʾλ��
bool isDragging = false; // ��ק״̬

HWND hwnd; // ȫ�ִ��ھ��
// ��ʼ�� GDI+
void InitGDIPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}
// �ر� GDI+
void ShutdownGDIPlus(ULONG_PTR gdiplusToken) {
    Gdiplus::GdiplusShutdown(gdiplusToken);
}
//����
void ShowContextMenu(HWND hwnd, int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 1, TEXT("���к�"));
    AppendMenu(hMenu, MF_STRING, 3, TEXT("������"));
    AppendMenu(hMenu, MF_STRING, 2, TEXT("�˳�"));

    // ��ʾ�˵�
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, x, y, 0, hwnd, NULL);

    // ����˵�
    DestroyMenu(hMenu);
}
// ���ڹ��̺���
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static std::vector<Gdiplus::Image*> images; // ���ڴ洢����ͼƬ
    static int currentImageIndex = 0; // ��ǰ��ʾ��ͼƬ����
    //static Gdiplus::Image* image = new Gdiplus::Image(L"1.png");
    static NOTIFYICONDATA nid = {};
    // ���ض���PNGͼ��

    images.push_back(new Gdiplus::Image(L"shime1.png"));
    images.push_back(new Gdiplus::Image(L"shime2.png"));
    images.push_back(new Gdiplus::Image(L"shime3.png"));

    // ���ͼ�����״̬
    for (size_t i = 0; i < images.size(); i++) {
        if (images[i]->GetLastStatus() != Gdiplus::Ok) {
            MessageBox(NULL, L"Unable to load one of the images", L"Error", MB_OK);
            delete images[i]; // �����Ѽ��ص�ͼ��
            images.erase(images.begin() + i); // �Ƴ�δ�ɹ����ص�ͼ��
            i--; // ��������
        }
    }

    switch (uMsg) {
    case WM_CREATE: {
        // �������ͼ��      
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = ID_TRAY_APP_ICON;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_APP + 1; // �Զ�����Ϣ
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // ʹ��Ĭ��ͼ��
        lstrcpy(nid.szTip, TEXT("�Զ�������Ӧ�ó���")); // �����ͣʱ��ʾ���ı�

        Shell_NotifyIcon(NIM_ADD, &nid); // �������ͼ��
    }
                  return 0;
    case WM_APP + 1: // �Զ�����Ϣ����������ͼ���¼�
        if (lParam == WM_LBUTTONDBLCLK) { // ˫�����
            ShowWindow(hwnd, SW_SHOW); // ��ʾ������
            SetForegroundWindow(hwnd); // ����������ǰ��
        }
        else if (lParam == WM_RBUTTONDOWN) { // �Ҽ����
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(hwnd, pt.x, pt.y); // ��ʾ�����Ĳ˵�
        }
        return 0;
    case WM_DESTROY:
        for (auto image : images) {
            delete image; // ����ͼ����Դ
        }
        // �Ƴ�����ͼ��
        Shell_NotifyIcon(NIM_DELETE, &nid);
        KillTimer(hwnd, 1); // �رն�ʱ��
        PostQuitMessage(0);
        return 0;
    case WM_TIMER: {
        if (wParam == 1) { // ȷ�������������õĶ�ʱ��ID
            currentImageIndex = (currentImageIndex + 1) % images.size(); // �л�����һ��ͼƬ
            InvalidateRect(hwnd, NULL, TRUE); // �����ػ�
        }
    }
                 return 0;

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1: // ���к�ѡ��
            MessageBox(hwnd, TEXT("��ã�"), TEXT("���к�"), MB_OK);
            break;
        case 2: // �˳�ѡ��
            DestroyWindow(hwnd); // ���ٴ���
            break;
        case 3: // ����ѡ��
            MessageBox(hwnd, TEXT("��ã�"), TEXT("����ѡ��"), MB_OK);
            break;
        }
    }
         return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;

        HDC hdc = BeginPaint(hwnd, &ps);
        //FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        // 
        // ����һ���ڴ�DC��ʵ��˫����
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);
        SelectObject(hdcMem, hBitmap);
        // ��������ƴ�������
        if (!images.empty()) {
            // ����Ҫ��ʾ�Ŀ�Ⱥ͸߶�
            int oldWidth = images[currentImageIndex]->GetWidth();  // ԭ���
            int oldHeight = images[currentImageIndex]->GetHeight();  // ԭ�߶�
            int height = 200;  //   // ԭ�߶�
            int width = height * oldWidth / oldHeight;  // ԭ���
            MoveWindow(hwnd, showPos.x,showPos.y,width, height, TRUE); // �ƶ�����
            Gdiplus::Graphics graphics(hdcMem);
            graphics.Clear(Gdiplus::Color(0, 0, 0, 0)); // ����Ϊ͸������             
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);// ����ƽ��ģʽ
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);// ���ò�ֵģʽ
            // �Ŵ���
            int scaleX = 1.0f * width;
            int scaleY = 1.0f * height;
            //graphics.DrawImage(images[currentImageIndex], 0, 0, images[currentImageIndex]->GetWidth() * scaleX, images[currentImageIndex]->GetHeight() * scaleY);
            graphics.DrawImage(images[currentImageIndex], 0, 0, scaleX, scaleY);

            // graphics.DrawImage(images[currentImageIndex], 0, 0, width, height); // �ڴ����л���ͼ��

        }

        // ���ڴ�DC�����ݻ��Ƶ���ĻDC
        BitBlt(hdc, 0, 0, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, hdcMem, 0, 0, SRCCOPY);
        //BOOL BitBlt(
        //    HDC hdcDest,         // Ŀ���豸������
        //    int nXDest,          // Ŀ��λ�õ� X ����
        //    int nYDest,          // Ŀ��λ�õ� Y ����
        //    int nWidth,         // ���Ƶ�λͼ�Ŀ��
        //    int nHeight,        // ���Ƶ�λͼ�ĸ߶�
        //    HDC hdcSrc,         // Դ�豸������
        //    int nXSrc,          // Դλͼ�� X ����
        //    int nYSrc,          // Դλͼ�� Y ����
        //    DWORD dwRop         // ��դ����ѡ�ָ��������Դ��Ŀ������
        //);

        // ������Դ
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        EndPaint(hwnd, &ps);
    }
                 return 0;
                 //������
    case WM_LBUTTONDOWN: { // ����������
        // ��¼��갴��ʱ��λ�ã���������ק״̬
        isDragging = true;
        offset.x = LOWORD(lParam);
        offset.y = HIWORD(lParam);
        SetCapture(hwnd); // �����������
    }
    return 0;
    case WM_RBUTTONDOWN: { // �������Ҽ����
        // ��ȡ�����λ��
    
        ShowContextMenu(hwnd, showPos.x, showPos.y); // ��ʾ�����Ĳ˵�
    }
                       return 0;

    case WM_MOUSEMOVE: {
        // ���������ק�����´���λ��
        if (isDragging) {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            // ��ȡ��ǰ����λ��
            RECT rect;
            GetWindowRect(hwnd, &rect);
            showPos.x = rect.left + (x - offset.x);
            showPos.y = rect.top + (y - offset.y);
            // �����µ�λ��
            MoveWindow(hwnd, showPos.x, showPos.y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        // ֹͣ��ק���ͷ����
        isDragging = false;
        ReleaseCapture(); // �ͷŲ���
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // GDI+ ��ʼ��
    InitGDIPlus();

    // ע�ᴰ����
    TCHAR CLASS_NAME[] = TEXT("MyWindowClass");

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // �������ڣ�û�б߿�ͱ�������
    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED| WS_EX_TOOLWINDOW,                   //��չ��ʽ ��չ��ʽ���������
        CLASS_NAME,         //��������
        TEXT("�ͻ�������"),       //���ڱ���
        WS_POPUP,           //������ʽ���ޱ߿�
        CW_USEDEFAULT, CW_USEDEFAULT, 150, 300, //λ�úʹ�С
        NULL,               //�����ھ��
        NULL,               //�˵����
        hInstance,          //Ӧ�ó���ʵ�����
        NULL                //��������
    );

    // ���ö�ʱ��
    SetTimer(hwnd, 1, 500, NULL); // ����һ��1��Ķ�ʱ����IDΪ1
    // ���ô���Ϊ��͸��
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY); // 192Ϊ��͸���ȣ�ֵ��Χ0-255   ָ��������Ϊ�ı�־������������ֵ֮һ��������ϣ���
    //LWA_COLORKEY��crKey ������������ɫ��Ϊ͸��ɫ��
     //   LWA_ALPHA��ʹ�� bAlpha �������ô��ڵ�͸���ȡ�
     //   ע�⣺��������һ����־�����Ա�ʹ�ã�������ͬʱʹ�á�

     // ��ʾ����
     //ShowWindow(hwnd, nCmdShow);
    ShowWindow(hwnd, SW_HIDE); // ����������

    // ��Ϣѭ��
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    // GDI+ �ر�

    ShutdownGDIPlus(gdiplusToken);
    return 0;
}
