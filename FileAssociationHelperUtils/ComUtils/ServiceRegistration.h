#pragma once

class ServiceRegistration
{
public:
    ServiceRegistration(void);
    ~ServiceRegistration(void);

    bool RegisterService();
    bool UnregisterService();

private:
    bool RegisterTLB(bool regist = true);
    bool RegisterResource(int resid, bool regist = true);
};