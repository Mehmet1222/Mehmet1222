#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#include <windows.h>
#include <tlhelp32.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <uxtheme.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cwchar>
#include <string>
#include <vector>
#include "control_logic.h"

namespace {
constexpr wchar_t kClass[] = L"MiranControl32Window";
constexpr wchar_t kDefaultFolder[] = L"C:\\Program Files\\Tencent\\GameLoop\\Application\\7.0.167.0\\GameLoopBox";
constexpr COLORREF BG = RGB(13, 20, 34), FG = RGB(232, 239, 250), MUTED = RGB(162, 177, 198);
constexpr COLORREF PANEL = RGB(25, 37, 56), GREEN = RGB(28, 142, 98), RED = RGB(182, 54, 65);
enum Id { Folder=100, Browse, OpenFolder, Refresh, Preset, Vertical, Horizontal, Interval,
          BothButtons, StartStop, Emergency, Save, ResetDefaults };
constexpr UINT_PTR TickTimer = 1, DetectTimer = 2;
constexpr int HotkeyStop = 1;

struct ProcessInfo { DWORD pid; std::wstring name; };
struct AppState {
    HWND window{}, title{}, subtitle{}, badge{}, folderLabel{}, folder{}, browse{}, openFolder{},
         detect{}, refresh{}, presetLabel{}, preset{}, verticalLabel{}, vertical{}, verticalValue{},
         horizontalLabel{}, horizontal{}, horizontalValue{}, intervalLabel{}, interval{}, intervalValue{},
         both{}, start{}, emergency{}, save{}, defaults{}, status{}, hint{}, disclosure{};
    HFONT normal{}, large{}, small{};
    HBRUSH background{}, panel{};
    miran::MotionConfig config;
    miran::Scheduler scheduler;
    std::vector<ProcessInfo> processes;
    std::wstring settingsPath, notice;
    bool enabled=false, hotkey=false, previousF8=false, previousForeground=false;
    DWORD lastForegroundPid=0;
    std::uint64_t moves=0;
    int dpi=96, scrollY=0;
} app;

int px(int value) { return MulDiv(value, app.dpi, 96); }
std::wstring text(HWND h) {
    int length=GetWindowTextLengthW(h);
    std::wstring value(static_cast<std::size_t>(length)+1, L'\0');
    GetWindowTextW(h, value.data(), length+1);
    value.resize(static_cast<std::size_t>(length));
    return value;
}
void label(HWND h, const std::wstring& value) { SetWindowTextW(h, value.c_str()); }
std::wstring folderPath() {
    auto value=text(app.folder);
    while (!value.empty() && (value.back()==L' ' || value.back()==L'\t')) value.pop_back();
    while (value.size()>3 && (value.back()==L'\\' || value.back()==L'/')) value.pop_back();
    return value;
}
bool directoryExists(const std::wstring& path) {
    DWORD attrs=GetFileAttributesW(path.c_str());
    return attrs!=INVALID_FILE_ATTRIBUTES && (attrs&FILE_ATTRIBUTE_DIRECTORY)!=0;
}
bool isGameLoopName(const wchar_t* name) {
    static const wchar_t* names[]={L"AppVPlayer.exe",L"aow_exe.exe",L"AndroidEmulatorEn.exe",
        L"AndroidEmulatorEx.exe",L"AndroidEmulator.exe",L"AndroidEmulatorExEn.exe",
        L"GameLoop.exe",L"GameLoopBox.exe",L"AppMarket.exe"};
    for (auto candidate:names) if (_wcsicmp(candidate,name)==0) return true;
    return false;
}
bool isEmulatorName(const wchar_t* name) {
    static const wchar_t* names[]={L"AppVPlayer.exe",L"aow_exe.exe",L"AndroidEmulatorEn.exe",
        L"AndroidEmulatorEx.exe",L"AndroidEmulator.exe",L"AndroidEmulatorExEn.exe"};
    for (auto candidate:names) if (_wcsicmp(candidate,name)==0) return true;
    return false;
}
bool foregroundIsGameLoop() {
    HWND current=GetForegroundWindow();
    if (!current || current==app.window || IsIconic(current)) return false;
    HWND root=GetAncestor(current,GA_ROOT);
    DWORD pid=0; GetWindowThreadProcessId(root?root:current,&pid);
    if (!pid) return false;
    // Revalidate the current PID's image on foreground changes and on every
    // movement decision; stale process-list entries must not permit PID reuse.
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    if (!process) return false;
    std::array<wchar_t,32768> path{}; DWORD size=static_cast<DWORD>(path.size());
    BOOL queried=QueryFullProcessImageNameW(process,0,path.data(),&size);
    CloseHandle(process);
    if (!queried) return false;
    const wchar_t* name=wcsrchr(path.data(),L'\\');
    return isEmulatorName(name?name+1:path.data());
}
void setStatus() {
    std::wstring value;
    if (!app.notice.empty()) value=(app.enabled?std::wstring(L"HAZIR  •  "):std::wstring(L"DURAKLATILDI  •  "))+app.notice;
    else if (!app.enabled) value=L"DURAKLATILDI  •  Fare hareketi gönderilmiyor.";
    else if (!foregroundIsGameLoop()) value=L"HAZIR  •  Hareket için GameLoop penceresini öne getir.";
    else value=app.config.both_buttons?L"AKTİF  •  Sol + sağ tuş basılıyken çalışır.":L"AKTİF  •  Sol tuş basılıyken çalışır.";
    label(app.status,value);
    label(app.start,app.enabled?L"DURDUR":L"BAŞLAT");
    InvalidateRect(app.start,nullptr,TRUE);
    InvalidateRect(app.status,nullptr,TRUE);
}
void stop(const std::wstring& message=L"") {
    app.enabled=false;
    app.scheduler.reset();
    app.notice=message;
    setStatus();
}
void detectProcesses(bool manual) {
    app.processes.clear();
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (snapshot!=INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry{}; entry.dwSize=sizeof(entry);
        if (Process32FirstW(snapshot,&entry)) {
            do {
                if (isGameLoopName(entry.szExeFile)) app.processes.push_back({entry.th32ProcessID,entry.szExeFile});
            } while (Process32NextW(snapshot,&entry));
        }
        CloseHandle(snapshot);
    }
    std::wstring value=app.processes.empty()?L"GAMELOOP BULUNAMADI":L"GAMELOOP AÇIK  •  "+std::to_wstring(app.processes.size())+L" ilgili süreç";
    label(app.detect,value);
    if (app.processes.empty() && app.enabled) stop(L"GameLoop kapandı veya süreç listesi alınamadı. Hareket durduruldu.");
    else if (manual) { app.notice=L"Süreç listesi yenilendi."; setStatus(); }
}
void syncSliders() {
    app.config.clamp();
    SendMessageW(app.vertical,TBM_SETPOS,TRUE,app.config.vertical);
    SendMessageW(app.horizontal,TBM_SETPOS,TRUE,app.config.horizontal);
    SendMessageW(app.interval,TBM_SETPOS,TRUE,app.config.interval_ms);
    SendMessageW(app.both,BM_SETCHECK,app.config.both_buttons?BST_CHECKED:BST_UNCHECKED,0);
    label(app.verticalValue,std::to_wstring(app.config.vertical)+L" birim");
    label(app.horizontalValue,(app.config.horizontal>0?L"+":L"")+std::to_wstring(app.config.horizontal)+L" birim");
    label(app.intervalValue,std::to_wstring(app.config.interval_ms)+L" ms");
}
void readSliders() {
    app.config.vertical=static_cast<int>(SendMessageW(app.vertical,TBM_GETPOS,0,0));
    app.config.horizontal=static_cast<int>(SendMessageW(app.horizontal,TBM_GETPOS,0,0));
    app.config.interval_ms=static_cast<int>(SendMessageW(app.interval,TBM_GETPOS,0,0));
    app.config.both_buttons=SendMessageW(app.both,BM_GETCHECK,0,0)==BST_CHECKED;
    app.config.clamp(); app.scheduler.reset();
    SendMessageW(app.preset,CB_SETCURSEL,3,0);
    syncSliders();
}
HWND createControl(const wchar_t* type,const wchar_t* title,DWORD style,int id=0) {
    HWND h=CreateWindowExW(0,type,title,WS_CHILD|WS_VISIBLE|style,0,0,1,1,app.window,
                          reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(app.normal),TRUE);
    return h;
}
HWND makeLabel(const wchar_t* value) { return createControl(L"STATIC",value,SS_LEFT); }
HWND makeButton(const wchar_t* value,int id) { return createControl(L"BUTTON",value,BS_OWNERDRAW|WS_TABSTOP,id); }
void place(HWND h,int x,int y,int w,int height) { MoveWindow(h,px(x),px(y-app.scrollY),std::max(1,px(w)),std::max(1,px(height)),TRUE); }
void layout() {
    RECT r{}; GetClientRect(app.window,&r);
    int viewportHeight=std::max(1,MulDiv(r.bottom,96,app.dpi));
    app.scrollY=std::clamp(app.scrollY,0,std::max(0,670-viewportHeight));
    SCROLLINFO scrolling{sizeof(scrolling),SIF_RANGE|SIF_PAGE|SIF_POS,0,669,static_cast<UINT>(viewportHeight),app.scrollY,0};
    SetScrollInfo(app.window,SB_VERT,&scrolling,TRUE);
    int width=MulDiv(r.right,96,app.dpi), margin=26, full=width-2*margin;
    place(app.title,margin,21,full-155,38); place(app.badge,width-160,31,134,24);
    place(app.subtitle,margin,67,full,24);
    place(app.detect,margin,107,full-110,25); place(app.refresh,width-128,102,102,30);
    place(app.folderLabel,margin,149,full,20);
    place(app.folder,margin,176,full-185,29);
    place(app.browse,width-202,176,78,29); place(app.openFolder,width-116,176,90,29);
    place(app.presetLabel,margin,225,110,24); place(app.preset,margin+120,220,full-120,180);
    auto row=[&](HWND a,HWND b,HWND c,int y) {
        place(a,margin,y+5,140,24); place(b,margin+142,y,full-250,36); place(c,width-126,y+5,100,24);
    };
    row(app.verticalLabel,app.vertical,app.verticalValue,268);
    row(app.horizontalLabel,app.horizontal,app.horizontalValue,316);
    row(app.intervalLabel,app.interval,app.intervalValue,364);
    place(app.both,margin,414,full,27);
    place(app.start,margin,461,170,43); place(app.emergency,margin+182,461,230,43);
    place(app.save,width-212,461,90,43); place(app.defaults,width-114,461,88,43);
    place(app.status,margin,527,full,42);
    place(app.hint,margin,579,full,35);
    place(app.disclosure,margin,624,full,25);
}
std::wstring settingsFile() {
    wchar_t base[MAX_PATH]{};
    if (FAILED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,base))) return L"";
    std::wstring folder=std::wstring(base)+L"\\MiranControl";
    if (!CreateDirectoryW(folder.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS) return L"";
    return folder+L"\\settings.ini";
}
void loadSettings() {
    app.settingsPath=settingsFile();
    std::wstring path=kDefaultFolder;
    if (!app.settingsPath.empty()) {
        app.config.vertical=static_cast<int>(GetPrivateProfileIntW(L"Motion",L"Vertical",5,app.settingsPath.c_str()));
        app.config.horizontal=static_cast<int>(GetPrivateProfileIntW(L"Motion",L"Horizontal",0,app.settingsPath.c_str()));
        app.config.interval_ms=static_cast<int>(GetPrivateProfileIntW(L"Motion",L"Interval",40,app.settingsPath.c_str()));
        app.config.both_buttons=GetPrivateProfileIntW(L"Motion",L"BothButtons",1,app.settingsPath.c_str())!=0;
        std::array<wchar_t,32768> saved{};
        GetPrivateProfileStringW(L"GameLoop",L"Folder",kDefaultFolder,saved.data(),static_cast<DWORD>(saved.size()),app.settingsPath.c_str());
        path=saved.data();
    }
    label(app.folder,path);
    syncSliders();
    SendMessageW(app.preset,CB_SETCURSEL,3,0);
}
bool writeAll(HANDLE file,const void* data,DWORD size) {
    DWORD done=0;
    return WriteFile(file,data,size,&done,nullptr) && done==size;
}
void saveSettings() {
    if (app.settingsPath.empty()) { app.notice=L"Ayar klasörü açılamadı; kayıt yapılmadı."; setStatus(); return; }
    std::wstring path=folderPath();
    if (path.empty() || path.find_first_of(L"\r\n")!=std::wstring::npos) {
        app.notice=L"Geçerli bir GameLoop klasör yolu gir."; setStatus(); return;
    }
    std::wstring content=L"[Motion]\r\nVertical="+std::to_wstring(app.config.vertical)+
        L"\r\nHorizontal="+std::to_wstring(app.config.horizontal)+L"\r\nInterval="+std::to_wstring(app.config.interval_ms)+
        L"\r\nBothButtons="+(app.config.both_buttons?std::wstring(L"1"):std::wstring(L"0"))+
        L"\r\n[GameLoop]\r\nFolder="+path+L"\r\n";
    std::wstring temp=app.settingsPath+L".tmp";
    HANDLE file=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    bool ok=file!=INVALID_HANDLE_VALUE;
    if (ok) {
        const wchar_t bom=0xFEFF;
        ok=writeAll(file,&bom,sizeof(bom)) && writeAll(file,content.data(),static_cast<DWORD>(content.size()*sizeof(wchar_t))) && FlushFileBuffers(file);
        CloseHandle(file);
    }
    if (ok) ok=MoveFileExW(temp.c_str(),app.settingsPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
    if (!ok) DeleteFileW(temp.c_str());
    app.notice=ok?L"Ayarlar kaydedildi. Program her açılışta duraklatılmış başlar.":L"Ayarlar kaydedilemedi; mevcut kayıt korundu.";
    setStatus();
}
int CALLBACK browseCallback(HWND hwnd,UINT msg,LPARAM,LPARAM data) {
    if (msg==BFFM_INITIALIZED) SendMessageW(hwnd,BFFM_SETSELECTIONW,TRUE,data);
    return 0;
}
void browseFolder() {
    auto initial=folderPath();
    BROWSEINFOW info{}; info.hwndOwner=app.window; info.lpszTitle=L"GameLoop klasörünü seç";
    info.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE; info.lpfn=browseCallback;
    info.lParam=reinterpret_cast<LPARAM>(initial.c_str());
    PIDLIST_ABSOLUTE selected=SHBrowseForFolderW(&info);
    if (selected) {
        wchar_t path[MAX_PATH]{};
        if (SHGetPathFromIDListW(selected,path)) label(app.folder,path);
        CoTaskMemFree(selected);
    }
}
void createUi() {
    HDC dc=GetDC(app.window); app.dpi=GetDeviceCaps(dc,LOGPIXELSX); ReleaseDC(app.window,dc);
    app.normal=CreateFontW(-px(15),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    app.large=CreateFontW(-px(30),0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    app.small=CreateFontW(-px(13),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    app.background=CreateSolidBrush(BG); app.panel=CreateSolidBrush(PANEL);
    app.title=makeLabel(L"MİRAN CONTROL"); SendMessageW(app.title,WM_SETFONT,reinterpret_cast<WPARAM>(app.large),TRUE);
    app.badge=makeLabel(L"32 BIT  /  x86");
    app.subtitle=makeLabel(L"GameLoop için ayarlanabilir fare hareketi");
    app.detect=makeLabel(L"GameLoop kontrol ediliyor…"); app.refresh=makeButton(L"Yenile",Refresh);
    app.folderLabel=makeLabel(L"GameLoop klasörü");
    app.folder=createControl(L"EDIT",kDefaultFolder,WS_TABSTOP|ES_AUTOHSCROLL|WS_BORDER,Folder);
    SendMessageW(app.folder,EM_SETLIMITTEXT,32000,0);
    app.browse=makeButton(L"Seç…",Browse); app.openFolder=makeButton(L"Klasörü aç",OpenFolder);
    app.presetLabel=makeLabel(L"Hazır ayar");
    app.preset=createControl(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,Preset);
    for (auto name:{L"Hafif  •  2 birim / 60 ms",L"Orijinal  •  5 birim / 40 ms",L"Güçlü  •  8 birim / 30 ms",L"Özel ayar"}) SendMessageW(app.preset,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));
    app.verticalLabel=makeLabel(L"Dikey hareket"); app.horizontalLabel=makeLabel(L"Yatay hareket"); app.intervalLabel=makeLabel(L"Hareket aralığı");
    app.vertical=createControl(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,Vertical);
    app.horizontal=createControl(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,Horizontal);
    app.interval=createControl(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,Interval);
    SendMessageW(app.vertical,TBM_SETRANGE,TRUE,MAKELPARAM(0,30));
    SendMessageW(app.horizontal,TBM_SETRANGE,TRUE,MAKELPARAM(-15,15));
    SendMessageW(app.interval,TBM_SETRANGE,TRUE,MAKELPARAM(10,200));
    app.verticalValue=makeLabel(L""); app.horizontalValue=makeLabel(L""); app.intervalValue=makeLabel(L"");
    app.both=createControl(L"BUTTON",L"Sol + sağ fare tuşu birlikte basılı olmalı",WS_TABSTOP|BS_AUTOCHECKBOX,BothButtons);
    SetWindowTheme(app.both,L"",L"");
    app.start=makeButton(L"BAŞLAT",StartStop); app.emergency=makeButton(L"ACİL DURDUR  •  F8",Emergency);
    app.save=makeButton(L"Kaydet",Save); app.defaults=makeButton(L"Sıfırla",ResetDefaults);
    app.status=makeLabel(L"");
    app.hint=makeLabel(L"Yalnızca GameLoop öndeyken çalışır. Alt+Tab, tuşları bırakma ve F8 hareketi keser.");
    app.disclosure=makeLabel(L"Bu sürüm fare hareketi uygular; RAM hedef taraması, aimbot veya ESP içermez.");
    SendMessageW(app.hint,WM_SETFONT,reinterpret_cast<WPARAM>(app.small),TRUE);
    SendMessageW(app.disclosure,WM_SETFONT,reinterpret_cast<WPARAM>(app.small),TRUE);
    loadSettings(); layout(); detectProcesses(false); setStatus();
    app.hotkey=RegisterHotKey(app.window,HotkeyStop,MOD_NOREPEAT,VK_F8)!=FALSE;
    SetTimer(app.window,TickTimer,10,nullptr); SetTimer(app.window,DetectTimer,1000,nullptr);
}
void onCommand(int id,int notification) {
    switch(id) {
    case Browse: browseFolder(); break;
    case OpenFolder: {
        auto path=folderPath();
        if (!directoryExists(path)) { app.notice=L"Bu klasör bulunamadı. Yolunu düzelt veya Seç düğmesini kullan."; setStatus(); break; }
        auto result=reinterpret_cast<INT_PTR>(ShellExecuteW(app.window,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL));
        if (result<=32) { app.notice=L"Klasör açılamadı."; setStatus(); }
        break;
    }
    case Refresh: detectProcesses(true); break;
    case Preset:
        if (notification==CBN_SELCHANGE) {
            int choice=static_cast<int>(SendMessageW(app.preset,CB_GETCURSEL,0,0));
            if (choice>=0 && choice<3) {
                const int vertical[]={2,5,8}, interval[]={60,40,30};
                app.config.vertical=vertical[choice]; app.config.interval_ms=interval[choice];
                app.config.horizontal=0; app.scheduler.reset(); syncSliders();
            }
        }
        break;
    case BothButtons: readSliders(); app.notice.clear(); setStatus(); break;
    case StartStop:
        if (app.enabled) stop();
        else {
            detectProcesses(false);
            if (app.processes.empty()) { app.notice=L"Önce GameLoop'u aç; sonra yeniden Başlat'a bas."; setStatus(); }
            else { app.enabled=true; app.notice.clear(); app.scheduler.reset(); setStatus(); }
        }
        break;
    case Emergency: stop(L"ACİL DURDURULDU  •  Yeniden çalıştırmak için Başlat'a bas."); break;
    case Save: saveSettings(); break;
    case ResetDefaults:
        stop(); app.config=miran::MotionConfig{}; label(app.folder,kDefaultFolder); syncSliders();
        SendMessageW(app.preset,CB_SETCURSEL,1,0); app.notice=L"Varsayılan ayarlar yüklendi. Kalıcı kayıt için Kaydet'e bas."; setStatus(); break;
    }
}
void onTick() {
    bool f8=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
    if (f8&&!app.previousF8) stop(L"F8 ile durduruldu. Yeniden çalıştırmak için Başlat'a bas.");
    app.previousF8=f8;
    if (!app.enabled) return;
    bool left=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0, right=(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0;
    bool foreground=foregroundIsGameLoop();
    if (foreground!=app.previousForeground) { app.previousForeground=foreground; if (app.notice.empty()) setStatus(); }
    auto movement=app.scheduler.tick(GetTickCount64(),app.enabled,foreground,left,right,app.config);
    if (!movement) return;
    // Recheck immediately before dispatch; queued timer messages never send a batch.
    if (!foregroundIsGameLoop()) { app.scheduler.reset(); return; }
    INPUT input{}; input.type=INPUT_MOUSE; input.mi.dx=movement->x; input.mi.dy=movement->y; input.mi.dwFlags=MOUSEEVENTF_MOVE;
    if (SendInput(1,&input,sizeof(input))!=1) stop(L"Fare girişi gönderilemedi. Hareket durduruldu; Windows erişim düzeyleri farklı olabilir.");
    else ++app.moves;
}
void drawButton(const DRAWITEMSTRUCT& item) {
    COLORREF color=PANEL;
    if (item.CtlID==StartStop) color=app.enabled?RED:GREEN;
    if (item.CtlID==Emergency) color=RGB(108,45,58);
    if ((item.itemState&ODS_SELECTED)!=0) color=RGB(GetRValue(color)*3/4,GetGValue(color)*3/4,GetBValue(color)*3/4);
    HBRUSH brush=CreateSolidBrush(color); FillRect(item.hDC,&item.rcItem,brush); DeleteObject(brush);
    SetBkMode(item.hDC,TRANSPARENT); SetTextColor(item.hDC,FG);
    HFONT old=static_cast<HFONT>(SelectObject(item.hDC,app.normal));
    auto caption=text(item.hwndItem); RECT r=item.rcItem;
    DrawTextW(item.hDC,caption.c_str(),static_cast<int>(caption.size()),&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SelectObject(item.hDC,old);
    if ((item.itemState&ODS_FOCUS)!=0) { InflateRect(&r,-3,-3); DrawFocusRect(item.hDC,&r); }
}
LRESULT CALLBACK windowProc(HWND window,UINT message,WPARAM wParam,LPARAM lParam) {
    switch(message) {
    case WM_CREATE: app.window=window; createUi(); return 0;
    case WM_SIZE: if (app.folder) layout(); return 0;
    case WM_GETMINMAXINFO: {
        auto info=reinterpret_cast<MINMAXINFO*>(lParam);
        RECT work{}; SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
        info->ptMinTrackSize={std::min<LONG>(px(800),work.right-work.left),std::min<LONG>(px(450),work.bottom-work.top)}; return 0;
    }
    case WM_COMMAND: onCommand(LOWORD(wParam),HIWORD(wParam)); return 0;
    case WM_HSCROLL: readSliders(); return 0;
    case WM_VSCROLL: {
        SCROLLINFO info{}; info.cbSize=sizeof(info);info.fMask=SIF_ALL;GetScrollInfo(window,SB_VERT,&info);
        int position=app.scrollY;
        switch (LOWORD(wParam)) {
        case SB_LINEUP: position-=24;break;case SB_LINEDOWN: position+=24;break;
        case SB_PAGEUP: position-=static_cast<int>(info.nPage);break;
        case SB_PAGEDOWN: position+=static_cast<int>(info.nPage);break;
        case SB_THUMBTRACK: position=info.nTrackPos;break;
        default: break;
        }
        app.scrollY=position;layout();InvalidateRect(window,nullptr,TRUE);return 0;
    }
    case WM_MOUSEWHEEL:
        app.scrollY-=static_cast<short>(HIWORD(wParam))/WHEEL_DELTA*60;
        layout();InvalidateRect(window,nullptr,TRUE);return 0;
    case WM_HOTKEY: if (wParam==HotkeyStop) stop(L"F8 ile durduruldu. Yeniden çalıştırmak için Başlat'a bas."); return 0;
    case WM_TIMER:
        if (wParam==TickTimer) onTick();
        else if (wParam==DetectTimer) detectProcesses(false);
        return 0;
    case WM_DRAWITEM: drawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam)); return TRUE;
    case WM_CTLCOLOREDIT: {
        auto dc=reinterpret_cast<HDC>(wParam); SetTextColor(dc,FG); SetBkColor(dc,PANEL); return reinterpret_cast<LRESULT>(app.panel);
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        auto dc=reinterpret_cast<HDC>(wParam); auto control=reinterpret_cast<HWND>(lParam);
        SetBkColor(dc,BG); SetTextColor(dc,(control==app.hint||control==app.disclosure||control==app.subtitle)?MUTED:FG);
        return reinterpret_cast<LRESULT>(app.background);
    }
    case WM_ERASEBKGND: { RECT r{};GetClientRect(window,&r);FillRect(reinterpret_cast<HDC>(wParam),&r,app.background);return 1; }
    case WM_CLOSE: stop(); DestroyWindow(window); return 0;
    case WM_DESTROY:
        app.enabled=false; KillTimer(window,TickTimer);KillTimer(window,DetectTimer);
        if (app.hotkey) UnregisterHotKey(window,HotkeyStop);
        for (auto font:{app.normal,app.large,app.small}) if (font) DeleteObject(font);
        if (app.background) DeleteObject(app.background);
        if (app.panel) DeleteObject(app.panel);
        PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,wParam,lParam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show) {
    static_assert(sizeof(void*)==4,"Miran Control must be built for 32-bit Windows (x86).");
    SetProcessDPIAware();
    HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_BAR_CLASSES|ICC_STANDARD_CLASSES}; InitCommonControlsEx(&controls);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=windowProc;wc.hInstance=instance;
    wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(nullptr,IDI_APPLICATION);wc.lpszClassName=kClass;
    if (!RegisterClassExW(&wc)) { if (SUCCEEDED(com)) CoUninitialize();return 1; }
    HDC dc=GetDC(nullptr);app.dpi=GetDeviceCaps(dc,LOGPIXELSX);ReleaseDC(nullptr,dc);
    RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    int width=static_cast<int>(std::min<LONG>(px(850),work.right-work.left));
    int height=static_cast<int>(std::min<LONG>(px(710),work.bottom-work.top));
    HWND window=CreateWindowExW(0,kClass,L"MİRAN CONTROL • 32 bit",WS_OVERLAPPEDWINDOW|WS_VSCROLL,
        work.left+std::max<LONG>(0,(work.right-work.left-width)/2),work.top+std::max<LONG>(0,(work.bottom-work.top-height)/2),width,height,
        nullptr,nullptr,instance,nullptr);
    if (!window) { if (SUCCEEDED(com)) CoUninitialize();return 1; }
    ShowWindow(window,show);UpdateWindow(window);
    MSG message{};
    while (GetMessageW(&message,nullptr,0,0)>0) {
        if (!IsDialogMessageW(window,&message)) { TranslateMessage(&message);DispatchMessageW(&message); }
    }
    if (SUCCEEDED(com)) CoUninitialize();
    return static_cast<int>(message.wParam);
}
