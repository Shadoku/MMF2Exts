#pragma once
#include <string>

class EmbersEngine
{
public:
    EmbersEngine();
    ~EmbersEngine();

    bool Execute(const char* code, std::string& result);
    bool Eval(const char* expression, std::string& result);
    void SetGlobalNumber(const char* name, double value);
    void SetGlobalString(const char* name, const char* value);
    double GetGlobalNumber(const char* name);
    bool GetGlobalString(const char* name, std::string& value);

private:
    void* engine;
};
