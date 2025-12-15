#include "Common.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

void Extension::AddBody(double mass, double x, double y, double vx, double vy)
{
        if (mass <= 0.0)
                mass = 1.0;

        bodies.push_back({ mass, x, y, vx, vy });
        lastAddedId = static_cast<int>(bodies.size());
}

void Extension::SetMass(int id, double mass)
{
        if (!IsValidId(id) || mass <= 0.0)
                return;

        bodies[static_cast<size_t>(id - 1)].mass = mass;
}

void Extension::SetVelocity(int id, double vx, double vy)
{
        if (!IsValidId(id))
                return;

        auto &body = bodies[static_cast<size_t>(id - 1)];
        body.vx = vx;
        body.vy = vy;
}

void Extension::StepSimulation(double deltaTime)
{
        if (deltaTime <= 0.0 || bodies.empty())
                return;

        RefreshTrackedBodyPositions();
        if (bodies.empty())
                return;

        std::vector<std::pair<double, double>> accelerations(bodies.size(), { 0.0, 0.0 });

        if (gravitationalConstant != 0.0)
        {
                for (size_t i = 0; i < bodies.size(); ++i)
                {
                        for (size_t j = i + 1; j < bodies.size(); ++j)
                        {
                                const double dx = bodies[j].x - bodies[i].x;
                                const double dy = bodies[j].y - bodies[i].y;
                                const double distSq = dx * dx + dy * dy;

                                if (distSq < 1e-12)
                                        continue;

                                const double invDist = 1.0 / std::sqrt(distSq);
                                const double invDist3 = invDist * invDist * invDist;
                                const double factor = gravitationalConstant * invDist3;

                                const double ax = factor * bodies[j].mass * dx;
                                const double ay = factor * bodies[j].mass * dy;
                                const double bx = -factor * bodies[i].mass * dx;
                                const double by = -factor * bodies[i].mass * dy;

                                accelerations[i].first += ax;
                                accelerations[i].second += ay;
                                accelerations[j].first += bx;
                                accelerations[j].second += by;
                        }
                }
        }

        for (size_t i = 0; i < bodies.size(); ++i)
        {
                bodies[i].vx += accelerations[i].first * deltaTime;
                bodies[i].vy += accelerations[i].second * deltaTime;
                bodies[i].x += bodies[i].vx * deltaTime;
                bodies[i].y += bodies[i].vy * deltaTime;
        }

        ApplyTrackedBodiesToObjects();
}

void Extension::ClearBodies()
{
        bodies.clear();
        lastAddedId = 0;
        trackedActives.clear();
}

void Extension::SetGravitationalConstant(double constant)
{
        gravitationalConstant = constant;
}

void Extension::TrackActive(int fixedValue, double mass, double vx, double vy)
{
        if (fixedValue == 0 || mass <= 0.0)
                return;

        for (auto &tracked : trackedActives)
        {
                if (tracked.fixedValue == fixedValue)
                {
                        SetMass(tracked.bodyId, mass);
                        SetVelocity(tracked.bodyId, vx, vy);
                        return;
                }
        }

        RunObjectMultiPlatPtr ro = Runtime.RunObjPtrFromFixed(fixedValue);
        if (!ro)
                return;

        HeaderObject* ho = ro->get_rHo();
        AddBody(mass, static_cast<double>(ho->X), static_cast<double>(ho->Y), vx, vy);
        trackedActives.push_back({ fixedValue, lastAddedId });
}

void Extension::UntrackActive(int fixedValue)
{
        for (std::size_t i = 0; i < trackedActives.size(); ++i)
        {
                if (trackedActives[i].fixedValue == fixedValue)
                {
                        const int bodyId = trackedActives[i].bodyId;
                        trackedActives.erase(trackedActives.begin() + static_cast<std::ptrdiff_t>(i));
                        if (IsValidId(bodyId))
                                RemoveBodyAtIndex(static_cast<std::size_t>(bodyId - 1));
                        return;
                }
        }
}

void Extension::RemoveBodyAtIndex(std::size_t index)
{
        if (index >= bodies.size())
                return;

        bodies.erase(bodies.begin() + static_cast<std::ptrdiff_t>(index));

        for (auto &tracked : trackedActives)
        {
                if (tracked.bodyId == static_cast<int>(index + 1))
                        tracked.bodyId = 0;
                else if (tracked.bodyId > static_cast<int>(index + 1))
                        --tracked.bodyId;
        }

        trackedActives.erase(std::remove_if(trackedActives.begin(), trackedActives.end(), [](const TrackedActive &t)
        {
                return t.bodyId <= 0;
        }), trackedActives.end());

        if (lastAddedId > static_cast<int>(bodies.size()))
                lastAddedId = static_cast<int>(bodies.size());
}

void Extension::RefreshTrackedBodyPositions()
{
        if (trackedActives.empty())
                return;

        for (std::size_t i = trackedActives.size(); i-- > 0;)
        {
                TrackedActive &tracked = trackedActives[i];
                RunObjectMultiPlatPtr ro = Runtime.RunObjPtrFromFixed(tracked.fixedValue);
                if (!ro || !IsValidId(tracked.bodyId))
                {
                        if (tracked.bodyId > 0)
                                RemoveBodyAtIndex(static_cast<std::size_t>(tracked.bodyId - 1));
                        else
                                trackedActives.erase(trackedActives.begin() + static_cast<std::ptrdiff_t>(i));
                        continue;
                }

                HeaderObject* ho = ro->get_rHo();
                Body &body = bodies[static_cast<std::size_t>(tracked.bodyId - 1)];
                body.x = static_cast<double>(ho->X);
                body.y = static_cast<double>(ho->Y);
        }
}

void Extension::ApplyTrackedBodiesToObjects()
{
        for (std::size_t i = trackedActives.size(); i-- > 0;)
        {
                TrackedActive &tracked = trackedActives[i];
                if (!IsValidId(tracked.bodyId))
                {
                        trackedActives.erase(trackedActives.begin() + static_cast<std::ptrdiff_t>(i));
                        continue;
                }

                RunObjectMultiPlatPtr ro = Runtime.RunObjPtrFromFixed(tracked.fixedValue);
                if (!ro)
                {
                        RemoveBodyAtIndex(static_cast<std::size_t>(tracked.bodyId - 1));
                        continue;
                }

                HeaderObject* ho = ro->get_rHo();
                Body &body = bodies[static_cast<std::size_t>(tracked.bodyId - 1)];

                ho->X = static_cast<int>(std::lround(body.x));
                ho->Y = static_cast<int>(std::lround(body.y));

                if (auto roc = ro->get_roc())
                {
                        roc->rcChanged = true;
                        roc->rcCheckCollisions = true;
                }
        }
}
