/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2003 Ulf Erikson <ulferikson@fastmail.fm>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>

#define PLUGIN_NAME "Mouse Gestures Plugin"
#define NO_OPTIONS "This plugin has no user configurable options."

#define KMELEON_PLUGIN_EXPORTS
#include "..\\kmeleon_plugin.h"
#include "..\\KMeleonConst.h"
#include "..\\resource.h"

#define _T(x) x

#define NOTFOUND -1


BOOL APIENTRY DllMain (
        HANDLE hModule,
        DWORD ul_reason_for_call,
        LPVOID lpReserved) {

  return TRUE;
}

LRESULT CALLBACK WndProc (
        HWND hWnd, UINT message,
        WPARAM wParam,
        LPARAM lParam);

void * KMeleonWndProc;

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);
void DoRebar(HWND rebarWnd);
int DoAccel(char *param);

kmeleonPlugin kPlugin = {
    KMEL_PLUGIN_VER,
    PLUGIN_NAME,
    DoMessage
};

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
    if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
        if (stricmp(subject, "Init") == 0) {
            Init();
        }
        else if (stricmp(subject, "Create") == 0) {
            Create((HWND)data1);
        }
        else if (stricmp(subject, "Config") == 0) {
            Config((HWND)data1);
        }
        else if (stricmp(subject, "Quit") == 0) {
            Quit();
        }
        else if (stricmp(subject, "DoMenu") == 0) {
            DoMenu((HMENU)data1, (char *)data2);
        }
        else if (stricmp(subject, "DoRebar") == 0) {
            DoRebar((HWND)data1);
        }
        else if (stricmp(subject, "DoAccel") == 0) {
            *(int *)data2 = DoAccel((char *)data1);
        }
        else return 0;

        return 1;
    }
    return 0;
}

UINT id_defercapture;

int Init(){
    id_defercapture = kPlugin.kFuncs->GetCommandIDs(1);
    return true;
}

void Create(HWND parent){
    KMeleonWndProc = (void *) GetWindowLong(parent, GWL_WNDPROC);
    SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
    MessageBox(parent, NO_OPTIONS, PLUGIN_NAME, 0);
}

void Quit(){
}

void DoMenu(HMENU menu, char *param){
}

int DoAccel(char *param) {
    return 0;
}

void DoRebar(HWND rebarWnd){
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef abs
#define abs(x) (((x)>0)?(x):(-(x)))
#endif

enum dir {NOMOVE, RIGHT, UPRIGHT, UP, UPLEFT, LEFT, DOWNLEFT, DOWN, DOWNRIGHT, BADMOVE};
typedef enum dir DIRECTION;

#define PREF_ "kmeleon.plugins.gestures."

DIRECTION findDir(POINT p1, POINT p2) {

    UINT MINDIST = 20;
    UINT MAXSLIP = 85;
    UINT SPLIT   = 50;

    kPlugin.kFuncs->GetPreference(PREF_INT, PREF_"mindist", &MINDIST, &MINDIST);
    kPlugin.kFuncs->GetPreference(PREF_INT, PREF_"maxslip", &MAXSLIP, &MAXSLIP);
    kPlugin.kFuncs->GetPreference(PREF_INT, PREF_"split", &SPLIT, &SPLIT);

    INT dx = p2.x - p1.x;
    INT dy = p2.y - p1.y;
    UINT dist = dx*dx + dy*dy;

    if (dist < MINDIST*MINDIST) return NOMOVE;
    if (dist < 3*MINDIST*MINDIST/2) return BADMOVE;

    double h = 0;
    double v = 90;
    double d = (v-h) / 2;

    double s1 = h + SPLIT * (d-h) / 100.0;
    double s2 = v + SPLIT * (d-v) / 100.0;

    double h_max = h + MAXSLIP * (s1-h) / 100.0;
    double v_min = v + MAXSLIP * (s2-v) / 100.0;

    double d_min = d + MAXSLIP * (s1-d) / 100.0;
    double d_max = d + MAXSLIP * (s2-d) / 100.0;

    double dir;
    if (dx == 0)
        dir = v;
    else
        dir = 2*v * abs(atan((double) dy / (double) dx)) / M_PI;

    if (dir <= h_max) {
        return dx > 0 ? RIGHT : LEFT;
    }
    else if (dir >= v_min) {
        return dy > 0 ? DOWN : UP;
    }
    else if (dir >= d_min && dir <= d_max) {
        if (dy > 0)
            return dx > 0 ? DOWNRIGHT : DOWNLEFT;
        else
            return dx > 0 ? UPRIGHT : UPLEFT;
    }

    return BADMOVE;
}

static UINT  m_captured;

static POINT m_posDown;
static SYSTEMTIME m_stDown;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
    if (message == WM_COMMAND){
        WORD command = LOWORD(wParam);

        if (command == id_defercapture) {
            SetCapture(hWnd);
        }
    }
    else if (message == WM_MOUSEACTIVATE){
        HWND hDesktopWnd = (HWND) wParam;
        UINT nHitTest = LOWORD(lParam);
        UINT mouseMsg = HIWORD(lParam);

        if (!m_captured && nHitTest == HTCLIENT) {
            if (mouseMsg == WM_RBUTTONDOWN) {
                if (GetKeyState(VK_SHIFT)   >= 0 &&
                    GetKeyState(VK_CONTROL) >= 0 &&
                    GetKeyState(VK_MENU)    >= 0) {

                    RECT rc = {0};
                    kPlugin.kFuncs->GetBrowserviewRect(hWnd, &rc);

                    GetCursorPos(&m_posDown);
                    if (m_posDown.x >= rc.left && m_posDown.x <= rc.right &&
                        m_posDown.y >= rc.top  && m_posDown.y <= rc.bottom) {

                        SetCapture(hWnd);
                        m_captured = mouseMsg;
                        GetSystemTime(&m_stDown);
                        PostMessage(hWnd, WM_COMMAND, id_defercapture, 0);

                        return MA_ACTIVATE;
                    }
                }
            }
        }
    }
    else if (message == WM_RBUTTONUP) {
        if (m_captured == WM_RBUTTONDOWN) {

            POINT m_posUp;
            GetCursorPos(&m_posUp);

            DIRECTION dir = findDir(m_posDown, m_posUp);

            SYSTEMTIME m_stUp;
            GetSystemTime(&m_stUp);

            ULONG u1 = m_stDown.wMilliseconds + 1000 * (m_stDown.wSecond + 60*(m_stDown.wMinute + 60*(m_stDown.wHour)));
            ULONG u2 = m_stUp.wMilliseconds + 1000 * (m_stUp.wSecond + 60*(m_stUp.wMinute + 60*(m_stUp.wHour)));

            UINT MAXTIME = 300;
            kPlugin.kFuncs->GetPreference(PREF_INT, PREF_"maxtime", &MAXTIME, &MAXTIME);

            if ( u2 - u1 > MAXTIME ) {
                if ( u2 - u1 > 3*MAXTIME/2 )
                    dir = NOMOVE;
                else
                    dir = (dir != NOMOVE ? BADMOVE : NOMOVE);
            }

            if (dir == NOMOVE) {
                PostMessage(GetFocus(), WM_CONTEXTMENU, (WPARAM) hWnd, MAKELONG(m_posUp.x, m_posUp.y));
            }
            else if (dir != BADMOVE) {
                char szDir[10][10] = { "nomove", "right", "upright", "up", "upleft", "left", "downleft", "down", "downright", "badmove" };

                char szPref[100];
                strcpy(szPref, PREF_);
                strcat(szPref, szDir[dir]);

                char szTxt[100];
                kPlugin.kFuncs->GetPreference(PREF_STRING, szPref, &szTxt, (char*)"");

                if (*szTxt) {
                    int id = kPlugin.kFuncs->GetID(szTxt);
                    if (id > 0)
                        SendMessage(hWnd, WM_COMMAND, id, 0L);
                }
            }

            ReleaseCapture();
            m_captured = 0;
            return 0;
        }
    }

    return CallWindowProc((WNDPROC)KMeleonWndProc, hWnd, message, wParam, lParam);
}

extern "C" {
    KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
        return &kPlugin;
    }
}