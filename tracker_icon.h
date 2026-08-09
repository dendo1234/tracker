#include "tracker_common.h"

#include <stdio.h>
#include <windows.h>
#include <Windowsx.h>

global_variable HMENU g_menu;

static LRESULT icon_callback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;

  switch (message) {
    case WM_USER: {
      // NOTE: icon notification
      UINT event = LOWORD(lParam);
      // UINT icon_id = HIWORD(lParam);

      int x = GET_X_LPARAM(wParam); 
      int y = GET_Y_LPARAM(wParam); 

      switch (event) {
        case WM_CONTEXTMENU: {
          // NOTE: must be foreground window before calling TrackPopupMenuEx
          SetForegroundWindow(window);


          UINT alignment_anim;
          if (GetSystemMetrics(SM_MENUDROPALIGNMENT)) {
            alignment_anim = TPM_RIGHTALIGN | TPM_HORPOSANIMATION;
          } else {
            alignment_anim = TPM_LEFTALIGN | TPM_HORNEGANIMATION;
          }

          if (TrackPopupMenuEx(
            g_menu,
            alignment_anim 
            | TPM_TOPALIGN 
            // | TPM_RETURNCMD // TPM_NONOTIFY
            | TPM_LEFTBUTTON
            | TPM_HORIZONTAL,
            x,
            y,
            window,
            0
          ) == 0) {
            TrackerError error;
            had_error(error)
            show_error_and_quit_message(error, "Failed to show NotifyIcon popup menu");
          }
        } break;
      }
    } break;
    case WM_COMMAND: {
      // NOTE: suposing we only have Exit tracker notifyIcon button:

      PostQuitMessage(0);
    } break;

    default: {
      result = DefWindowProc(window, message, wParam, lParam);
    }
  }

  return result;
}

internal TrackerError icon_create(void)  {
  TrackerError error = {};

  g_menu = CreatePopupMenu();
  if (g_menu) {
    // NOTE: menu created
    if (AppendMenuW(
        g_menu,
        MF_ENABLED | MF_STRING | MF_UNCHECKED,
        0,
        L"Exit tracker"
    )) {

      WNDCLASSEXW wnd_class = {
        .cbSize = sizeof(WNDCLASSEXW),
        .style = 0,
        .lpfnWndProc = icon_callback,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = GetModuleHandleA(0),
        .hIcon = 0,
        .hCursor = 0,
        .hbrBackground = 0,
        .lpszMenuName = 0,
        .lpszClassName = L"WindowIconClass",
        .hIconSm = 0,
      };
      if (RegisterClassExW(&wnd_class)) {
        // NOTE: NotifyIcon window class registered
        HWND window_handler = CreateWindowExW(WS_EX_TOOLWINDOW, wnd_class.lpszClassName, L"tracker notify icon", WS_ICONIC, 
                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandleA(0), 0);

        if (window_handler) {
          UINT has_icon = 0;

          HICON icon = (HICON)LoadImageA(
            0,
            (LPCSTR)IDI_APPLICATION,
            IMAGE_ICON,
            0,
            0,
            LR_DEFAULTSIZE | LR_SHARED
          );
          if (icon) {
            has_icon = NIF_ICON;
          } else {
            had_error(error);
          }


          NOTIFYICONDATAW nid = {
            .cbSize = sizeof(NOTIFYICONDATAW),
            .hWnd = window_handler,
            .uID = 0,
            .uFlags = NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP | has_icon,
            .uCallbackMessage = WM_USER,
            .hIcon = icon,
            .uVersion = NOTIFYICON_VERSION_4,
          };
          _snwprintf_s(nid.szTip, sizeof(nid.szTip), sizeof(nid.szTip), L"tracker v%u", VERSION);

          if (Shell_NotifyIconW(NIM_ADD, &nid)) {
            if (Shell_NotifyIconW(NIM_SETVERSION, &nid)) {
                OutputDebugStringA("Created NotifyIcon and set NOTIFYICON_VERSION_4\n");
            } else {
              had_error(error)
            }
          } else {
            had_error(error)
          }
        } else {
          had_error(error)
        }
      } else {
        had_error(error)
      }
    } else {
      had_error(error)
    }
  } else {
    had_error(error)
  }

  return error;
}
