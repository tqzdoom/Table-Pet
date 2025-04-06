#include <windows.h>
#include <gdiplus.h>
#include <vector>
#pragma comment(lib, "gdiplus.lib")

#include <shellapi.h>
#define ID_TRAY_APP_ICON 5000 // 托盘图标的ID
#define ID_TRAY_EXIT      5001 // 退出菜单ID
#define ID_TRAY_GREET     5002 // 打招呼菜单的ID

using namespace Gdiplus;
ULONG_PTR gdiplusToken;  // 这里声明为全局变量
POINT offset; // 坐标偏移
static POINT showPos; // 显示位置
bool isDragging = false; // 拖拽状态

HWND hwnd; // 全局窗口句柄
// 初始化 GDI+
void InitGDIPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}
// 关闭 GDI+
void ShutdownGDIPlus(ULONG_PTR gdiplusToken) {
    Gdiplus::GdiplusShutdown(gdiplusToken);
}
//功能
void ShowContextMenu(HWND hwnd, int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 1, TEXT("打招呼"));
    AppendMenu(hMenu, MF_STRING, 3, TEXT("完善中"));
    AppendMenu(hMenu, MF_STRING, 2, TEXT("退出"));

    // 显示菜单
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, x, y, 0, hwnd, NULL);

    // 清理菜单
    DestroyMenu(hMenu);
}
// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static std::vector<Gdiplus::Image*> images; // 用于存储多张图片
    static int currentImageIndex = 0; // 当前显示的图片索引
    //static Gdiplus::Image* image = new Gdiplus::Image(L"1.png");
    static NOTIFYICONDATA nid = {};
    // 加载多张PNG图像

    images.push_back(new Gdiplus::Image(L"shime1.png"));
    images.push_back(new Gdiplus::Image(L"shime2.png"));
    images.push_back(new Gdiplus::Image(L"shime3.png"));

    // 检查图像加载状态
    for (size_t i = 0; i < images.size(); i++) {
        if (images[i]->GetLastStatus() != Gdiplus::Ok) {
            MessageBox(NULL, L"Unable to load one of the images", L"Error", MB_OK);
            delete images[i]; // 清理已加载的图像
            images.erase(images.begin() + i); // 移除未成功加载的图像
            i--; // 更新索引
        }
    }

    switch (uMsg) {
    case WM_CREATE: {
        // 添加托盘图标      
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = ID_TRAY_APP_ICON;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_APP + 1; // 自定义消息
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // 使用默认图标
        lstrcpy(nid.szTip, TEXT("自定义托盘应用程序")); // 鼠标悬停时显示的文本

        Shell_NotifyIcon(NIM_ADD, &nid); // 添加托盘图标
    }
                  return 0;
    case WM_APP + 1: // 自定义消息，处理托盘图标事件
        if (lParam == WM_LBUTTONDBLCLK) { // 双击左键
            ShowWindow(hwnd, SW_SHOW); // 显示主窗口
            SetForegroundWindow(hwnd); // 将窗口置于前景
        }
        else if (lParam == WM_RBUTTONDOWN) { // 右键点击
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(hwnd, pt.x, pt.y); // 显示上下文菜单
        }
        return 0;
    case WM_DESTROY:
        for (auto image : images) {
            delete image; // 清理图像资源
        }
        // 移除托盘图标
        Shell_NotifyIcon(NIM_DELETE, &nid);
        KillTimer(hwnd, 1); // 关闭定时器
        PostQuitMessage(0);
        return 0;
    case WM_TIMER: {
        if (wParam == 1) { // 确保这是我们设置的定时器ID
            currentImageIndex = (currentImageIndex + 1) % images.size(); // 切换到下一张图片
            InvalidateRect(hwnd, NULL, TRUE); // 请求重绘
        }
    }
                 return 0;

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1: // 打招呼选项
            MessageBox(hwnd, TEXT("你好！"), TEXT("打招呼"), MB_OK);
            break;
        case 2: // 退出选项
            DestroyWindow(hwnd); // 销毁窗口
            break;
        case 3: // 完善选项
            MessageBox(hwnd, TEXT("你好！"), TEXT("完善选项"), MB_OK);
            break;
        }
    }
         return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;

        HDC hdc = BeginPaint(hwnd, &ps);
        //FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        // 
        // 创建一个内存DC来实现双缓冲
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);
        SelectObject(hdcMem, hBitmap);
        // 在这里绘制窗口内容
        if (!images.empty()) {
            // 设置要显示的宽度和高度
            int oldWidth = images[currentImageIndex]->GetWidth();  // 原宽度
            int oldHeight = images[currentImageIndex]->GetHeight();  // 原高度
            int height = 200;  //   // 原高度
            int width = height * oldWidth / oldHeight;  // 原宽度
            MoveWindow(hwnd, showPos.x,showPos.y,width, height, TRUE); // 移动窗口
            Gdiplus::Graphics graphics(hdcMem);
            graphics.Clear(Gdiplus::Color(0, 0, 0, 0)); // 设置为透明背景             
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);// 设置平滑模式
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);// 设置插值模式
            // 放大倍数
            int scaleX = 1.0f * width;
            int scaleY = 1.0f * height;
            //graphics.DrawImage(images[currentImageIndex], 0, 0, images[currentImageIndex]->GetWidth() * scaleX, images[currentImageIndex]->GetHeight() * scaleY);
            graphics.DrawImage(images[currentImageIndex], 0, 0, scaleX, scaleY);

            // graphics.DrawImage(images[currentImageIndex], 0, 0, width, height); // 在窗口中绘制图像

        }

        // 将内存DC的内容绘制到屏幕DC
        BitBlt(hdc, 0, 0, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, hdcMem, 0, 0, SRCCOPY);
        //BOOL BitBlt(
        //    HDC hdcDest,         // 目标设备上下文
        //    int nXDest,          // 目标位置的 X 坐标
        //    int nYDest,          // 目标位置的 Y 坐标
        //    int nWidth,         // 复制的位图的宽度
        //    int nHeight,        // 复制的位图的高度
        //    HDC hdcSrc,         // 源设备上下文
        //    int nXSrc,          // 源位图的 X 坐标
        //    int nYSrc,          // 源位图的 Y 坐标
        //    DWORD dwRop         // 光栅操作选项，指定如何组合源和目标像素
        //);

        // 清理资源
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        EndPaint(hwnd, &ps);
    }
                 return 0;
                 //鼠标操作
    case WM_LBUTTONDOWN: { // 监听左键点击
        // 记录鼠标按下时的位置，并设置拖拽状态
        isDragging = true;
        offset.x = LOWORD(lParam);
        offset.y = HIWORD(lParam);
        SetCapture(hwnd); // 捕获鼠标输入
    }
    return 0;
    case WM_RBUTTONDOWN: { // 监听点右键点击
        // 获取鼠标点击位置
    
        ShowContextMenu(hwnd, showPos.x, showPos.y); // 显示上下文菜单
    }
                       return 0;

    case WM_MOUSEMOVE: {
        // 如果正在拖拽，更新窗口位置
        if (isDragging) {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            // 获取当前窗口位置
            RECT rect;
            GetWindowRect(hwnd, &rect);
            showPos.x = rect.left + (x - offset.x);
            showPos.y = rect.top + (y - offset.y);
            // 计算新的位置
            MoveWindow(hwnd, showPos.x, showPos.y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        // 停止拖拽并释放鼠标
        isDragging = false;
        ReleaseCapture(); // 释放捕获
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // GDI+ 初始化
    InitGDIPlus();

    // 注册窗口类
    TCHAR CLASS_NAME[] = TEXT("MyWindowClass");

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // 创建窗口（没有边框和标题栏）
    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED| WS_EX_TOOLWINDOW,                   //扩展样式 扩展样式：层叠窗口
        CLASS_NAME,         //窗口类名
        TEXT("客户区窗口"),       //窗口标题
        WS_POPUP,           //窗口样式：无边框
        CW_USEDEFAULT, CW_USEDEFAULT, 150, 300, //位置和大小
        NULL,               //父窗口句柄
        NULL,               //菜单句柄
        hInstance,          //应用程序实例句柄
        NULL                //附加数据
    );

    // 设置定时器
    SetTimer(hwnd, 1, 500, NULL); // 设置一个1秒的定时器，ID为1
    // 设置窗口为半透明
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY); // 192为不透明度，值范围0-255   指定函数行为的标志，可以是以下值之一（或者组合）：
    //LWA_COLORKEY：crKey 参数包含的颜色作为透明色。
     //   LWA_ALPHA：使用 bAlpha 参数设置窗口的透明度。
     //   注意：以上任意一个标志都可以被使用，但不能同时使用。

     // 显示窗口
     //ShowWindow(hwnd, nCmdShow);
    ShowWindow(hwnd, SW_HIDE); // 隐藏主窗口

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    // GDI+ 关闭

    ShutdownGDIPlus(gdiplusToken);
    return 0;
}
