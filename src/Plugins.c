#include "Helpers.h"
#include "Plugins.h"

#define MAX_PLUGINS 32
#define NP3_PLUGIN_EXPORT_NAME "NP3_PluginExecute"

typedef void (*NP3_PLUGIN_EXECUTE)(HWND);

typedef struct PLUGIN_ENTRY {
    UINT    id;
    HPATHL  path;
    HMODULE module;
    NP3_PLUGIN_EXECUTE execute;
    WCHAR   name[SMALL_BUFFER];
} PLUGIN_ENTRY;

static PLUGIN_ENTRY s_Plugins[MAX_PLUGINS];
static unsigned     s_PluginCount = 0;
static HMENU        s_hmenuPlugins = NULL;

static void Plugins_Enumerate(void)
{
    s_PluginCount = 0;
    HPATHL dir = Path_Copy(Paths.ModuleDirectory);
    Path_Append(dir, L"plugins");
    if (!Path_IsExistingDirectory(dir)) {
        Path_Release(dir);
        return;
    }

    HPATHL search = Path_Copy(dir);
    Path_Append(search, L"*.dll");
    WIN32_FIND_DATAW fd; ZeroMemory(&fd, sizeof(fd));
    HANDLE hFind = FindFirstFileW(Path_Get(search), &fd);
    if (IS_VALID_HANDLE(hFind)) {
        do {
            if (s_PluginCount >= MAX_PLUGINS) { break; }
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                PLUGIN_ENTRY* p = &s_Plugins[s_PluginCount];
                p->id = IDM_PLUGINS_FIRST + s_PluginCount;
                p->path = Path_Copy(dir);
                Path_Append(p->path, fd.cFileName);
                p->module = NULL;
                p->execute = NULL;
                StringCchCopy(p->name, COUNTOF(p->name), fd.cFileName);
                s_PluginCount++;
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
    Path_Release(search);
    Path_Release(dir);
}

void Plugins_Init(void)
{
    Plugins_Enumerate();
}

void Plugins_Release(void)
{
    for (unsigned i = 0; i < s_PluginCount; ++i) {
        PLUGIN_ENTRY* p = &s_Plugins[i];
        if (p->module) {
            FreeLibrary(p->module);
            p->module = NULL;
            p->execute = NULL;
        }
        if (p->path) { Path_Release(p->path); p->path = NULL; }
    }
    s_PluginCount = 0;
    if (s_hmenuPlugins) { DestroyMenu(s_hmenuPlugins); s_hmenuPlugins = NULL; }
}

bool Plugins_InsertMenu(HMENU hMenuBar)
{
    Plugins_Enumerate();
    if (s_hmenuPlugins) {
        DestroyMenu(s_hmenuPlugins);
    }
    s_hmenuPlugins = CreatePopupMenu();
    WCHAR item[SMALL_BUFFER];
    GetLngString(IDM_PLUGINS_MANAGE, item, COUNTOF(item));
    AppendMenu(s_hmenuPlugins, MF_STRING, IDM_PLUGINS_MANAGE, item);
    if (s_PluginCount > 0) {
        AppendMenu(s_hmenuPlugins, MF_SEPARATOR, 0, NULL);
        for (unsigned i = 0; i < s_PluginCount; ++i) {
            AppendMenu(s_hmenuPlugins, MF_STRING, s_Plugins[i].id, s_Plugins[i].name);
        }
    }
    WCHAR title[SMALL_BUFFER];
    GetLngString(IDS_MUI_MENU_PLUGINS, title, COUNTOF(title));
    int pos = GetMenuItemCount(hMenuBar) - 1;
    return InsertMenu(hMenuBar, pos, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)s_hmenuPlugins, title);
}

static void Plugins_OpenFolder(void)
{
    HPATHL dir = Path_Copy(Paths.ModuleDirectory);
    Path_Append(dir, L"plugins");
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.hwnd = Globals.hwndMain;
    sei.lpFile = Path_Get(dir);
    sei.nShow = SW_SHOWNORMAL;
    ShellExecuteExW(&sei);
    Path_Release(dir);
}

bool Plugins_Command(int cmdID)
{
    if (cmdID == IDM_PLUGINS_MANAGE) {
        Plugins_OpenFolder();
        return true;
    }
    for (unsigned i = 0; i < s_PluginCount; ++i) {
        PLUGIN_ENTRY* p = &s_Plugins[i];
        if (cmdID == p->id) {
            if (!p->module) {
                p->module = LoadLibrary(Path_Get(p->path));
                if (p->module) {
                    p->execute = (NP3_PLUGIN_EXECUTE)GetProcAddress(p->module, NP3_PLUGIN_EXPORT_NAME);
                }
            }
            if (p->execute) { p->execute(Globals.hwndMain); }
            return true;
        }
    }
    return false;
}
