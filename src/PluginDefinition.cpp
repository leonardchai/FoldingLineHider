//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// The data of plugin config
//
struct ConfigData
{
public:
    int isFoldingLineHidden = 1;
};

ConfigData configData;

// shortcuts
ShortcutKey shortcutCollapse = { false, true, false, VK_LEFT };
ShortcutKey shortcutUncollapse = { false, true, false, VK_RIGHT };

//
// for path string manipulation
//

#include <vector>
#include <string>
#include <corecrt_wstring.h>
#include <shlwapi.h>
#include <shlobj.h>

using namespace std;

#ifdef UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif



//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );

    // register menu commands
    loadConfig(&configData);

    setCommand(0, TEXT("Hide Folding Line (Toggle)"), toggleFoldingLineVisibility, nullptr, configData.isFoldingLineHidden);
    setCommand(1, TEXT("Collapse Current Level"), collapseCurrentLevel, &shortcutCollapse, 0);
    setCommand(2, TEXT("Uncollapse Current Level&nbsp;&nbsp;"), uncollapseCurrentLevel, &shortcutUncollapse, 0);
    setCommand(3, TEXT("About..."), about, nullptr, 0);

    // initial visibility
    configData.isFoldingLineHidden = configData.isFoldingLineHidden ? 0 : 1;
    toggleFoldingLineVisibility();
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//

void setFoldingStyle(HWND wndHandle, bool shouldBeHidden)
{
    if (shouldBeHidden)
    {
        LPARAM blackAndTransparentColor = 0; // RGBA(0,0,0,0)
		::SendMessage(wndHandle, SCI_SETELEMENTCOLOUR, SC_ELEMENT_FOLD_LINE, blackAndTransparentColor);
    }
    else
    {
		::SendMessage(wndHandle, SCI_RESETELEMENTCOLOUR, SC_ELEMENT_FOLD_LINE, 0);
    }
}
void toggleFoldingLineVisibility()
{
    configData.isFoldingLineHidden = configData.isFoldingLineHidden ? 0 : 1;
    saveConfig(&configData);

    ::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[0]._cmdID, configData.isFoldingLineHidden);

    if (nppData._scintillaMainHandle != 0)
        setFoldingStyle(nppData._scintillaMainHandle, configData.isFoldingLineHidden);

    if (nppData._scintillaSecondHandle != 0)
    	setFoldingStyle(nppData._scintillaSecondHandle, configData.isFoldingLineHidden);
}

void collapseCurrentLevel()
{
    ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_FOLD_CURRENT);
}

void uncollapseCurrentLevel()
{
    ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_UNFOLD_CURRENT);
}

void about()
{
    ::MessageBox(nppData._nppHandle,
        TEXT("FoldingLineHider v1.1\n\nby leonardchai@gmail.com"),
        TEXT("About..."), MB_OK);
}


//
// config
//

void conditionalCreatePath(TCHAR* path)
{
    TCHAR tempPath[MAX_PATH];
    _tcscpy(tempPath, path);

    // create config path if not exist
    if (::PathFileExists(tempPath) == FALSE)
    {
        vector<tstring>  vPaths;

        *_tcsrchr(tempPath, '\\') = NULL;
        do {
            vPaths.push_back(tempPath);
            *_tcsrchr(tempPath, NULL) = NULL;
        } while (::PathFileExists(tempPath) == FALSE);

        for (int i = static_cast<int>(vPaths.size()) - 1; i >= 0; i--)
        {
            _tcscpy(tempPath, vPaths[i].c_str());
            ::CreateDirectory(tempPath, NULL);
        }
    }
}

void prepareIniFile(TCHAR* iniFilePath)
{
    TCHAR configPath[MAX_PATH];
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);
    ::PathRemoveBackslash(configPath);

    conditionalCreatePath(configPath);

    // create ini file (conditional)
    _tcscpy(iniFilePath, configPath);
    _tcscat(iniFilePath, TEXT("FoldingLineHider.ini"));

    if (!PathFileExists(iniFilePath))
    {
        HANDLE hFile = ::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
#ifdef UNICODE
            BYTE BOM[] = { 0xFF, 0xFE };
            DWORD dwWritten = 0;
            ::WriteFile(hFile, BOM, sizeof(BOM), &dwWritten, NULL);
#endif      
            ::CloseHandle(hFile);
        }
    }

}

void loadConfig(ConfigData* config)
{
    TCHAR iniFilePath[MAX_PATH];
    prepareIniFile(iniFilePath);

    config->isFoldingLineHidden = ::GetPrivateProfileInt(TEXT("FoldingLineHider"), TEXT("FoldingLineHidden"), 9, iniFilePath);
}


void saveConfig(ConfigData* config)
{
    TCHAR iniFilePath[MAX_PATH];
    prepareIniFile(iniFilePath);

    const TCHAR* one = TEXT("1");
    const TCHAR* zero = TEXT("0");
    ::WritePrivateProfileString(TEXT("FoldingLineHider"), TEXT("FoldingLineHidden"), config->isFoldingLineHidden ? one : zero, iniFilePath);
}

