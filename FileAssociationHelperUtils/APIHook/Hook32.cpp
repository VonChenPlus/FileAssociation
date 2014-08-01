#include "Hook.h"
#include <intrin.h>
#include <Windows.h>

namespace apihook
{
    namespace
    {
        const unsigned char target_pattern1[] =
        {
            0x90, 0x90, 0x90, 0x90, 0x90, // nop * 5
            0x8b, 0xff, // mov edi, edi
        };

        const unsigned char target_pattern2[] =
        {
            0x90, 0x90, 0x90, 0x90, 0x90, // nop * 5
            0x8b, 0xc0, // mov eax, eax
        };

        int JmpValue(void *target_proc, void *hook_proc)
        {
            return reinterpret_cast<int>(hook_proc) - reinterpret_cast<int>(target_proc);
        }

        bool HookCode(unsigned char *code, void *target_proc, void *hook_proc)
        {
            if (memcmp(code, target_pattern1, sizeof(target_pattern1)) == 0 ||
                memcmp(code, target_pattern2, sizeof(target_pattern2)) == 0)
            {
                code[0] = 0xe9; // jmp
                *reinterpret_cast<int*>(&code[1]) = JmpValue(target_proc, hook_proc);
                code[5] = 0xeb; // jmp
                code[6] = 0xf9; // -7
                return true;
            }
            else
            {
                return false;
            }
        }

        bool UnhookCode(unsigned char *code, void *target_proc, void *hook_proc)
        {
            if (code[0] == 0xe9 &&  // jmp
                *reinterpret_cast<int*>(&code[1]) == JmpValue(target_proc, hook_proc) &&
                code[5] == 0xeb &&  // jmp
                code[6] == 0xf9)    // -7
            {
                memcpy(code, target_pattern1, sizeof(target_pattern1));
                return true;
            }
            else
            {
                return false;
            }
        }

        bool UpdateCode(long long *patch, long long old_code, long long new_code)
        {
            if (_InterlockedCompareExchange64(patch, new_code, old_code) == old_code)
            {
                ::FlushInstructionCache(GetCurrentProcess(), patch, 8);
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    void *Hook(void *target_proc, void *hook_proc)
    {
        void *real_proc = nullptr;

        unsigned char *patch_proc = reinterpret_cast<unsigned char *>(target_proc) - 5;

        DWORD protect;
        if (::VirtualProtect(patch_proc, 8, PAGE_EXECUTE_WRITECOPY, &protect))
        {
            // It is safe to use SEH to guard the modify code region.
            __try
            {
                long long old_code, new_code;
                old_code = new_code = *reinterpret_cast<long long*>(patch_proc);

                if (HookCode(reinterpret_cast<unsigned char*>(&new_code), target_proc, hook_proc) &&
                    UpdateCode(reinterpret_cast<long long*>(patch_proc), old_code, new_code))
                {
                    real_proc = patch_proc + 7;
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                // When other code change the memory, exception may occur.
                // Return directly without change the memory type.
                return nullptr;
            }

            ::VirtualProtect(patch_proc, 8, protect, &protect);
        }

        return real_proc;
    }

    bool Unhook(void *real_proc, void *hook_proc)
    {
        bool ret = false;

        unsigned char *target_proc = reinterpret_cast<unsigned char *>(real_proc) - 2;
        unsigned char *patch_proc = reinterpret_cast<unsigned char *>(target_proc) - 5;

        DWORD protect;
        if (::VirtualProtect(patch_proc, 8, PAGE_EXECUTE_WRITECOPY, &protect))
        {
            // It is safe to use SEH to guard the modify code region.
            __try
            {
                long long old_code, new_code;
                old_code = new_code = *reinterpret_cast<long long*>(patch_proc);

                if (UnhookCode(reinterpret_cast<unsigned char*>(&new_code), target_proc, hook_proc) &&
                    UpdateCode(reinterpret_cast<long long*>(patch_proc), old_code, new_code))
                {
                    ret = true;
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                // When other code change the memory, exception may occur.
                // Return directly without change the memory type.
                return false;
            }

            ::VirtualProtect(patch_proc, 8, protect, &protect);
        }

        return ret;
    }

    void *HookDLL(const wchar_t *dllname, const char *procname, void *hook_proc)
    {
        void *real_proc = nullptr;

        HMODULE module = ::LoadLibraryW(dllname);
        if (module != nullptr)
        {
            void *target_proc = ::GetProcAddress(module, procname);
            if (target_proc != nullptr)
            {
                real_proc = Hook(target_proc, hook_proc);
            }
            // Not call FreeLibrary here, because HookDLL must make sure the DLL not unloaded.
        }

        return real_proc;
    }

    bool UnhookDLL(const wchar_t *dllname, void *real_proc, void *hook_proc)
    {
        HMODULE module = ::GetModuleHandleW(dllname);
        if (module != nullptr && Unhook(real_proc, hook_proc))
        {
            ::FreeLibrary(module);
            return true;
        }
        else
        {
            return false;
        }
    }
}