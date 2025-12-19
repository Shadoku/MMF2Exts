#include "Common.hpp"

bool Extension::DummyCondition()
{
        return true;
}

bool Extension::IsObjectRegistered(int fixedValue)
{
        return registeredScripts.find(fixedValue) != registeredScripts.end();
}
