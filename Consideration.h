/*
MIT License

Copyright (c) 2016 Patrick Lafferty

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "WorldState.h"
#include "Tasks.h"

/*
A Consideration applies some function to some value to calculate a score.
This score is used to compare tasks when deciding on a goal for an AI Character.
*/

namespace AI
{

	/*
	Where to find some value in WorldState
	eg Scalar consideration types find their values in WorldState::Current::Values
	*/
	enum class ConsiderationType
	{
		Repeat, 
		Scalar,
		Flag,
		Distance,
		ConsumableFlag,
		ConsumableValue,
		ConsumableVector,
		AuditoryStimulus,
		Predicate,
		ValueOverTimeTracker,
		LocusImportance,
		LocusAge
	};

	//the function to use to score the value extracted from world state
	enum class FunctionType 
	{
		Logistic, 
		Quadratic,
		Exponential,
		Gaussian,
		Step,
		Linear
	};

	//y = L / (1 + e ^ -k * (x - x0))
	struct Logistic 
	{
		/** Curve's maximum value */
		float L;

		/** Steepness of curve */
		float k;

		/** X-value of midpoint */
		float x0;
	};

	//y = ax^2 + bx + c
	struct Quadratic
	{
		/** coefficient for x^2 */
		float a;

		/** coefficient for x */
		float b;

		/** constant */
		float c;
	};

	//y = a^x
	struct Exponential
	{
		float a;
	};

	//y = a * e^(-(x - b)^2 / (2*c)^2)
	struct Gaussian
	{
		/** Height of curve's peak */
		float a;

		/** Position of the centre */
		float b;

		/** Width of the bell */
		float c;
	};

	struct Step
	{
		float crossover;
	};

	struct Linear
	{
	};

	struct Domain
	{
		Domain() 
		{
			min = 0.f;
			max = 1.f;
		}

		float min;
		float max;
	};

	struct RepeatConsideration
	{
		TaskIdentifier identifier;
	};

	struct ScalarConsideration
	{
		WorldStateIdentifier identifier;
	};

	struct DistanceConsideration
	{
		WorldStateIdentifier from;
		WorldStateIdentifier to;
	};

	struct FlagConsideration
	{
		WorldStateIdentifier identifier;
		bool negate;
	};

	struct ConsumableFlagConsideration
	{
		ConsumableFact fact;
	};

	struct ConsumableValueConsideration
	{
		ConsumableFact fact;
	};

	struct ConsumableVectorConsideration
	{
		ConsumableFact fact;
	};

	enum class ValueOverTimeTracker_Property 
	{
		MaxValue,
		DurationBelowValue,
		Reacted,
		DurationExceedsExtension
	};

	struct ValueOverTimeTrackerConsideration
	{
		WorldStateIdentifier identifier;

		ValueOverTimeTracker_Property trackerProperty;
	};

	struct LocusImportanceConsideration
	{
		ConsumableFact factContainingLocusId;
	};

	struct LocusAgeConsideration
	{
		ConsumableFact factContainingLocusId;
	};
		
	struct FunctionDefinition
	{
		FunctionType type;

		float verticalOffset {0.f};
		float horizontalOffset {0.f};
		float verticalStretch {1.f};
		float horizontalStretch {1.f};

		union
		{
			Logistic logistic;
			Quadratic quadratic;
			Exponential exponential;
			Gaussian gaussian;
			Step step;		
			Linear linear;
		};
	};

	struct AuditoryStimulus
	{
	};

	struct Consideration
	{
		ConsiderationType type;
		FunctionDefinition function;
		bool useSubsetMax {false};
		int group {0}; //group 0 is base score, then multiplied by the max of the remaining groups

		union 
		{
			RepeatConsideration repeat;
			ScalarConsideration scalar;						
			FlagConsideration flag;
			DistanceConsideration distance;			
			ConsumableFlagConsideration consumableFlag;
			ConsumableValueConsideration consumableValue;
			ConsumableVectorConsideration consumableVector;
			AuditoryStimulus auditoryStimulus;
			ValueOverTimeTrackerConsideration valueTracker;
			LocusImportanceConsideration locusImportance;
			LocusAgeConsideration locusAge;
		};

		std::function<bool(WorldState&)> predicateFunction;
	};	
}