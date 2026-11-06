// ENCORE. Basement Studio
// Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).

#include "Shm.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace bridgeos
{
    struct Memory { HANDLE mapping; void* view; };
    struct Event  { HANDLE handle; };

    static Memory* map (HANDLE h, size_t bytes, bool zero)
    {
        if (h == nullptr) return nullptr;
        auto* view = MapViewOfFile (h, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
        if (view == nullptr) { CloseHandle (h); return nullptr; }
        if (zero) std::memset (view, 0, bytes);
        return new Memory { h, view };
    }

    Memory* createMemory (const wchar_t* name, size_t bytes)
    {
        return map (CreateFileMappingW (INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, (DWORD) bytes, name), bytes, true);
    }

    Memory* openMemory (const wchar_t* name, size_t bytes)
    {
        return map (OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, name), bytes, false);
    }

    void* address (Memory* m) { return m != nullptr ? m->view : nullptr; }

    void close (Memory* m)
    {
        if (m == nullptr) return;
        UnmapViewOfFile (m->view);
        CloseHandle (m->mapping);
        delete m;
    }

    Event* createEvent (const wchar_t* name)
    {
        auto h = CreateEventW (nullptr, FALSE, FALSE, name);
        return h != nullptr ? new Event { h } : nullptr;
    }

    Event* openEvent (const wchar_t* name)
    {
        auto h = OpenEventW (EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, name);
        return h != nullptr ? new Event { h } : nullptr;
    }

    void signal (Event* e) { if (e != nullptr) SetEvent (e->handle); }

    bool wait (Event* e, int ms) { return e != nullptr && WaitForSingleObject (e->handle, (DWORD) ms) == WAIT_OBJECT_0; }

    void close (Event* e)
    {
        if (e == nullptr) return;
        CloseHandle (e->handle);
        delete e;
    }

    int binaryBits (const wchar_t* path)
    {
        FILE* f = _wfopen (path, L"rb");
        if (f == nullptr) return 0;
        unsigned char dos[64] = {};
        int bits = 0;
        if (std::fread (dos, 1, sizeof (dos), f) == sizeof (dos) && dos[0] == 'M' && dos[1] == 'Z')
        {
            const long peAt = (long) (dos[0x3c] | (dos[0x3d] << 8) | (dos[0x3e] << 16) | (dos[0x3f] << 24));
            unsigned char pe[6] = {};
            if (std::fseek (f, peAt, SEEK_SET) == 0 && std::fread (pe, 1, sizeof (pe), f) == sizeof (pe) && pe[0] == 'P' && pe[1] == 'E')
            {
                const int machine = pe[4] | (pe[5] << 8);
                bits = machine == 0x14c ? 32 : (machine == 0x8664 || machine == 0xaa64) ? 64 : 0;
            }
        }
        std::fclose (f);
        return bits;
    }

    bool processHasWindow (unsigned long pid)
    {
        struct Search { DWORD pid; bool found; } search { (DWORD) pid, false };
        EnumWindows ([] (HWND h, LPARAM p) -> BOOL
        {
            auto* s = reinterpret_cast<Search*> (p);
            DWORD owner = 0;
            GetWindowThreadProcessId (h, &owner);
            if (owner == s->pid && IsWindowVisible (h)) { s->found = true; return FALSE; }
            return TRUE;
        }, (LPARAM) &search);
        return search.found;
    }

    void killProcess (unsigned long pid)
    {
        if (auto h = OpenProcess (PROCESS_TERMINATE, FALSE, (DWORD) pid)) { TerminateProcess (h, 1); CloseHandle (h); }
    }

    unsigned long currentProcessId() { return GetCurrentProcessId(); }

    /** This process and everything it started (activation prompts run in a process of their own, a child of the plugin) */
    static std::vector<DWORD> processTree()
    {
        std::vector<DWORD> tree { GetCurrentProcessId() };
        auto snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return tree;
        std::vector<PROCESSENTRY32W> all;
        PROCESSENTRY32W entry {};
        entry.dwSize = sizeof (entry);
        for (auto more = Process32FirstW (snapshot, &entry); more; more = Process32NextW (snapshot, &entry))
            all.push_back (entry);
        CloseHandle (snapshot);
        for (size_t i = 0; i < tree.size(); ++i)
            for (auto& p : all)
                if (p.th32ParentProcessID == tree[i] && p.th32ProcessID != tree[i]
                     && std::find (tree.begin(), tree.end(), p.th32ProcessID) == tree.end())
                    tree.push_back (p.th32ProcessID);
        return tree;
    }

    void hideWindowsWhileScanning (int giveUpMs, const wchar_t* flagFile)
    {
        std::thread ([giveUpMs, flag = std::wstring (flagFile != nullptr ? flagFile : L"")]
        {
            struct Search { const std::vector<DWORD>* tree; int hidden; };
            DWORD firstSeen = 0;
            std::vector<DWORD> tree;
            for (int tick = 0;; ++tick)
            {
                Sleep (25);
                if (tick % 4 == 0) tree = processTree();
                Search s { &tree, 0 };
                EnumWindows ([] (HWND h, LPARAM p) -> BOOL
                {
                    auto* s = reinterpret_cast<Search*> (p);
                    DWORD owner = 0;
                    GetWindowThreadProcessId (h, &owner);
                    if (IsWindowVisible (h) && std::find (s->tree->begin(), s->tree->end(), owner) != s->tree->end())
                    {
                        ShowWindowAsync (h, SW_HIDE);
                        ++s->hidden;
                    }
                    return TRUE;
                }, (LPARAM) &s);
                if (s.hidden > 0 && firstSeen == 0) firstSeen = GetTickCount();
                if (firstSeen != 0 && GetTickCount() - firstSeen > (DWORD) giveUpMs)
                {
                    if (! flag.empty()) if (FILE* f = _wfopen (flag.c_str(), L"w")) std::fclose (f);
                    // nothing left behind: a prompt's own process would outlive this one
                    for (auto pid : processTree())
                        if (pid != GetCurrentProcessId()) killProcess (pid);
                    TerminateProcess (GetCurrentProcess(), 3);
                }
            }
        }).detach();
    }
}
