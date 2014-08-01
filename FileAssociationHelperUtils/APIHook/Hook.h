#ifndef APIHOOK_HOOK_H_
#define APIHOOK_HOOK_H_

namespace apihook
{
    void *Hook(void *target_proc, void *hook_proc);
    bool Unhook(void *real_proc, void *hook_proc);

    void *HookDLL(const wchar_t *dllname, const char *procname, void *hook_proc);
    bool UnhookDLL(const wchar_t *dllname, void *real_proc, void *hook_proc);

    template<class Proc>
    Proc *Hook(Proc *target_proc, Proc *hook_proc)
    {
        return reinterpret_cast<Proc*>(Hook(static_cast<void*>(target_proc), static_cast<void*>(hook_proc)));
    }

    template<class Proc>
    bool Unhook(Proc *real_proc, Proc *hook_proc)
    {
        return Unhook(static_cast<void*>(real_proc), static_cast<void*>(hook_proc));
    }

    template<class Proc>
    Proc *HookDLL(const wchar_t *dllname, const char *procname, Proc *hook_proc)
    {
        return reinterpret_cast<Proc*>(HookDLL(dllname, procname, static_cast<void*>(hook_proc)));
    }

    template<class Proc>
    bool UnhookDLL(const wchar_t *dllname, Proc *real_proc, Proc *hook_proc)
    {
        return UnhookDLL(dllname, static_cast<void*>(real_proc), static_cast<void*>(hook_proc));
    }

    /*
    * HookDLL helper template.
    *
    * It will not change real_proc value if hook operation failed.
    * Because real_proc may still used by hook proc when unhooked or hook failed.
    */
    template<class Proc>
    bool HookDLL(const wchar_t *dllname, const char *procname, Proc *hook_proc, Proc *&real_proc)
    {
        Proc *proc = apihook::HookDLL(dllname, procname, hook_proc);
        if (proc != nullptr)
        {
            real_proc = proc;
            return true;
        }
        else
        {
            return false;
        }
    }
}

#endif // APIHOOK_HOOK_H_