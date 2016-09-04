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

#include "Planner.h"
#include "IFixedTick.h"
#include <deque>
#include <random>
#include "Tasks.h"
#include "WorldQuerySystem/WorldQuerySystem.h"
#include "Barker.h"
#include "UE_Replacements.h"

namespace AI
{
	struct FUtilityScores
	{
		std::string goalName;
		std::vector<float> scores;
	};

	struct Utility
	{
		TaskIdentifier task;
		float score;
	};

	class HierarchicalTaskNetworkComponent : public Component, public IFixedTickable
	{
	public:

		HierarchicalTaskNetworkComponent(Actor* owner);
		virtual void initializeComponent() override;
		virtual void fixedTick(float dt) override;		

		void joinConversation();

		bool getCurrentStateFlag(WorldStateIdentifier identifier);
		float getCurrentStateValue(WorldStateIdentifier identifier);
		Math::Vector3 getCurrentStateVector(WorldStateIdentifier identifier);

	private:

		void tickEmotionalState(float dt);

		void sense();
		void senseVisual();
		void senseAuditory();

		void perceive();
		void perceiveVisual();
		void perceiveAuditory();

		void addEngramToLocus(Engram& engram);
		void recalculateLoci(std::vector<int> updatedLociIndices);
		void produceFacts();

		//when ignoring certain actors, we don't want any sensory/short term memory that refers to said actors
		auto purgeMemoryOfIgnoredActors() -> void;

		TaskIdentifier evaluateNeeds();
		void updateTaskHistory();
		void createPlan(TaskIdentifier goal);
		void decide();
		void performTask(float dt);
		void updateHUD_PlanPath();		

		int frameCount {0};

		float detailSightRadius;
		float detailAngle;
		float motionSightRadius;
		float motionAngle;

		Planner planner;
		WorldState state;
		TaskIdentifier currentGoal;

		std::deque<TaskIdentifier> taskHistory;	

		std::mt19937 eng{std::random_device{}()};

		WorldQuerier worldQuerySystem;

		std::vector<Utility> utilities;
		std::vector<Utility>::iterator utilityIterator;

	public:

		std::string currentGoalName;
		std::string currentTaskName;

		std::vector<std::string> planPath;
		float planProgress;

		Barker barker;

		std::vector<FUtilityScores> utilityScores;

		bool isBeingDebugViewed;
	};
}