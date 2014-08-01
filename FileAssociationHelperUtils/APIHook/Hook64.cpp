#include "Hook.h"
#include <Windows.h>
#include "hde64.h"

namespace apihook
{
    namespace
    {
        const unsigned char target_pattern1[] =
        {
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90,// nop * 6
        };

        void *Get64KAlignedAddress(void *address)
        {
            return reinterpret_cast<void*>(reinterpret_cast<long long>(address) & 0xffffffffffff0000);
        }

        void* FindFreeAddress(void *target_proc, void *dst_proc)
        {
            long long search_boundary = 0x80000000 - 0x10000;  // 2GB -64k
            void *max_address = target_proc;
            void *min_address = dst_proc;
            if (dst_proc > target_proc)
            {
                max_address = dst_proc;
                min_address = target_proc;
            }
            void *address = reinterpret_cast<unsigned char*>(max_address) - search_boundary;

            if (reinterpret_cast<long long>(address) < 0)
                address = reinterpret_cast<unsigned char*>(0x000000000010000);
            void * highest_lmt = reinterpret_cast<unsigned char*>(min_address) + search_boundary;
            if (address > highest_lmt)
                return nullptr;
            MEMORY_BASIC_INFORMATION t;
            address = Get64KAlignedAddress(address);
            while (address <= highest_lmt)
            {
                VirtualQuery(address, &t, sizeof(MEMORY_BASIC_INFORMATION));
                if (t.State == MEM_FREE)
                    return address;
                address = reinterpret_cast<unsigned char*>(address) + 0x10000;  // 64k
            }
            return nullptr;
        }

        void * FindDestnation(void * target_proc, int jmp_value, int ori_code_size)
        {
            return jmp_value  + ori_code_size + reinterpret_cast<unsigned char *>(target_proc);
        }

        int JmpValue(void *target_proc, void *hook_proc)
        {
            return static_cast<int>(reinterpret_cast<long long>(hook_proc) - reinterpret_cast<long long>(target_proc));
        }

        int TransformOriginalCode(void *target_proc, unsigned char *alloc_address, void * dst_address, hde64s hs)
        {
            int ori_code_size = hs.len; // ori_code_size bytes
            int rel_jmp_size = 5; // 5 bytes
            if (hs.opcode == 0xE9 || hs.opcode== 0xEB || hs.opcode == 0xE8)
            {
                //jmp ,call
                alloc_address[32] = hs.opcode ;
                if( hs.opcode == 0xEB)
                    alloc_address[32] = 0xE9; 
                *reinterpret_cast<int *>(&alloc_address[33]) = JmpValue(&alloc_address[32] + rel_jmp_size, dst_address);
                return 32 + rel_jmp_size;
            }    
            else if (hs.modrm_mod == 0x00 && hs.modrm_rm == 0x05)
            {
                // [rip + disp32]
                int new_displacement = static_cast<int>(reinterpret_cast<unsigned char *>(dst_address) - &alloc_address[32] - ori_code_size);//
                memcpy(&alloc_address[32], target_proc, ori_code_size );
                int i;
                for (i = 32;i < 32 + ori_code_size;i++)
                {
                    if (*reinterpret_cast<UINT *>(&alloc_address[i]) == hs.disp.disp32)
                    {
                        *reinterpret_cast<int *>(&alloc_address[i]) = new_displacement;
                        break;
                    }
                }
                return 32 + ori_code_size;
            }
            else
            {
                memcpy(&alloc_address[32], target_proc, hs.len);
                return 32 + ori_code_size;
            }
        }

        int SetJmpValue(hde64s hs)
        {
            if (hs.opcode == 0xE9 || hs.opcode == 0xE8) //jmp ,call
            {
                return	hs.imm.imm32;
            }
            else if (hs.opcode== 0xEB)
            {
                if (hs.imm.imm8 & 0x80)
                    return hs.imm.imm32 | 0xFFFFFF00;
                return hs.imm.imm32;
            }   
            else if (hs.modrm_mod == 0x00 && hs.modrm_rm == 0x05) // [rip + disp32]
            {
                return hs.disp.disp32;
            }
            else
            {
                return -hs.len;
            }
        }

        unsigned char *JmpHook(void *target_proc, void *hook_proc, hde64s hs)
        {
            if ((hs.opcode & 0xF0) == 0x70 || hs.opcode == 0xE3 || (hs.opcode2 & 0xF0) == 0x80) // Jcc
            {
                return nullptr;
            }
            int ori_code_size = hs.len; // ori_code_size bytes
            int alloc_size = 64; //64 bytes
            int size_of_jmp = 6; //6 bytes
            int jmp_index;
            void *free_address;
            unsigned char *alloc_address;
            void *dst_address;

            int jmp_value = SetJmpValue(hs);
            dst_address = FindDestnation(target_proc, jmp_value, ori_code_size);
            free_address = FindFreeAddress(target_proc, dst_address);
            alloc_address = reinterpret_cast<unsigned char*>(VirtualAlloc(free_address, alloc_size, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE));
            if (!free_address || !alloc_address)
                return nullptr;
            unsigned char *real_proc = reinterpret_cast<unsigned char *>(target_proc) + ori_code_size;
            *reinterpret_cast<long long*>(&alloc_address[0]) = reinterpret_cast<long long>(hook_proc); // address of hook function

            *reinterpret_cast<long long*>(&alloc_address[8]) = reinterpret_cast<long long>(real_proc);// address of the target function + ori_code_size

            *reinterpret_cast<unsigned char *>(&alloc_address[16]) = static_cast<unsigned char>(ori_code_size); // ori_code_size bytes

            memcpy(&alloc_address[17], target_proc, ori_code_size); // original code

            jmp_index = TransformOriginalCode(target_proc, alloc_address, dst_address, hs);
            alloc_address[jmp_index] = 0xff;
            alloc_address[jmp_index + 1] = 0x25; // jmp 
            *reinterpret_cast<int *>(&alloc_address[jmp_index + 2]) = JmpValue(&alloc_address[jmp_index] + size_of_jmp, &alloc_address[8]); 
            return alloc_address;
        }

        unsigned char* HookCode(unsigned char *code, void *target_proc, void *hook_proc, hde64s hs)
        {
            if (memcmp(code, target_pattern1, sizeof(target_pattern1)))
                return nullptr;
            int ori_code_size = hs.len;
            unsigned char *alloc_address = JmpHook(target_proc, hook_proc, hs);
            if (alloc_address)
            {
                code[0] = 0xff;
                code[1] = 0x25;
                *reinterpret_cast<int*>(&code[2]) = JmpValue(target_proc, alloc_address); // jump to alloc_address
                code[6] = 0xeb; // jmp
                code[7] = 0xf8; // -8
                memcpy(&code[8], target_pattern1, ori_code_size - 2);
                return alloc_address;
            }
            else
            {
                return nullptr;
            }
        }

        bool UnhookCode(unsigned char *code, void *target_proc, void* /*hook_proc*/, unsigned char *alloc_address)
        {
            if (code[0] == 0xff &&
                code[1] == 0x25 &&
                *reinterpret_cast<int*>(&code[2]) == JmpValue(target_proc, alloc_address) &&
                code[6] == 0xeb && // jmp
                code[7] == 0xf8  // -8
                )
            {
                memcpy(code, target_pattern1, sizeof(target_pattern1));
                int ori_code_size = static_cast<int>(alloc_address[16]);
                memcpy(&code[6], &alloc_address[17], ori_code_size);
                VirtualFree(alloc_address, 0, MEM_RELEASE);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool UpdateCode(void *dst, void *src, size_t size)
        {
            if (memcpy(dst, src, size))
                return true;
            else
                return false;
        }
    }

    void *Hook(void *target_proc, void *hook_proc)
    {
        void *real_proc = nullptr;

        unsigned char *patch_proc = reinterpret_cast<unsigned char *>(target_proc) - 6;

        DWORD protect;
        if (::VirtualProtect(patch_proc, 8, PAGE_EXECUTE_WRITECOPY, &protect))
        {
            // It is safe to use SEH to guard the modify code region.
            __try
            {
                hde64s hs;
                hde64_disasm(target_proc, &hs);
                int reserve_size = 6;
                int ori_code_size = hs.len;
                unsigned char *new_code = patch_proc;
                unsigned char * alloc_address = HookCode(new_code, target_proc, hook_proc, hs);
                if (alloc_address && UpdateCode(patch_proc, new_code, ori_code_size + reserve_size))
                {
                    real_proc = alloc_address + 32;
                }
                else if (alloc_address)
                {
                    VirtualFree(alloc_address, 0, MEM_RELEASE); // updateCode failed
                    return nullptr;
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
        unsigned char *alloc_address = reinterpret_cast<unsigned char *>(real_proc) - 32;
        int ori_code_size = static_cast<int>(alloc_address[16]);
        int max_code_size = 16; // 16 bytes
        int reserve_size = 6; // 6 bytes

        long long target_address = *reinterpret_cast<long long *>(reinterpret_cast<unsigned char *>(real_proc) - 24 ) ;
        unsigned char *target_proc = reinterpret_cast<unsigned char*>(target_address) - ori_code_size;
        unsigned char *patch_proc = reinterpret_cast<unsigned char *>(target_proc) - reserve_size;


        DWORD protect;
        if (::VirtualProtect(patch_proc, 8, PAGE_EXECUTE_WRITECOPY, &protect))
        {
            // It is safe to use SEH to guard the modify code region.
            __try
            {
                unsigned char *new_code = patch_proc;
                if (UnhookCode(new_code, target_proc, hook_proc ,alloc_address) && ori_code_size < max_code_size)
                {
                    UpdateCode(patch_proc, new_code, ori_code_size + reserve_size);
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
