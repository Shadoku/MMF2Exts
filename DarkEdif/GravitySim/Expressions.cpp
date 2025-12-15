#include "Common.hpp"

double Extension::BodyX(int id)
{
        if (!IsValidId(id))
                return 0.0;

        return bodies[static_cast<size_t>(id - 1)].x;
}

double Extension::BodyY(int id)
{
        if (!IsValidId(id))
                return 0.0;

        return bodies[static_cast<size_t>(id - 1)].y;
}

double Extension::BodyVX(int id)
{
        if (!IsValidId(id))
                return 0.0;

        return bodies[static_cast<size_t>(id - 1)].vx;
}

double Extension::BodyVY(int id)
{
        if (!IsValidId(id))
                return 0.0;

        return bodies[static_cast<size_t>(id - 1)].vy;
}

int Extension::BodyCount()
{
        return static_cast<int>(bodies.size());
}

int Extension::LastAddedBody()
{
        return lastAddedId;
}

double Extension::GravitationalConstant()
{
        return gravitationalConstant;
}
