.-/*
*  Copyright (C) 2005 Dorian Boissonnade
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/



/* 
Menu to use
Load Session{
session()
}

&Sessions{
:Load Session
sessions(save, Save Session)
sessions(undo, Undo Last Closed)
}

Supported accel

sessions(save)
sessions(undo)
*/

#include "stdafx.h"
#include <stdlib.h>
#include <atlconv.h>

#define KMELEON_PLUGIN_EXPORTS
#include "..\kmeleon_plugin.h"
#include "..\kmeleonconst.h"
#include "..\utils.h"
#include "..\LocalesUtils.h"
#include "..\strconv.h"

#include "resource.h"

#define _Tr(x) kPlugin.kFuncs->Translate(_T(x))

#define PLUGIN_NAME "Session Saver Plugin"

#define PREFERENCE_SESSION_AUTOLOAD   "kmeleon.plugins.sessions.autoload"
#define PREFERENCE_SESSION_OPENSTART   "kmeleon.plugins.sessions.openStart"
#define PREFERENCE_SESSION_ASKAUTOLOAD   "kmeleon.plugins.sessions.ask_autoload"
#define PREFERENCE_SESSION_MAXUNDO   "kmeleon.plugins.sessions.maxUndo"
#define PREFERENCE_CLEANSHUTDOWN "kmeleon.plugins.sessions.cleanShutdown"
char* kPreviousSessionName = "Previous Session";
char* kLastSessionName = "Last Session";

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

#include "sessions.h"
bool Session::loading = false;

Session currentSession;
Session undo;

// This limit to 100 the number of saved session
#define MAX_SAVED_SESSION 100

int Load();
void Create(HWND parent);
void Destroy(HWND parent);
void CreateTab(HWND parent, HWND tab);
void DestroyTab(HWND parent, HWND tab);
void MoveTab(HWND parent, HWND tab);

void Config(HWND parent);
void Close(HWND parent);
void DoMenu(HMENU menu, char *param);
int DoAccel(char *param);
void Undo(HWND hWnd);
void Quit();

kmeleonFunctions *kFuncs;
WNDPROC KMeleonWndProc;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int id_undo_close;
int id_save_session;
int id_load_session;
int id_delete_session;
int last_session_id;
int id_config;

int bFirstStart = 1;
//int gMaxUndo = 5;
bool gLoading = false;
Locale* gLoc = NULL;

char* sessions_list[MAX_SAVED_SESSION] = {0};

HMENU sessionsMenu = NULL;



void BuildSessionMenu()
{
	if (!IsMenu(sessionsMenu)) return;

   int count;
	while (count = GetMenuItemCount(sessionsMenu))
		DeleteMenu(sessionsMenu, 0, MF_BYPOSITION);

	last_session_id = id_load_session;

	for(int i=0;sessions_list[i];i++)
	{
		if (stricmp(sessions_list[i], kLastSessionName)!=0) 
			AppendMenuA(sessionsMenu, MF_STRING, last_session_id, sessions_list[i]);
		last_session_id++;
		if (last_session_id>id_load_session + MAX_SAVED_SESSION) break;
	}
}

// Have to use my own, because oji initialisation use the crt one
char* _strtok (char * string, const char * control)
{
   static char* nextoken;

   if (string)
      nextoken = string;

   char *tok;
   if ( (tok = strstr(nextoken, control)) ) {
      *tok = 0;
      char *ret = nextoken;
      nextoken = tok + strlen(control);
      return ret;
   }
   else return 0;
}

void GetSessionList()
{
   int len = kFuncs->GetPreference(PREF_STRING, "kmeleon.plugins.sessions2.list", NULL, NULL);
   char *sessionsList = new char[len+1];
   kFuncs->GetPreference(PREF_STRING, "kmeleon.plugins.sessions2.list", sessionsList, _T(""));

   int i = 0;
   char* token = strtok(sessionsList, ",");
   while (token) {
	   if (i>=MAX_SAVED_SESSION) break;
		sessions_list[i++] = strdup(token);
		token = strtok(NULL, ",");
   }
   delete [] sessionsList;
}

void WriteSessionList()
{
	char *sessionsList = new char[4096];
	sessionsList[0]=0;
	for(int i=0;sessions_list[i];i++)
	{
		strcat(sessionsList,sessions_list[i]);
		strcat(sessionsList,",");
	}
	
	kFuncs->SetPreference(PREF_STRING, "kmeleon.plugins.sessions2.list", sessionsList, TRUE);
	delete [] sessionsList;
}

void AddSessionList(const char* name)
{
	bool bFound = false;

	int i;
	for(i=0;sessions_list[i];i++)
	{
		if (strcmp(name, sessions_list[i]) == 0) {
            bFound = true;
		}
	}

	if (!bFound)
	{
		sessions_list[i] = strdup(name);
		WriteSessionList();
	}
}


void DeleteSession(const char* name)
{

	currentSession.deleteSession(name);/*
	char * prefname = new char[strlen(name)+ 26];
	prefname[0]=0;
	strcat(prefname, "kmeleon.plugins.sessions.");
	strcat(prefname, name);

	kFuncs->DelPreference(prefname);
	delete [] prefname;*/

	for(int i=0;sessions_list[i];i++)
	{
		if (strcmp(name, sessions_list[i]) == 0) {
			delete sessions_list[i];
            break;
		}
	}
	
	for(;sessions_list[i+1];i++)
		sessions_list[i] = sessions_list[i+1];

	sessions_list[i] = NULL;

	WriteSessionList();
}

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Load") == 0) {
         return Load();
      }
      else if (stricmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
	  else if (stricmp(subject, "Destroy") == 0) {
         Destroy((HWND)data1);
      }
	  else if (stricmp(subject, "CreateTab") == 0) {
         CreateTab((HWND)data1, (HWND)data2);
      }
	  else if (stricmp(subject, "DestroyTab") == 0) {
         DestroyTab((HWND)data1, (HWND)data2);
      }
	  else if (stricmp(subject, "MoveTab") == 0) {
         MoveTab((HWND)data1, (HWND)data2);
      }
      else if (stricmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
	  else if (stricmp(subject, "DoAccel") == 0) {
         *(int *)data2 = DoAccel((char *)data1);
      }
	  else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
	  else if (stricmp(subject, "Undo") == 0) {
		 Undo(0);
	  }
	  else if (stricmp(subject, "Quit") == 0) {
         Quit();
      }
      else return 0;

      return 1;
   }
   return 0;
}




HINSTANCE ghInstance;
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
   switch (ul_reason_for_call) {
      case DLL_PROCESS_ATTACH:
         ghInstance = (HINSTANCE) hModule;
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
      break;
   }
   return TRUE;
}

int Load() {
   kFuncs = kPlugin.kFuncs;

   // Not compatible with the layers plugin.
   kmeleonPlugin* layers = kFuncs->Load("layers.dll");
   if (layers && layers->loaded) {
      kPlugin.description = PLUGIN_NAME " [Not Compatible With Layers]";
      return -1;
   }

   gLoc = Locale::kmInit(&kPlugin);

   kPreviousSessionName = strdup(CT_to_UTF8(gLoc->GetString(IDS_PREVIOUS_SESSION)));
   kLastSessionName = strdup(CT_to_UTF8(gLoc->GetString(IDS_LAST_SESSION)));

   id_undo_close = kPlugin.kFuncs->GetCommandIDs(1);
   id_save_session = kPlugin.kFuncs->GetCommandIDs(1);
   id_config = kPlugin.kFuncs->GetCommandIDs(1);
   last_session_id = id_load_session = kPlugin.kFuncs->GetCommandIDs(MAX_SAVED_SESSION); 
   //id_delete_session = kPlugin.kFuncs->GetCommandIDs(MAX_SAVED_SESSION); 
   
   // This session list is really some bad stuff because
   // plugins can't enumerate preferences
   GetSessionList();
   AddSessionList(kPreviousSessionName);
   return 1;
}

#define PWM_AUTOLOAD WM_APP + 1000
#define PWM_HACKRESTORE WM_APP + 1001

void CreateTab(HWND parent, HWND tab)
{
	currentSession.addTab(parent, tab);
}

void MoveTab(HWND newtab, HWND oldtab)
{
	Window w = currentSession.findWindowWithTab(newtab);
	w.moveTab(newtab, oldtab);
}

HWND destroying = NULL;

void DestroyTab(HWND parent, HWND tab)
{
	if (destroying != parent) {
		Window w = currentSession.getWindow(parent);
		Tab t = w.getTab(tab);
	
		w = Window(NULL);
		w.addTab(t);
		undo.addWindow(w);
	}
	
    currentSession.removeTab(parent, tab);	
}

void Create(HWND hWndParent) {

	if (IsWindowUnicode(hWndParent))
	{
		KMeleonWndProc = (WNDPROC) GetWindowLongW(hWndParent, GWL_WNDPROC);
		SetWindowLongW(hWndParent, GWL_WNDPROC, (LONG)WndProc);
	}
	else
	{
		KMeleonWndProc = (WNDPROC) GetWindowLongA(hWndParent, GWL_WNDPROC);
		SetWindowLongA(hWndParent, GWL_WNDPROC, (LONG)WndProc);
	}

	currentSession.addWindow(hWndParent);

	// Post a message for autoload after the creation of 
	// the frame is done.
	if (bFirstStart) {
		bFirstStart = 0;

		// Look for a bad shutdown
		int ok = IDNO;
		int clean = true;
		kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CLEANSHUTDOWN, &clean, &clean);
		if (!clean) {
			
			ok = MessageBox(NULL, 
				gLoc->GetString(IDS_SESSION_RECOVERY_MSG),
				gLoc->GetString(IDS_SESSION_RECOVERY),
				 MB_YESNO|MB_ICONQUESTION);

			if (ok == IDYES) PostMessage(hWndParent, PWM_AUTOLOAD, 1, 0);
		}
		
		clean = false;
		kFuncs->SetPreference(PREF_BOOL, PREFERENCE_CLEANSHUTDOWN, &clean, TRUE);

		if (ok != IDYES) {
		// Ask to load the startup the session, but the session is actually
		// loaded later because we can't open a window until one is fully created
		BOOL b = FALSE;
		kFuncs->GetPreference(PREF_BOOL, PREFERENCE_SESSION_AUTOLOAD, (void*)&b, (void*)&b);
		if (b) {
				// Get the start session name
			char *name = new char[256];
			name[0] = 0;
			kFuncs->GetPreference(PREF_STRING, PREFERENCE_SESSION_OPENSTART, name, "");

			// If it's not empty
			if (strlen(name)>0) 
			{
				ok = IDYES;
				kFuncs->GetPreference(PREF_BOOL, PREFERENCE_SESSION_ASKAUTOLOAD, (void*)&b, (void*)&b);
				if (b)
				{	
					// Ask to load the session
					ok = MessageBox(NULL, 
						gLoc->GetStringFormat(IDS_STARTUP_SESSION_MSG, (const TCHAR*)CANSI_to_T(name)),
						gLoc->GetString(IDS_STARTUP_SESSION),
						MB_YESNO|MB_ICONQUESTION);
				}
				if (ok == IDYES) PostMessage(hWndParent, PWM_AUTOLOAD, 0, 0);
			}
			delete [] name;
		}
		}
	}

	// Can't use show directly or else the history plugin crash kmeleon
	/*if (gStateForNextWindow!=-1)
		PostMessage(hWndParent, PWM_HACKRESTORE, gStateForNextWindow, 0);*/

	//ShowWindow(hWndParent, gStateForNextWindow);
}

void Destroy(HWND hWnd) {
	destroying = hWnd;
	currentSession.removeWindow(hWnd);
	undo.addWindow(currentSession.getWindow(hWnd));

	int gMaxUndo = 5;
	kFuncs->GetPreference(PREF_INT, PREFERENCE_SESSION_MAXUNDO, (void*)&gMaxUndo, (void*)&gMaxUndo);
	undo.limit(gMaxUndo);
}

void DoMenu(HMENU menu, char *param){
   if (*param != 0){
      char *action = param;
      char *string = strchr(param, ',');
      if (string) {
         *string = 0;
         string = SkipWhiteSpace(string+1);
      }
      else
         string = action;

	  int command = 0;
      if (stricmp(action, "Save") == 0){
         command = id_save_session;
		 AppendMenuA(menu, MF_STRING, command, string);
      }
	  else if (stricmp(action, "Load") == 0){
		  if (sessionsMenu) return;
	     sessionsMenu = CreateMenu();
		  BuildSessionMenu();
		  AppendMenuA(menu, MF_POPUP, (UINT)sessionsMenu, string);
	  }
	  /*else if (stricmp(action, "Delete") == 0){
		  AppendMenu(menu, MF_POPUP, (UINT)sessionsMenu, string);
	  }*/
      else if (stricmp(action, "Undo") == 0){
         command = id_undo_close;
		 AppendMenuA(menu, MF_STRING, command, string);
      }
   }
	else {
		//if (sessionsMenu) return;
		sessionsMenu = menu;
		BuildSessionMenu();
	}


}

int DoAccel(char *param)
{
   if (stricmp(param, "Save") == 0)
      return id_save_session;
   if (stricmp(param, "Undo") == 0)
      return id_undo_close;
   if (stricmp(param, "Config") == 0)
      return id_config;
   return 0;
}

void Undo(HWND hWnd = NULL)
{
	Window w = undo.lastWindow();
	if (w.hWnd) w.open();
	else  {
		Tab t = w.lastTab();
		t.setParent(hWnd);
		t.open(false);
	}
}

void Quit()
{
	//SaveSession(kLastSessionName);
	currentSession.saveSession(kPreviousSessionName);
	currentSession.empty();
	undo.empty();

	for(int i=0;sessions_list[i];i++)
		delete sessions_list[i];

	bool b = true;
    kFuncs->SetPreference(PREF_BOOL, PREFERENCE_CLEANSHUTDOWN, &b, TRUE);
	delete kPreviousSessionName;
	delete kLastSessionName;
	delete gLoc;
}

BOOL CALLBACK
ConfigDlgProc( HWND hwnd,
			  UINT Message,
			  WPARAM wParam,
			  LPARAM lParam )
{
	switch (Message) {
	  case WM_INITDIALOG: {
			char* name = new char[256];
   	 		kFuncs->GetPreference(PREF_STRING, PREFERENCE_SESSION_OPENSTART, name, "");
			
		  for (int i=0;sessions_list[i];i++)
		  {
			  int index;
			  USES_CONVERSION;
			  index = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_ADDSTRING, 
				0, (LPARAM)(LPTSTR)A2T(sessions_list[i])); 
			  SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_SETITEMDATA, 
				index, (LPARAM)i);
			  if (strcmp(name, sessions_list[i]) == 0)
				  SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_SETCURSEL, index, 0); 

			  index = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_ADDSTRING, 
				0, (LPARAM)(LPTSTR)A2T(sessions_list[i])); 
			  SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_SETITEMDATA, 
				index, (LPARAM)i); 

		  }

			delete name;

		  int b=0;
		  kFuncs->GetPreference(PREF_BOOL, PREFERENCE_SESSION_AUTOLOAD, (void*)&b, (void*)&b);
		  CheckDlgButton(hwnd, IDC_CHECK_AUTOLOAD, b);
		  b=0;
		  kFuncs->GetPreference(PREF_BOOL, PREFERENCE_SESSION_ASKAUTOLOAD, (void*)&b, (void*)&b);
		  CheckDlgButton(hwnd, IDC_CHECK_ASK, b);

		  int gMaxUndo = 5;
		  kFuncs->GetPreference(PREF_INT, PREFERENCE_SESSION_MAXUNDO, (void*)&gMaxUndo, (void*)&gMaxUndo);
		  SetDlgItemInt(hwnd, IDC_EDIT_MAXUNDO, gMaxUndo, FALSE);
		  return TRUE;
		 }

	  case WM_COMMAND:

		  switch (LOWORD(wParam)) {
			case IDOK: {
				int i = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_GETITEMDATA, SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_GETCURSEL, 0, 0), 0); 
				if (i!=-1) 
					kFuncs->SetPreference(PREF_STRING, PREFERENCE_SESSION_OPENSTART, sessions_list[i], FALSE);
				else
					kFuncs->SetPreference(PREF_STRING, PREFERENCE_SESSION_OPENSTART, "", FALSE);
				int b = IsDlgButtonChecked(hwnd, IDC_CHECK_AUTOLOAD);
				kFuncs->SetPreference(PREF_BOOL, PREFERENCE_SESSION_AUTOLOAD, (void*)&b, FALSE);
				b = IsDlgButtonChecked(hwnd, IDC_CHECK_ASK);
				kFuncs->SetPreference(PREF_BOOL, PREFERENCE_SESSION_ASKAUTOLOAD, (void*)&b, FALSE);
				int gMaxUndo = GetDlgItemInt(hwnd, IDC_EDIT_MAXUNDO, NULL, FALSE);
				kFuncs->SetPreference(PREF_INT, PREFERENCE_SESSION_MAXUNDO, (void*)&gMaxUndo, FALSE);
				EndDialog( hwnd, IDOK );
				}
				break;

			case IDCANCEL:
				EndDialog( hwnd, IDCANCEL );
				break;

			case IDC_BUTTON_DELETE: {
				int index = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_GETCURSEL, 0, 0);
				int i = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_GETITEMDATA, index , 0); 
				
				DeleteSession(sessions_list[i]);
				BuildSessionMenu();
				
				i = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_GETITEMDATA, SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_GETCURSEL, 0, 0), 0); 
				char* name = sessions_list[i];

				SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_RESETCONTENT, 0, 0);
				SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_RESETCONTENT, 0, 0);



		  for (int i=0;sessions_list[i];i++)
		  {
			  int index;
			  index = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_ADDSTRING, 
				0, (LPARAM) sessions_list[i]); 
			  SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_SETITEMDATA, 
				index, (LPARAM)i);
			  if (name && strcmp(name, sessions_list[i]) == 0)
				  SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_SETCURSEL, index, 0); 

			  index = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_ADDSTRING, 
				0, (LPARAM) sessions_list[i]); 
			  SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_SETITEMDATA, 
				index, (LPARAM)i); 

		  }

			
			}
				break;
		  }
		  break;

	  default:
		  return FALSE;
	}
	return TRUE;
}

void Config(HWND parent){
   gLoc->DialogBoxParam(MAKEINTRESOURCE(IDD_CONFIG), parent, (DLGPROC)ConfigDlgProc);
}

// Callback for the dialog answering a session name
BOOL CALLBACK
PromptDlgProc( HWND hwnd,
			  UINT Message,
			  WPARAM wParam,
			  LPARAM lParam )
{
	static TCHAR* answer;

	switch (Message) {
	  case WM_INITDIALOG: 
		  answer = (TCHAR*)lParam;
		  return TRUE;
	  case WM_COMMAND:
		  switch (LOWORD(wParam)) {
	  case IDOK:
		  GetDlgItemText(hwnd, IDC_ANSWER, answer, 256);
		  if ( !_tcscmp(answer, _T("list")) || !_tcscmp(answer, CUTF8_to_T(kLastSessionName)) || _tcschr(answer, _T(',')) )
			  MessageBox(hwnd, gLoc->GetString(IDS_INVALID_NAME), _T(""), MB_OK|MB_ICONERROR);
		  else
			  EndDialog( hwnd, IDOK );
		  break;
	  case IDCANCEL:
		  EndDialog( hwnd, IDCANCEL );
		  break;
		  }
		  break;

	  default:
		  return FALSE;
	}
	return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   struct frame* pFrame;

   switch (message) {
		
	   case PWM_HACKRESTORE:
		/*   if (wParam  == SW_SHOWNOACTIVATE)
		   {
			HWND active = GetActiveWindow();
			
			ShowWindow(hWnd, wParam);
			
			// Restore only if the last active is a kmeleon window
			if (pFrame = findFrame(active))
				SetActiveWindow(active);
			
		   }
		   else*/
			   ShowWindow(hWnd, wParam);

			break;

		case PWM_AUTOLOAD: {

			// This should be called at start when the first window is created
			// We can't create a window until one is done (I think) so I load 
			// the start session here.
			char name[256];
			char url[2048];
			char homepage[2048];

			name[0] = 0;
			if (wParam) // Loading last session after crash
				strcpy(name, kLastSessionName);
			else // Loading defined start session in pref
				kFuncs->GetPreference(PREF_STRING, PREFERENCE_SESSION_OPENSTART, name, "");
			
			kFuncs->GetPreference(PREF_STRING, "kmeleon.general.homePage", homepage, "");
			
			url[0] = 0;
			//if (!kFuncs->GetGlobalVar(PREF_STRING, "URL", url))
				kFuncs->GetGlobalVar(PREF_STRING, "URLBAR", url);
			
			char *cmd = GetCommandLineA();
			bool keep = strstr(cmd, "http://") || strstr(cmd, "-url");
//			bool keep = strcmp(url, "about:blank") && strcmp(url, homepage);
			Session load;
			Session::loading = true;
			
			if (load.loadSession(name)) {
				currentSession.close_except(hWnd);
				if (load.open() && !keep)
					currentSession.close(hWnd);
			}

			Session::loading = false;
			if (keep)
				SetForegroundWindow(hWnd);
			break;
		}

		case UWM_UPDATEBUSYSTATE:
			
			if (wParam != 0 || gLoading) break;

			char **urls, **titles;
			int index, count;
			if (!lParam) {
				if (kFuncs->GetMozillaSessionHistory(hWnd, &titles, &urls, &count, &index))
					currentSession.updateWindow(hWnd, index, count, (const char**)urls, (const char**)titles);
			}
			else {
				if (kFuncs->GetMozillaSessionHistory((HWND)lParam, &titles, &urls, &count, &index))
					currentSession.updateWindow(hWnd, (HWND)lParam, index, count, (const char**)urls, (const char**)titles);
			}

			currentSession.saveSession(kLastSessionName);
           	break;
			
		case WM_COMMAND:
			WORD command = LOWORD(wParam);
			if (command == id_undo_close)
				Undo(hWnd);
			
			else if (command == id_save_session) {

				// Ask for a session name
				TCHAR* answer = new TCHAR[256];
				answer[0]=0;
				int ok = gLoc->DialogBoxParam(
				  MAKEINTRESOURCE(IDD_PROMPT), hWnd, (DLGPROC)PromptDlgProc,(LPARAM)answer);
				
				// Save the session, first remove closed frame
				// Then rebuild the menu
				if (ok == IDOK && _tcslen(answer)>0) {
					currentSession.flush();
					USES_CONVERSION;
					currentSession.saveSession((LPSTR)T2A(answer));
					AddSessionList((LPSTR)T2A(answer));
					BuildSessionMenu();
				}
				delete [] answer;
			}

			else if (command >= id_load_session && command<id_load_session+MAX_SAVED_SESSION)
			{
				int n = command - id_load_session;
				char* name = sessions_list[command - id_load_session];

				if (name) {
					Session load;
					Session::loading = true;
					if (load.loadSession(name)) {
						BOOL warn = FALSE, nowarn=FALSE;
						kPlugin.kFuncs->GetPreference(PREF_BOOL, "browser.tabs.warnOnClose", &warn, &warn);
						kPlugin.kFuncs->SetPreference(PREF_BOOL, "browser.tabs.warnOnClose", &nowarn, TRUE);
						currentSession.close_except(hWnd);
						if (load.open())
							currentSession.close(hWnd);
						kPlugin.kFuncs->SetPreference(PREF_BOOL, "browser.tabs.warnOnClose", &warn, FALSE);
					}
					Session::loading = false;
				}
			}
			else if (command == id_config)
				Config(hWnd);

			break;
	    }
	
	
   if (IsWindowUnicode(hWnd))
	return CallWindowProcW(KMeleonWndProc, hWnd, message, wParam, lParam);
   else
	return CallWindowProcA(KMeleonWndProc, hWnd, message, wParam, lParam);
}

extern "C" {

   KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
      return &kPlugin;
   }
}