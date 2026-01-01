#pragma once
#include "DarkEdif.hpp"
#include <vector>

class Extension
{
public:

	RunHeader* rhPtr;
	RunObjectMultiPlatPtr rdPtr; // you should not need to access this
#ifdef __ANDROID__
	global<jobject> javaExtPtr;
#elif defined(__APPLE__)
	void* const objCExtPtr;
#endif

	Edif::Runtime Runtime;

	static const int MinimumBuild = 254;
	static const int Version = 1;

	// If you change OEFLAGS, make sure you modify RUNDATA so the data is available, or you'll get crashes!
	// For example, OEFLAGS::VALUES makes use of the AltVals rv struct.
	static const OEFLAGS OEFLAGS = OEFLAGS::NONE;
	static const OEPREFS OEPREFS = OEPREFS::NONE;

        static const int WindowProcPriority = 100;

#ifdef _WIN32
	Extension(RunObject* const rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr);
#elif defined(__ANDROID__)
	Extension(const EDITDATA* const edPtr, const jobject javaExtPtr);
#else
	Extension(const EDITDATA* const edPtr, void* const objCExtPtr);
#endif
	~Extension();

        // Physics body representation
        struct Body
        {
                double mass;
                double x;
                double y;
                double vx;
                double vy;
        };

        std::vector<Body> bodies;
        struct TrackedActive
        {
                int fixedValue;
                int bodyId;
        };
        std::vector<TrackedActive> trackedActives;
        double gravitationalConstant;
        int lastAddedId;

        /// Actions

                void AddBody(double mass, double x, double y, double vx, double vy);
                void SetMass(int id, double mass);
                void SetVelocity(int id, double vx, double vy);
                void StepSimulation(double deltaTime);
                void ClearBodies();
                void SetGravitationalConstant(double constant);
                void TrackActive(int fixedValue, double mass, double vx, double vy);
                void UntrackActive(int fixedValue);

        /// Conditions

                bool BodyExists(int id);

        /// Expressions

                double BodyX(int id);
                double BodyY(int id);
                double BodyVX(int id);
                double BodyVY(int id);
                int BodyCount();
                int LastAddedBody();
                double GravitationalConstant();



	/* These are called if there's no function linked to an ID */

	void UnlinkedAction(int ID);
	long UnlinkedCondition(int ID);
	long UnlinkedExpression(int ID);




	/*  These replace the functions like HandleRunObject that used to be
		implemented in Runtime.cpp. They work exactly the same, but they're
		inside the extension class.
	*/

	REFLAG Handle();
	REFLAG Display();

        short FusionRuntimePaused();
        short FusionRuntimeContinued();

private:
        bool IsValidId(int id) const;
        void RemoveBodyAtIndex(std::size_t index);
        void RefreshTrackedBodyPositions();
        void ApplyTrackedBodiesToObjects();
};
