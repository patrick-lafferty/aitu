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

#include "Tasks.h"
#include "ConditionOpSatisfies.h"
#include <algorithm>
#include "Utility.h"
#include <iostream>
#include "Animations.h"
#include "WorldQuerySystem/EnvironmentQueries.h"
#include "Barker.h"
#include "HierarchicalTaskNetworkComponent.h"

using namespace Math;

namespace AI
{	
	void Task::addSubtask(Task& task)
	{
		if (!subtasks.empty())
		{
			subtasks.back().isLastInCompound = false;
		}

		subtasks.push_back(task);
		subtasks.back().isLastInCompound = true;
	}

	Task create_Wait(float time)
	{
		Task wait {"wait"};		
		wait.action = Action::Wait;
		wait.parameters.vectors.resize(1);
		wait.setup = [=](Task& task, const WorldState& state, const WorldQuerier&)
		{
			//parameters.vectors[0] is {current alarm time, total alarm time, UNUSED}
			task.parameters.vectors[0].x = 0;
			task.parameters.vectors[0].y = time;
		};
		
		wait.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::TimeElapsed, 0});

		return wait;
	}

	Task create_Bark(Bark desired)
	{
		Task bark{"bark"};		
		bark.action = Action::Bark;
		bark.parameters.values.push_back(static_cast<int>(desired));

		return bark;
	}

	Task create_PlayMontage(Montage montage)
	{
		Task playMontage {"playMontage"};		
		playMontage.action = Action::PlayMontage;
		playMontage.parameters.values.resize(3);
		playMontage.parameters.vectors.resize(1);
		playMontage.parameters.values[0] = static_cast<float>(montage);
		playMontage.parameters.values[1] = 0; //0 = needs to be started, 1 = already started
		playMontage.parameters.values[2] = 0; //elapsed time
		playMontage.setup = [](Task& task, const WorldState& state, const WorldQuerier&)
		{
			//parameters.vectors[0] is {current alarm time, total alarm time, UNUSED}
			task.parameters.vectors[0].x = 0;
			task.parameters.vectors[0].y = 1;
		};
		
		playMontage.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::TimeElapsed, 0});

		return playMontage;
	}	

	#if 0
	auto create_PlayAnimation(EAnimationId animation, EAnimationType type) -> Task
	{
		Task playAnimation {"playAnimation"};
		playAnimation.action = Action::PlayAnimation;
		playAnimation.parameters.values.resize(4); //6
		playAnimation.parameters.vectors.resize(3); //2
		
		/*
		Parameters:
		values: 
		NO 0 is the current timer for TimeElapsed satisfiable predicate, used to end the task
		NO 1 is the total alarm time for TimeElapsed, setting parameter 0 = parameter 1 ends the task
		2 is the EAnimationId casted to float
		3 is EAnimationPlayerState
		4 is elapsed animation time
		5 is EAnimationType

		vectors:
		0 is {current timer, total alarm time, UNUSED}
		1 is WorldStateIdentifier, the value to give for blend space params (x, y, 0)
		2 is the actual blend space parameters to use
		*/		

		playAnimation.parameters.values[0] = static_cast<float>(animation); 
		playAnimation.parameters.values[1] = static_cast<float>(EAnimationPlayerState::NeedsToStart);
		playAnimation.parameters.values[2] = 0;
		playAnimation.parameters.values[3] = static_cast<float>(type);
		
		playAnimation.parameters.vectors[2] = {0.f, 0.f, 0.f};

		playAnimation.setup = [](Task& task, const WorldState& state, const WorldQuerier&)
		{
			task.parameters.vectors[0].x = 0;
			task.parameters.vectors[0].y = 1;
		};

		//playAnimation.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::TimeElapsed, 0});

		return playAnimation;
	}

	auto create_PlayBlendAnimation_SourceWorldState(EAnimationId animation, WorldStateIdentifier worldStateIdX, WorldStateIdentifier worldStateIdY) -> Task
	{
		auto playAnimation = create_PlayAnimation(animation, EAnimationType::BlendSpace);
		playAnimation.parameters.vectors[0] = {static_cast<float>(worldStateIdX), static_cast<float>(worldStateIdY), 0};

		playAnimation.loop = [](WorldState& state, WorldQuerier const&, Task& task)
		{
			task.parameters.vectors[1].x = state.current.values[static_cast<WorldStateIdentifier>(FMath::FloorToInt(task.parameters.vectors[0].x))];
			task.parameters.vectors[1].y = state.current.values[static_cast<WorldStateIdentifier>(FMath::FloorToInt(task.parameters.vectors[0].y))];
		};

		return playAnimation;
	}

	template<class Func>
	auto create_PlayBlendAnimation_SourceCalculate(EAnimationId animation, bool calculateOnce, Func&& calculate) -> Task
	{
		auto playAnimation = create_PlayAnimation(animation, EAnimationType::BlendSpace);
		playAnimation.loop = [calculate = std::forward<Func>(calculate), calculateOnce](WorldState& state, WorldQuerier const& worldQuerySystem, Task& task)
		{
			task.parameters.vectors[2] = calculate(state, worldQuerySystem, task);
			
			if (calculateOnce)
				task.loop = nullptr;
		};

		return playAnimation;
	}
	#endif

	Task create_MoveToDestination(float walkSpeed, Math::Vector3 debugSphereColour, float acceptanceRadius = 1.f, bool stopOnOverlap = true, bool usePathfinding = true, int parameterIndex = -1)
	{
		/*
		Parameters:
		values: 
		0 is walk speed
		1 is NeedsMoveToCalled
		*/
		Task moveToDestination {"moveToDestination"};
		moveToDestination.action = Action::MoveToDestination;
		moveToDestination.parameters.values.push_back(walkSpeed);
		moveToDestination.parameters.values.push_back(1.f);
		moveToDestination.parameters.vectors.push_back({acceptanceRadius, stopOnOverlap ? 1.f : 0.f, usePathfinding ? 1.f : 0.f});
		
		moveToDestination.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::NearDestination, parameterIndex});
		
		return moveToDestination;
	}

	#if 0
	auto create_MoveToActor(float walkSpeed, BlackboardKey key, float acceptanceRadius = 1.f, bool stopOnOverlap = true, bool usePathfinding = true, int parameterIndex = -1) -> Task
	{
		/*
		Parameters:
		values: 
		0 is walk speed
		1 is NeedsMoveToCalled
		*/
		Task moveToActor {"moveToActor"};
		moveToActor.action = Action::MoveToActor;
		moveToActor.parameters.values.push_back(walkSpeed);
		moveToActor.parameters.values.push_back(1.f);
		moveToActor.parameters.vectors.push_back({acceptanceRadius, stopOnOverlap ? 1.f : 0.f, usePathfinding ? 1.f : 0.f});
		moveToActor.parameters.vectors.push_back({static_cast<float>(key), 0.f, 0.f});
		moveToActor.preconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::Blackboard_ValueNotNull, 1});
		
		moveToActor.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::NearActor, parameterIndex});
		
		return moveToActor;
	}
	#endif

	Task create_Browse()
	{
		Task browse  {"browse", TaskType::Compound};
		Task selectStack;
		selectStack.action = Action::SelectDestination;
		selectStack.parameters.values.push_back(static_cast<float>(DestinationType::RandomStack));

		auto moveToDestination = create_MoveToDestination(70.f, {0.8f, 0.4f, 0.4f});
		auto wait = create_Wait(5.f);

		browse.addSubtask(selectStack);
		browse.addSubtask(moveToDestination);
		browse.addSubtask(wait);

		return browse;
	}

	Task create_Wander()
	{
		Task wander {"wander", TaskType::Compound};
		Task selectDestination;
		selectDestination.action = Action::SelectDestination;
		selectDestination.parameters.values.push_back(static_cast<float>(DestinationType::RandomLocation));
		
		auto moveToDestination = create_MoveToDestination(70.f, {0.0f, 0.8f, 0.1f});		

		Task stopMoving {"stopMoving"};
		stopMoving.action = Action::StopMoving;

		auto wait = create_Wait(5.f);

		wander.addSubtask(selectDestination);
		wander.addSubtask(moveToDestination);
		wander.addSubtask(stopMoving);
		wander.addSubtask(wait);

		return wander;
	}
	
	Task create_MoveToPlayer()
	{
		Task move {"moveToPlayer"};
		move.action = Action::MoveToPlayer;
		move.preconditions.requiredValues.push_back({WorldStateIdentifier::Alertness, ConditionOp::GreaterEqual, 80.f});
		move.preconditions.requiredFlags.push_back({WorldStateIdentifier::PlayerIdentified, true});
		
		move.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::NearPlayer, -1});

		return move;
	}

	Task create_ChaseSight()
	{
		Task chaseSight {"chaseSight", TaskType::Compound};
		chaseSight.identifier = TaskIdentifier::ChaseSight;
		chaseSight.preconditions.requiredFlags.push_back({WorldStateIdentifier::PlayerIdentified, true});
		chaseSight.preconditions.requiredValues.push_back(
			{WorldStateIdentifier::Alertness, ConditionOp::GreaterThan, 60.f});

		auto bark = create_Bark(Bark::RegainedSightOfPlayer);
		auto move = create_MoveToPlayer();

		move.loop = [](WorldState& state, WorldQuerier const&, Task& task)
		{
			/*auto& target = state.current.vectors[WorldStateIdentifier::PlayerPosition];
			auto& start = state.current.vectors[WorldStateIdentifier::CurrentPosition];
			auto xAxis = target - start;
			xAxis.normalize();
			
			state.animationDriver->isHeadTracking = true;
			state.animationDriver->headRotation = 				
				(FRotationMatrix::MakeFromXZ(xAxis, {0.f, 0.f, 1.f}).ToQuat() * FQuat(FRotator(90, 0, 0)) * FQuat(FRotator(0.f, 0.f, 90.f))).Rotator();
			*/
		};

		move.finally = [](WorldState& state)
		{
			//state.animationDriver->isHeadTracking = false;
		};

		Task stop;
		stop.debugName = "stop";
		stop.action = Action::StopMoving;

		chaseSight.addSubtask(bark);
		chaseSight.addSubtask(move);
		chaseSight.addSubtask(stop);

		chaseSight.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::NearPlayer, -1});

		return chaseSight;
	}

	Task create_ChaseLastKnownPosition()
	{
		Task chaseLastKnownPosition {"chaseLastKnownPosition", TaskType::Compound};
		chaseLastKnownPosition.identifier = TaskIdentifier::ChaseLastKnownPosition;
		chaseLastKnownPosition.preconditions.consumableVectors.push_back(ConsumableFact::Player_LastKnownLocation);
		chaseLastKnownPosition.preconditions.requiredFlags.push_back({WorldStateIdentifier::PlayerIdentified, false});

		auto bark = create_Bark(Bark::LostSightOfPlayer);	

		auto moveToLastKnownLocation = create_MoveToDestination(320.f, {0.f, 1.f, 0.f});
		moveToLastKnownLocation.preconditions.consumableVectors.push_back(ConsumableFact::Player_LastKnownLocation);
		moveToLastKnownLocation.setup = [=](Task& task, WorldState& state, const WorldQuerier&)
		{
			state.current.vectors[WorldStateIdentifier::Destination] = state.facts.vectors[ConsumableFact::Player_LastKnownLocation].value;
		};
		moveToLastKnownLocation.loop = [](WorldState& state, WorldQuerier const&, Task& task)
		{
			/*auto& target = state.current.vectors[WorldStateIdentifier::PlayerPosition];
			auto& start = state.current.vectors[WorldStateIdentifier::CurrentPosition];
			auto xAxis = target - start;
			xAxis.normalize();
			
			state.animationDriver->isHeadTracking = true;
			state.animationDriver->headRotation = 				
				(FRotationMatrix::MakeFromXZ(xAxis, {0.f, 0.f, 1.f}).ToQuat() * FQuat(FRotator(90, 0, 0)) * FQuat(FRotator(0.f, 0.f, 90.f))).Rotator();
			*/
		};

		moveToLastKnownLocation.finish = [](WorldState& state, Task& task)
		{
			state.consumeFactVector(ConsumableFact::Player_LastKnownLocation);
		};

		moveToLastKnownLocation.finally = [](WorldState& state)
		{
			//state.animationDriver->isHeadTracking = false;
		};
		
		chaseLastKnownPosition.addSubtask(bark);
		chaseLastKnownPosition.addSubtask(moveToLastKnownLocation);

		return chaseLastKnownPosition;
	}

	Task create_ChaseLastKnownHeading()
	{
		/*
		1 - move to last known location
		2 - move X distance along heading
		*/

		Task chaseHeading {"chaseHeading", TaskType::Compound};
		chaseHeading.identifier = TaskIdentifier::ChaseLastKnownHeading;
		chaseHeading.preconditions.consumableVectors.push_back(ConsumableFact::Player_ForwardVector);
		chaseHeading.preconditions.requiredFlags.push_back({WorldStateIdentifier::PlayerIdentified, false});

		Task moveAlongHeading {"moveAlongHeading"};
		//amount to move along heading
		//note: should change based when alt paths appear, such as hallway with other paths 
		moveAlongHeading.parameters.values.push_back(320);
		moveAlongHeading.parameters.values.push_back(1000);
		moveAlongHeading.parameters.vectors.push_back({0.f, 0.f, 1.f});
		moveAlongHeading.preconditions.consumableVectors.push_back(ConsumableFact::Player_ForwardVector);
		moveAlongHeading.action = Action::MoveToDestination;		
		moveAlongHeading.setup = [=](Task& task, WorldState& state, const WorldQuerier&)
		{
			auto& currentPosition = state.current.vectors[WorldStateIdentifier::CurrentPosition];
			auto playerForwardVector = state.facts.vectors[ConsumableFact::Player_ForwardVector].value;

			auto destination = currentPosition + playerForwardVector * task.parameters.values[1];
			state.current.vectors[WorldStateIdentifier::Destination] = destination;
		};

		moveAlongHeading.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::NearDestination, -1});
		moveAlongHeading.finish = [](WorldState& state, Task& task)
		{
			state.consumeFactVector(ConsumableFact::Player_ForwardVector);
		};

		Task stopMoving {"stopMoving"};
		stopMoving.action = Action::StopMoving;

		chaseHeading.addSubtask(moveAlongHeading);
		chaseHeading.addSubtask(stopMoving);		

		return chaseHeading;
	}	

	Task create_Chase(TaskDatabase& tasks)
	{
		Task chase {"chase", TaskType::Abstract};
		chase.identifier = TaskIdentifier::Chase;

		auto chaseSight = create_ChaseSight();
		auto chasePosition = create_ChaseLastKnownPosition();
		auto chaseHeading = create_ChaseLastKnownHeading();

		tasks.addTask(TaskIdentifier::ChaseSight, chaseSight);
		tasks.addTask(TaskIdentifier::ChaseLastKnownPosition, chasePosition);
		//tasks.addTask(TaskIdentifier::ChaseLastKnownHeading, chaseHeading);

		tasks.abstractTaskImplementations[TaskIdentifier::Chase].push_back(chaseSight);
		tasks.abstractTaskImplementations[TaskIdentifier::Chase].push_back(chasePosition);
		//tasks.abstractTaskImplementations[TaskIdentifier::Chase].push_back(chaseHeading);

		return chase;		
	}

	Task create_Search()
	{
		Task search {"search", TaskType::Compound};

		/*
		stop moving
		"huh? where'd he go?"
		look around
		"must be around here somewhere..."
		pick random spots to walk to
		*/
		auto expressConfusion = create_Bark(Bark::LostSightOfPlayer);		

		Task searchImplementation {"searchImplementation", TaskType::Recursive};
		searchImplementation.maxRepeats = 5;
		searchImplementation.postconditions.requiredFlags.push_back({WorldStateIdentifier::PlayerIdentified, true});

		//auto lookAround = create_PlayMontage(Montage::LookAround);		

		Task selectDestination {"selectDestination"};
		selectDestination.action = Action::SelectDestination;

		selectDestination.parameters.values.push_back(static_cast<float>(DestinationType::RandomLocation));

		auto bark_playerSearch = create_Bark(Bark::StartPlayerSearch);		

		Task moveToDestination {"moveToDestination"};
		moveToDestination.action = Action::MoveToDestination;
		moveToDestination.parameters.values.push_back(110);	
		moveToDestination.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::NearDestination, -1});
				
		Task stop {"stop"};
		stop.action = Action::StopMoving;

		//searchImplementation.addSubtask(lookAround);
		searchImplementation.addSubtask(bark_playerSearch);
		searchImplementation.addSubtask(selectDestination);
		searchImplementation.addSubtask(moveToDestination);
		searchImplementation.addSubtask(stop);

		search.addSubtask(expressConfusion);
		search.addSubtask(searchImplementation);

		search.postconditions.requiredFlags.push_back({WorldStateIdentifier::PlayerIdentified, true});
		search.finally = [](WorldState& state)
		{
			state.consumeFactFlag(ConsumableFact::HasLead);
		};

		return search;
	}

	Task create_Stare()
	{
		Task stare {"stare", TaskType::Compound};

		Task focusPlayer {"focusPlayer"};
		focusPlayer.action = Action::FocusPlayer;

		auto bark = create_Bark(Bark::StaringAtPlayer);

		//auto playMontage = create_PlayMontage(Montage::Stare);

		stare.addSubtask(focusPlayer);
		stare.addSubtask(bark);
		//stare.addSubtask(playMontage);

		stare.postconditions.requiredFlags.push_back({WorldStateIdentifier::PlayerIdentified, true});

		return stare;
	}

	Task create_Bother()
	{
		Task botherGroup {"bother", TaskType::Compound};
		botherGroup.preconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::NearPlayer, -1});

		auto bark = create_Bark(Bark::Bother);

		Task bother {"bother"};
		bother.action = Action::BotherPlayer;		
		
		auto wait = create_Wait(3);

		botherGroup.addSubtask(bark);
		botherGroup.addSubtask(bother);
		botherGroup.addSubtask(wait);

		return botherGroup;
	}

	Task create_FaceSound()
	{
		Task faceSound {"faceSound"};
		faceSound.type = TaskType::Compound;

		auto bark = create_Bark(Bark::HeardSomething);
		auto wait = create_Wait(.5f);

		Task lookAt {"lookAt"};
		lookAt.action = Action::LookAt;		
		lookAt.setup = [=](Task& task, WorldState& state, const WorldQuerier&)
		{
			auto position = state.current.vectors[WorldStateIdentifier::CurrentPosition];
			auto noiseDisturbance = state.facts.vectors[ConsumableFact::NoiseDisturbance];
			noiseDisturbance.value.z = position.z;

			unsigned int locusId = state.facts.values[ConsumableFact::NoiseDisturbance].value;
			auto it = std::find_if(begin(state.memory.focusLocus), end(state.memory.focusLocus), 
				[=](const FocusLocus& f){return f.id == locusId;});

			noiseDisturbance.value.x = it->engrams[0].stimulus.x;
			noiseDisturbance.value.y = it->engrams[0].stimulus.y;

			task.parameters.vectors.push_back({0, 1, 0});
			task.parameters.vectors.push_back(noiseDisturbance.value);
			/*task.parameters.values.push_back(0); //interpDt
			task.parameters.values.push_back(1);*/
			task.parameters.values.push_back(0); //needToCalc 
		}; 
		lookAt.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::TimeElapsed, 0});		

		//auto stare = create_Stare();
		/*auto playMontage = create_PlayMontage(Montage::Stare);

		playMontage.loop = [](WorldState& state, WorldQuerier const&, Task& task)
		{
			auto& curiosity = state.current.values[WorldStateIdentifier::Curiosity];
			curiosity = FMath::Clamp(curiosity - 0.01f, 0.f, 100.f);
		};*/

		faceSound.addSubtask(bark);
		faceSound.addSubtask(lookAt);
		//faceSound.addSubtask(playMontage);

		faceSound.finally = [](WorldState& state)
		{
			state.consumeFactVector(ConsumableFact::NoiseDisturbance);
		};

		return faceSound;
	}

	Task create_InvestigateSound()
	{
		Task investigateSound {"investigateSound", TaskType::Compound};
		investigateSound.preconditions.consumableValues.push_back(ConsumableFact::Lead);

		auto bark = create_Bark(Bark::BeginInvestigation);

		auto moveToSound = create_MoveToDestination(200.f, {0.f, 1.f, 0.f});//, 1.f, true, true, 1);
		moveToSound.setup = [=](Task& task, WorldState& state, const WorldQuerier&)
		{
			auto currentPosition = state.current.vectors[WorldStateIdentifier::CurrentPosition];
			unsigned int locusId = state.facts.values[ConsumableFact::Lead].value;
			auto it = std::find_if(begin(state.memory.focusLocus), end(state.memory.focusLocus), 
				[=](const FocusLocus& f){return f.id == locusId;});

			if (it != end(state.memory.focusLocus))
			{
					state.current.vectors[WorldStateIdentifier::Destination] = Math::Vector3 {it->engrams.back().stimulus.x, it->engrams.back().stimulus.y, currentPosition.z};
			}
			else
			{
				state.current.vectors[WorldStateIdentifier::Destination] = currentPosition;
				state.consumeFactValue(ConsumableFact::Lead);
			}
		};

		Task stopMoving {"stopMoving"};
		stopMoving.action = Action::StopMoving;

		Task lookAround {"lookAround"};
		lookAround.parameters.vectors.push_back({});
		lookAround.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{		
			/*auto requestId = state.animationDriver->reactionDriver->addReaction({EReactionType::Sight, 50.f, 0.f});
			task.parameters.vectors[0].x = requestId;*/						
		};

		lookAround.loop = [](WorldState& state, WorldQuerier const&, Task& task)
		{
			auto& curiosity = state.current.values[WorldStateIdentifier::Curiosity];
			curiosity = clamp(curiosity - 10.f / 30.f, 0.f, 100.f);
		};

		lookAround.finally = [](WorldState& state)
		{
			//state.animationDriver->reactionDriver->tracker->stop();
		};

		lookAround.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::ReactionFinished, 0});

		//auto wait = create_Wait(30);

		investigateSound.addSubtask(bark);
		investigateSound.addSubtask(moveToSound);
		investigateSound.addSubtask(stopMoving);
		investigateSound.addSubtask(lookAround);
		//investigateSound.addSubtask(playMontage);
		
		investigateSound.postconditions.consumableFlags.push_back(ConsumableFact::HasLead);
		investigateSound.finally = [](WorldState& state)
		{
			state.consumeFactValue(ConsumableFact::Lead);
			state.produceFactFlag(ConsumableFact::HasLead, true);
			state.produceFactFlag(ConsumableFact::LostLead, true);
		};

		return investigateSound;
	}

	#if 0
	auto create_FollowPath() -> Task
	{
		Task followPath {"followPath", TaskType::Recursive};
		followPath.parameters.vectors.push_back({false, 0, 0});
		followPath.maxRepeats = 1;

		followPath.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::ParameterFlag, 0});
		followPath.finally = [](WorldState& state)
		{
			state.consumeFactValue(ConsumableFact::NoiseDisturbance);
			state.produceFactFlag(ConsumableFact::HasLead, true);
		};

		followPath.loop = [](WorldState& state, WorldQuerier const&, Task& task)
		{
			//figure out how many repeats we'll need
			int locusId = state.facts.values[ConsumableFact::NoiseDisturbance].value;
			auto it = std::find_if(begin(state.memory.focusLocus), end(state.memory.focusLocus), 
				[=](const FocusLocus& f){return f.id == locusId;});

			if (it != end(state.memory.focusLocus))
			{
				//pick engrams one second apart from eachother
				int previousId = 0;
				int previousAge = it->engrams[0].age;
				bool found = false;
				const int oneSecond = 3;//frames
				std::vector<int> ids;
				ids.push_back(previousId);

				for (int i = 1; i < it->engrams.size(); i++)
				{
					found = false;

					if (previousAge - it->engrams[i].age > oneSecond)
					{
						found = true;
						previousId = i;
						previousAge = it->engrams[i].age;
						ids.push_back(i);
					}
				}

				if (!found)
				{
					ids.push_back(it->engrams.size() - 1);
				}

				//now that we have the line segments, find the closest segment
				struct LineSegment
				{ 
					int start, end;
				};
				std::vector<LineSegment> segments;
				for (int i = 0; i < ids.size() - 1; i++)
				{
					segments.push_back({ids[i], ids[i+1]});
				}

				auto currentPosition = state.current.vectors[WorldStateIdentifier::CurrentPosition];

				auto closestSegment = std::min_element(begin(segments), end(segments),
					[&](const LineSegment& first, const LineSegment& second)
				{
					return FMath::PointDistToSegmentSquared(
						currentPosition,
						{it->engrams[first.start].stimulus.x, it->engrams[first.start].stimulus.y, currentPosition.z}, 
						{it->engrams[first.end].stimulus.x, it->engrams[first.end].stimulus.y, currentPosition.z})

						< FMath::PointDistToSegmentSquared(
							currentPosition,
							{it->engrams[second.start].stimulus.x, it->engrams[second.start].stimulus.y, currentPosition.z},
							{it->engrams[second.end].stimulus.x, it->engrams[second.end].stimulus.y, currentPosition.z});
				});
				
				Math::Vector3 start {it->engrams[closestSegment->start].stimulus.x, it->engrams[closestSegment->start].stimulus.y, currentPosition.z};
				Math::Vector3 end {it->engrams[closestSegment->end].stimulus.x, it->engrams[closestSegment->end].stimulus.y, currentPosition.z};
				auto closestPointOnLine = FMath::ClosestPointOnSegment(currentPosition, start, end);				

				//if we're >90% of the way to the end, set target to next segment
				auto t = dot((closestPointOnLine - start), end - start) / (end - start).Size();

				if (t > 0.9f)
				{
					if (closestSegment != ((segments.end()) - 1))
					{
						closestSegment++;						
						task.remainingRepeats++;
					}
					else 
					{
						task.parameters.vectors[0].x = true;
					}
				}

				state.current.vectors[WorldStateIdentifier::Destination] = {it->engrams[closestSegment->end].stimulus.x, it->engrams[closestSegment->end].stimulus.y, currentPosition.z};
			}
		};

		auto moveToDestination = create_MoveToDestination(200.f, {0.3f, 0.1f, 0.7f});

		followPath.addSubtask(moveToDestination);

		return followPath;
	}
	#endif

	#if 0
	auto create_pickNearbySeat() -> Task
	{
		Task selectSeat {"pickNearbySeat"};
		selectSeat.action = Action::Query;
		selectSeat.identifier = TaskIdentifier::FindSeat_PickNearbySeat;
		selectSeat.parameters.values.push_back(static_cast<float>(DestinationType::AnyNearbySeat));		
		selectSeat.parameters.vectors.push_back({false, 0, 0});
		selectSeat.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{
			worldQuerySystem.queryEnvironment(EEnvironmentQueryId::FindNearbySeat, EEnvQueryRunMode::Type::RandomBest25Pct,
				FQueryFinishedSignature::CreateLambda([&](std::shared_ptr<FEnvQueryResult>& queryResult)
				{
					task.parameters.vectors[0].x = true;
					
					auto result = FindAvailableSeat(queryResult);

					if (std::get<0>(result))
					{
						auto& seat = std::get<1>(result)->Seats[std::get<2>(result)];
						seat.isTaken = true;
						state.produceFactFlag(ConsumableFact::HasPickedSeat, true);
						state.storeInBlackboard(FName {*UEnum::GetValueAsString(TEXT("/Script/WoodenSphere.EBlackboardKey"), EBlackboardKey::Seat)}, seat.target);
					}
					else
					{
						task.failed = true;
					}
				}
				));
		};
		selectSeat.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::ParameterFlag, 0});
		selectSeat.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::Blackboard_ValueNotNull, static_cast<int>(selectSeat.parameters.vectors.size())});
		selectSeat.parameters.vectors.push_back({static_cast<float>(EBlackboardKey::Seat), 0, 0});
		selectSeat.postconditions.consumableFlags.push_back(ConsumableFact::HasPickedSeat);

		return selectSeat;
	}

	auto create_searchAreaForSeat() -> Task
	{
		Task searchAreaForSeat {"searchAreaForSeat", TaskType::Compound};
		searchAreaForSeat.identifier = TaskIdentifier::FindSeat_SearchAreaForSeat;

		auto searchArea = create_MoveToDestination(70.f, {0.f, 0.5f, 0.5f});
		searchArea.parameters.values.push_back(0.f);
		searchArea.parameters.vectors.push_back({false, 0, 0});
		searchArea.breakConditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::ParameterFlag, 1});//TODO: parameterflag should be customizable by having an index

		searchArea.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{
			//std::swap(task.parameters.values[0], task.parameters.values[1]);
			std::vector<FEnvironmentQueryParameter> parameters;
			parameters.Add({EEnvironmentQueryParameterId::MinDistance, 4000});
			parameters.Add({EEnvironmentQueryParameterId::MaxDistance, 7000});

			worldQuerySystem.queryEnvironment(EEnvironmentQueryId::PickRandomLocation, EEnvQueryRunMode::Type::RandomBest25Pct, parameters,			
				FQueryFinishedSignature::CreateLambda([&](std::shared_ptr<FEnvQueryResult>& queryResult)
			{
				state.current.vectors[WorldStateIdentifier::Destination] = queryResult->GetItemAsLocation(0);
				//std::swap(task.parameters.values[0], task.parameters.values[1]);
			}));
		};

		searchArea.loop = [](WorldState& state, WorldQuerier const& worldQuerySystem, Task& task)
		{
			/*worldQuerySystem.queryEnvironmentWithBlackboard(EEnvironmentQueryId::FindNearbySeat, EBlackboardKey::Seat, EEnvQueryRunMode::Type::RandomBest25Pct,
				[&](std::shared_ptr<FEnvQueryResult>& queryResult)*/
			worldQuerySystem.queryEnvironment(EEnvironmentQueryId::FindNearbySeat, EEnvQueryRunMode::Type::RandomBest25Pct,
				FQueryFinishedSignature::CreateLambda([&](std::shared_ptr<FEnvQueryResult>& queryResult)
				{
					auto result = FindAvailableSeat(queryResult);

					if (std::get<0>(result))
					{
						//task.parameters.flags[0] = true;
						task.parameters.vectors[1].x = true;
						/*auto& seat = std::get<1>(result)->Seats[std::get<2>(result)];
						seat.isTaken = true;
						state.current.vectors[WorldStateIdentifier::Destination] = seat.target->GetComponentLocation();*/
						auto& seat = std::get<1>(result)->Seats[std::get<2>(result)];
						seat.isTaken = true;
						state.produceFactFlag(ConsumableFact::HasPickedSeat, true);
						state.storeInBlackboard(FName {*UEnum::GetValueAsString(TEXT("/Script/WoodenSphere.EBlackboardKey"), EBlackboardKey::Seat)}, seat.target);
					}					
				}
				));
		};

		auto pickNearbySeat = create_pickNearbySeat();

		searchAreaForSeat.addSubtask(searchArea);
		searchAreaForSeat.addSubtask(pickNearbySeat);

		searchAreaForSeat.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::Blackboard_ValueNotNull, static_cast<int>(searchAreaForSeat.parameters.vectors.size())});
		searchAreaForSeat.parameters.vectors.push_back({static_cast<float>(EBlackboardKey::Seat), 0, 0});
		searchAreaForSeat.postconditions.consumableFlags.push_back(ConsumableFact::HasPickedSeat);

		return searchAreaForSeat;
	}

	auto create_FindSeat(TaskDatabase& tasks) -> Task
	{
		Task findSeat {"findSeat", TaskType::Abstract};
		findSeat.identifier = TaskIdentifier::FindSeat;

		auto pickNearbySeat = create_pickNearbySeat();
		auto searchAreaForSeat = create_searchAreaForSeat();

		tasks.addTask(TaskIdentifier::FindSeat_PickNearbySeat, pickNearbySeat);
		tasks.addTask(TaskIdentifier::FindSeat_SearchAreaForSeat, searchAreaForSeat);

		tasks.abstractTaskImplementations[TaskIdentifier::FindSeat].push_back(pickNearbySeat);
		tasks.abstractTaskImplementations[TaskIdentifier::FindSeat].push_back(searchAreaForSeat);

		findSeat.postconditions.consumableFlags.push_back(ConsumableFact::HasPickedSeat);

		return findSeat;
	}

	auto create_Relax() -> Task
	{
		Task relax {"relax", TaskType::Compound};
		relax.preconditions.consumableFlags.push_back(ConsumableFact::HasPickedSeat);
		relax.preconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::Blackboard_ValueNotNull, static_cast<int>(relax.parameters.vectors.size())});
		relax.parameters.vectors.push_back({static_cast<float>(EBlackboardKey::Seat), 0, 0});

		auto bark = create_Bark(Bark::GoingToSit);

		auto moveToSeat = create_MoveToDestination(70.f, {0.f, 0.5f, 0.5f}, 1.f, false, true, 1);
		moveToSeat.parameters.vectors.push_back({80.f, 0.f, 0.f});
		
		moveToSeat.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{
			auto seat = Cast<USceneComponent>(state.blackboard->GetValueAsObject(FName {*UEnum::GetValueAsString(TEXT("/Script/WoodenSphere.EBlackboardKey"), EBlackboardKey::Seat)}));
			state.current.vectors[WorldStateIdentifier::Destination] = seat->GetComponentLocation();
		};

		moveToSeat.loop = [](WorldState& state, WorldQuerier const& worldQuerySystem, Task& task) 
		{
			auto& currentPosition = state.current.vectors.at(WorldStateIdentifier::CurrentPosition_Feet);
			auto& destination = state.current.vectors.at(WorldStateIdentifier::Destination);

			if (distanceSquared(currentPosition, destination) < 3000.f)
			{
				task.parameters.values[1] = 1.f;
				task.parameters.vectors[0].z = 0.f;
			}
		};

		auto faceAwayFromSeat = create_PlayBlendAnimation_SourceCalculate(EAnimationId::Turn, true,
			[](WorldState& state, auto& worldQuerySystem, auto& task) 
		{
			/*worldQuerySystem.queryEnvironment(EEnvironmentQueryId::FindNearbySeat, EEnvQueryRunMode::Type::RandomBest25Pct,
					FQueryFinishedSignature::CreateLambda([](auto& queryResult)
					{
						int pause = 0;	
					})
				);
			return Math::Vector3{0.f, 0.f, 0.f};*/
			
			auto seat = Cast<USceneComponent>(state.blackboard->GetValueAsObject(FName {*UEnum::GetValueAsString(TEXT("/Script/WoodenSphere.EBlackboardKey"), EBlackboardKey::Seat)}));
			
			bool isLeftOfMe = dot(worldQuerySystem.getRightVector(), seat->GetOwner()->GetActorForwardVector()) < 0.f;
			
			auto oldMin = 0.f;
			auto oldMax = 3.14159f;
			auto oldRange = oldMax - oldMin;
			auto newRange = 100.f - 0.f;
			auto newMin = 0.f;
			
			auto oldValue = acos(dot(worldQuerySystem.getForwardVector(), seat->GetOwner()->GetActorForwardVector()));
			auto newValue = (((oldValue - oldMin) * newRange) / oldRange) + newMin;
			//GEngine->AddOnScreenDebugMessage(0, 100, FColor::Black, std::string::FromInt(100.f * oldValue));
			//GEngine->AddOnScreenDebugMessage(1, 100, FColor::Black, std::string::FromInt(newValue));
			return Math::Vector3{!isLeftOfMe ? 100.f : 100.f, newValue, 0.f};
		});

		//auto sitDown = create_PlayAnimation(EAnimationId::SitDown, EAnimationType::Animation);
		Task sitDown;
		sitDown.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{
			state.animationDriver->isSitting = true;
		};

		Task stopMoving;
		stopMoving.action = Action::StopMoving;
		
		auto wait = create_Wait(60);
		wait.finally = [](WorldState& state)
		{
			state.animationDriver->isSitting = false;
		};

		relax.addSubtask(bark);
		relax.addSubtask(moveToSeat);
		relax.addSubtask(stopMoving);
		relax.addSubtask(faceAwayFromSeat);
		relax.addSubtask(sitDown);
		relax.addSubtask(wait);
		//relax.addSubtask(sitDown);

		auto m = create_MoveToDestination(70.f, {0.f, 0.5f, 0.5f});
		m.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{
			state.current.vectors[WorldStateIdentifier::Destination] = Math::Vector3{4000.f, -4000.f, 20.f};
		};
		//relax.addSubtask(m);


		relax.postconditions.requiredValues.push_back({WorldStateIdentifier::Stance, ConditionOp::EqualTo, static_cast<float>(Stance::Sitting)});

		return relax;		
	}
	#endif

	

	#if 0
	auto create_React_Dismiss() -> Task
	{
		Task reactDismiss;
		reactDismiss.type = TaskType::Compound;
		reactDismiss.preconditions.consumableFlags.push_back(ConsumableFact::LostLead);//HasLead);

		auto bark = create_Bark(Bark::Dismiss);

		Task react {"reactDismiss"};
		react.action = Action::Reaction;		
		react.parameters.vectors.push_back({});
		react.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{
			auto requestId = state.animationDriver->reactionDriver->addReaction({EReactionType::Dismiss});
			task.parameters.vectors[0].x = requestId;

			if (state.animationDriver->isSitting)
			{
				state.animationDriver->isSitting = false;
			}
		};

		react.finish = [](WorldState& state, Task& task)
		{
			state.consumeFactFlag(ConsumableFact::LostLead);//HasLead);
			state.animationDriver->reactionDriver->tracker->stop();
		};

		react.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::ReactionFinished, 0});

		reactDismiss.addSubtask(bark);
		reactDismiss.addSubtask(react);

		return reactDismiss;
	}
	
	auto create_React_FoundPlayer() -> Task
	{
		Task foundPlayer;
		foundPlayer.type = TaskType::Compound;
		foundPlayer.preconditions.consumableFlags.push_back(ConsumableFact::PlayerRecentlyIdentified);
		
		auto bark = create_Bark(Bark::FoundPlayer);

		Task react {"foundPlayer"};
		react.action = Action::Reaction;
		react.parameters.vectors.push_back({});
		react.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{			
			auto requestId = state.animationDriver->reactionDriver->addReaction({EReactionType::FoundPlayer, FMath::RandBool() ? 25.f : 0.f, 0.f});
			task.parameters.vectors[0].x = requestId;

			auto& playerLocation = state.current.vectors[WorldStateIdentifier::PlayerPosition];
			auto faceDirectionArgs = calculateFaceDirectionArgs(playerLocation.x, playerLocation.y, worldQuerySystem);
			state.animationDriver->faceDirection(std::get<0>(faceDirectionArgs), std::get<1>(faceDirectionArgs));

			if (state.animationDriver->isSitting)
			{
				state.animationDriver->isSitting = false;
			}
		};

		react.loop = [](WorldState& state, WorldQuerier const&, Task& task)
		{
			auto& target = state.current.vectors[WorldStateIdentifier::PlayerPosition];
			auto& start = state.current.vectors[WorldStateIdentifier::CurrentPosition];
			auto xAxis = target - start;
			xAxis.normalize();
			
			state.animationDriver->isHeadTracking = true;
			state.animationDriver->headRotation = 				
				(FRotationMatrix::MakeFromXZ(xAxis, {0.f, 0.f, 1.f}).ToQuat() * FQuat(FRotator(90, 0, 0)) * FQuat(FRotator(0.f, 0.f, 90.f))).Rotator();
		};

		react.finally = [](WorldState& state)
		{			
			state.consumeFactFlag(ConsumableFact::PlayerRecentlyIdentified);
			state.animationDriver->reactionDriver->tracker->stop();
			state.animationDriver->isHeadTracking = false;
		};

		react.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::ReactionFinished, 0});

		foundPlayer.addSubtask(bark);
		foundPlayer.addSubtask(react);

		return foundPlayer;
	}
	
	auto create_React_LostPlayer() -> Task
	{
		Task lostPlayer;
		lostPlayer.type = TaskType::Compound;
		lostPlayer.preconditions.consumableFlags.push_back(ConsumableFact::HasLead);

		auto bark = create_Bark(Bark::LostPlayer);

		Task react {"lostPlayer"};
		react.action = Action::Reaction;
		react.parameters.vectors.push_back({});
		react.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{				
			auto& tracker = state.valueTrackers[WorldStateIdentifier::Alertness];
			float energy = tracker.maxValue >= 80.f ?
				75.f : tracker.maxValue >= 70.f ?
				50.f : tracker.maxValue >= 60.f ?
				25.f : 0.f;
			auto requestId = state.animationDriver->reactionDriver->addReaction({EReactionType::LostPlayer,  energy, 0.f});
			task.parameters.vectors[0].x = requestId;
		};

		react.finally = [](WorldState& state)//, Task& task)
		{
			state.consumeFactFlag(ConsumableFact::HasLead);
			state.animationDriver->reactionDriver->tracker->stop();
		};

		react.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::ReactionFinished, 0});

		lostPlayer.addSubtask(bark);
		lostPlayer.addSubtask(react);

		return lostPlayer;
	}

	auto create_React_Sight() -> Task
	{
		Task reactSight;
		reactSight.type = TaskType::Compound;
		reactSight.preconditions.consumableValues.push_back(ConsumableFact::Glimpse);

		auto bark = create_Bark(Bark::SawSomething);

		Task react {"reactSight"};
		react.action = Action::Reaction;
		react.parameters.vectors.push_back({});
		react.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{
			//TODO: change to react task
			//abort reaction, face, wait for face to end
			auto currentReaction = state.animationDriver->reactionDriver->currentReaction;
			//TODO: need to calculate direction/angle for sight reaction
			auto requestId = state.animationDriver->reactionDriver->addReaction({EReactionType::Sight});
			task.parameters.vectors[0].x = requestId;			
			
			auto locus = std::find_if(begin(state.memory.focusLocus), end(state.memory.focusLocus), 
				[id = FMath::RoundToInt(state.facts.values[ConsumableFact::Glimpse].value)]
				(auto& l) {return l.id == id;});
			
			auto faceDirectionArgs = calculateFaceDirectionArgs(locus->engrams[0].saw.x, locus->engrams[0].saw.y, worldQuerySystem);
			state.animationDriver->faceDirection(std::get<0>(faceDirectionArgs), std::get<1>(faceDirectionArgs));

			if (state.animationDriver->isSitting)
			{
				state.animationDriver->isSitting = false;
			}
		};

		react.breakConditions.requiredFlags.push_back({WorldStateIdentifier::PlayerIdentified, true});

		react.finally = [](WorldState& state)
		{
			state.consumeFactValue(ConsumableFact::Glimpse);
			state.produceFactFlag(ConsumableFact::HasLead, true);
			state.animationDriver->reactionDriver->tracker->stop();
			state.animationDriver->reactionDriver->abortReaction();
		};

		react.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::ReactionFinished, 0});

		reactSight.addSubtask(bark);
		reactSight.addSubtask(react);

		return reactSight;
	}

	auto create_React_Sound() -> Task
	{
		Task reactSound;
		reactSound.type = TaskType::Compound;
		reactSound.preconditions.consumableValues.push_back(ConsumableFact::NoiseDisturbance);

		auto bark = create_Bark(Bark::HeardSomething);

		Task react {"reactSound"};
		react.action = Action::Reaction;
		react.parameters.vectors.push_back({});
		react.setup = [](Task& task, WorldState& state, WorldQuerier const& worldQuerySystem)
		{
			/*
			figure out the direction the sound came from, convert it to use the reaction blendspace's range of 0-100
			0: left
			25: behind
			50: behindmild
			75: right

			0: right
			20: behind_right
			40: behind_mild_right
			60: behind_mild_left
			80: behind_left
			100: left
			*/
			
			auto locus = std::find_if(begin(state.memory.focusLocus), end(state.memory.focusLocus), 
				[id = FMath::RoundToInt(state.facts.values[ConsumableFact::NoiseDisturbance].value)]
			(auto& l) {return l.id == id;});
			
			if (locus == end(state.memory.focusLocus) || locus->engrams.empty())
			{
				task.failed = true;
				return;
			}

			auto relativeRotation = worldQuerySystem.calculateRelativeRotationTo(locus->engrams[0].stimulus.x, locus->engrams[0].stimulus.y);//noiseLocation.x, noiseLocation.y);
			float angle;
			//TODO: make use of behind_mild_{left/right} with energy
			if (std::get<1>(relativeRotation)) //is behind
			{
				angle = std::get<0>(relativeRotation) > 0.f ? 20.f : 80.f;
			}
			else
			{
				angle = std::get<0>(relativeRotation) > 0.f ? 0.f : 100.f;
			}

			auto energy {locus->currentImportance};
			auto requestId = state.animationDriver->reactionDriver->addReaction({EReactionType::Sound, angle, energy});
			task.parameters.vectors[0].x = requestId;

			if (state.animationDriver->isSitting)
			{
				state.animationDriver->isSitting = false;
			}
		};

		react.finally = [](WorldState& state)
		{
			auto fact = state.consumeFactValue(ConsumableFact::NoiseDisturbance);			
			state.produceFactFlag(ConsumableFact::HasLead, true);
			state.produceFactValue(ConsumableFact::Lead, fact.value);
			state.animationDriver->reactionDriver->tracker->stop();
		};

		react.postconditions.satisfiedPredicates.push_back({SatisfiablePredicateIdentifier::ReactionFinished, 0});

		reactSound.addSubtask(bark);
		reactSound.addSubtask(react);

		return reactSound;
	}

	auto create_TrackHead() -> Task
	{
		Task trackHead {"trackHead"};
		trackHead.preconditions.requiredFlags.push_back({WorldStateIdentifier::PlayerIdentified, true});

		trackHead.loop = [](WorldState& state, WorldQuerier const& worldQuerySystem, Task& task)
		{
			auto& target = state.current.vectors[WorldStateIdentifier::PlayerPosition];
			auto& start = state.current.vectors[WorldStateIdentifier::CurrentPosition];
			auto xAxis = target - start;
			xAxis.normalize();
			
			state.animationDriver->isHeadTracking = true;
			state.animationDriver->headRotation = 				
				(FRotationMatrix::MakeFromXZ(xAxis, {0.f, 0.f, 1.f}).ToQuat() * FQuat(FRotator(90, 0, 0)) * FQuat(FRotator(0.f, 0.f, 90.f))).Rotator();
			
			if (dot(worldQuerySystem.getForwardVector(), xAxis) < 0.7071f)
			{
				auto faceDirectionArgs = calculateFaceDirectionArgs(target.x, target.y, worldQuerySystem);
				state.animationDriver->faceDirection(std::get<0>(faceDirectionArgs), std::get<1>(faceDirectionArgs));
			}			
		};		

		trackHead.finally = [](WorldState& state)
		{
			state.animationDriver->isHeadTracking = false;
			state.animationDriver->needsToFaceDirection = false;
		};

		return trackHead;
	}
	#endif

	Task create_Null()
	{
		Task null {"null"};

		return null;
	}

	void setupPredicates(TaskDatabase& tasks)
	{
		//const int numberOfPredicates = 4;
		//tasks.satisfiablePredicates.reserve(numberOfPredicates);

		tasks.satisfiablePredicates.push_back({SatisfiablePredicateIdentifier::NearPlayer,
			[](const WorldState& state, const Task& task, int parameterIndex)
		{
			auto& currentPosition = state.current.vectors.at(WorldStateIdentifier::CurrentPosition);
			auto& destination = state.current.vectors.at(WorldStateIdentifier::PlayerPosition);

			return distanceSquared(currentPosition, destination) < 100000.f;
		}});

		tasks.satisfiablePredicates.push_back({SatisfiablePredicateIdentifier::NearDestination,
			[](const WorldState& state, const Task& task, int parameterIndex)
		{
			auto& currentPosition = state.current.vectors.at(WorldStateIdentifier::CurrentPosition_Feet);
			auto& destination = state.current.vectors.at(WorldStateIdentifier::Destination);

			float minimumDistance = 50000.f;

			if (parameterIndex != -1)
			{
				minimumDistance = task.parameters.vectors[parameterIndex].x;
			}
			
			return distanceSquared(currentPosition, destination) < minimumDistance;
		}});

		/*tasks.satisfiablePredicates.push_back({SatisfiablePredicateIdentifier::NearActor,
			[](const WorldState& state, const Task& task, int parameterIndex)
		{
			auto& currentPosition = state.current.vectors.at(WorldStateIdentifier::CurrentPosition_Feet);

			auto& parameters = task.parameters.vectors[parameterIndex];
			EBlackboardKey blackboardKey {static_cast<EBlackboardKey>(FMath::RoundToInt(parameters.x))};
			auto destination = Cast<Actor>(state.blackboard->GetValueAsObject({*UEnum::GetValueAsString(TEXT("/Script/WoodenSphere.EBlackboardKey"), blackboardKey)}))->GetActorLocation();

			float minimumDistance = 50000.f;

			if (parameterIndex != -1)
			{
				minimumDistance = task.parameters.vectors[parameterIndex].y;
			}
			
			return distanceSquared(currentPosition, destination) < minimumDistance;
		}});*/

		tasks.satisfiablePredicates.push_back({SatisfiablePredicateIdentifier::TimeElapsed,			
			[](const WorldState& state, const Task& task, int parameterIndex)
		{
			auto& parameters = task.parameters.vectors[parameterIndex];
			return parameters.x >= parameters.y;
		}});

		tasks.satisfiablePredicates.push_back({SatisfiablePredicateIdentifier::ParameterFlag,
			[](const WorldState& state, const Task& task, int parameterIndex)
		{
			auto& parameters = task.parameters.vectors[parameterIndex];
			return parameters.x != 0;
		}});

		/*tasks.satisfiablePredicates.push_back({SatisfiablePredicateIdentifier::Blackboard_ValueNotNull,
			[](const WorldState& state, const Task& task, int parameterIndex)
		{
			auto& parameters = task.parameters.vectors[parameterIndex];
			EBlackboardKey blackboardKey {static_cast<EBlackboardKey>(FMath::RoundToInt(parameters.x))};
			return state.blackboard->GetValueAsObject({*UEnum::GetValueAsString(TEXT("/Script/WoodenSphere.EBlackboardKey"), blackboardKey)}) != nullptr;
		}});*/

		/*tasks.satisfiablePredicates.push_back({SatisfiablePredicateIdentifier::ReactionFinished,
			[](const WorldState& state, const Task& task, int parameterIndex)
		{
			auto& parameters = task.parameters.vectors[parameterIndex];
			return state.animationDriver->reactionDriver->tracker->getRequestId() == FMath::RoundToInt(parameters.x)
				&& state.animationDriver->reactionDriver->tracker->reactionHasFinished();
		}});*/

	}

	bool operator==(const TaskVertex& lhs, const TaskVertex& rhs)
	{
		return lhs.identifier == rhs.identifier;
	}

	bool areAdjacent(Task& a, Task& b)
	{
		auto& precondition = a.preconditions;		

		for (auto& flag : precondition.requiredFlags)
		{
			const auto& matchingFlag = std::find_if(begin(b.postconditions.requiredFlags), end(b.postconditions.requiredFlags),
				[flag](auto& f) {return f.id == flag.id;});

			if (matchingFlag != end(b.postconditions.requiredFlags))
			{
				if (flag.flag == matchingFlag->flag)
				{
					return true;
				}
			}
		}

		for (auto& value : precondition.requiredValues)
		{
			const auto& matchingValue = std::find_if(begin(b.postconditions.requiredValues), end(b.postconditions.requiredValues),
				[value](auto& v){return v.id == value.id;});

			if (matchingValue != end(b.postconditions.requiredValues))
			{
				switch(matchingValue->op)
				{
					case ConditionOp::LessThan
						:
					{
						return canLessThanSatisfy(matchingValue->value, value.op, value.value);
					}
					case ConditionOp::GreaterThan
						:
					{
						return canGreaterThanSatisfy(matchingValue->value, value.op, value.value);
					}
					case ConditionOp::EqualTo
						:
					{
						return canEqualToSatisfy(matchingValue->value, value.op, value.value);
					}
					case ConditionOp::NotEqualTo
						:
					{
						return canNotEqualToSatisfy(matchingValue->value, value.op, value.value);
					}
					case ConditionOp::LessEqual
						:
					{
						return canLessEqualSatisfy(matchingValue->value, value.op, value.value);
					}
					case ConditionOp::GreaterEqual
						:
					{
						return canGreaterEqualSatisfy(matchingValue->value, value.op, value.value);
					}
				}
			}
		}

		for (auto satisfiedPredicate : precondition.satisfiedPredicates)
		{
			const auto& it = std::find_if(begin(b.postconditions.satisfiedPredicates), end(b.postconditions.satisfiedPredicates),
				[satisfiedPredicate](auto& sp){return sp.identifier == satisfiedPredicate.identifier;});	
			
			if (it != end(b.postconditions.satisfiedPredicates))
			{
				return true;
			}
		}

		for (auto flag : precondition.consumableFlags)
		{
			if (std::find(begin(b.postconditions.consumableFlags), end(b.postconditions.consumableFlags), flag)
				!= end(b.postconditions.consumableFlags))
			{
				return true;
			}
		}

		for (auto value : precondition.consumableValues)
		{
			if (std::find(begin(b.postconditions.consumableValues), end(b.postconditions.consumableValues), value)
				!= end(b.postconditions.consumableValues))
			{
				return true;
			}
		}

		for (auto vector : precondition.consumableVectors)
		{
			if (std::find(begin(b.postconditions.consumableVectors), end(b.postconditions.consumableVectors), vector)
				!= end(b.postconditions.consumableVectors))
			{
				return true;
			}
		}

		return false;
	}

	void TaskDatabase::addTask(TaskIdentifier identifier, Task task)
	{
		task.identifier = identifier;

		tasks[identifier] = task;
		//TODO: add unique vertex id here
		TaskVertex newVertex;
		newVertex.identifier = identifier;

		for(auto& vertex : taskGraph)
		{
			if (areAdjacent(tasks[vertex.identifier], task))
			{
				vertex.adjacentTasks.push_back(task.identifier);
			}

			if (areAdjacent(task, tasks[vertex.identifier]))
			{
				newVertex.adjacentTasks.push_back(vertex.identifier);
			}
		}

		taskGraph.push_back(newVertex);
	}

	TaskDatabase setupTasks()
	{
		TaskDatabase tasks;
		
		tasks.addTask(TaskIdentifier::Browse, create_Browse());
		tasks.addTask(TaskIdentifier::Wander, create_Wander());
		tasks.addTask(TaskIdentifier::Chase, create_Chase(tasks));
		//tasks.addTask(TaskIdentifier::Search, create_Search());
		//tasks.addTask(TaskIdentifier::Stare, create_Stare());
		tasks.addTask(TaskIdentifier::Bother, create_Bother());
		tasks.addTask(TaskIdentifier::FaceSound, create_FaceSound());
		tasks.addTask(TaskIdentifier::InvestigateSound, create_InvestigateSound());
		/*tasks.addTask(TaskIdentifier::FollowPath, create_FollowPath());
		tasks.addTask(TaskIdentifier::FindSeat, create_FindSeat(tasks));
		tasks.addTask(TaskIdentifier::Relax, create_Relax());
		tasks.addTask(TaskIdentifier::React_Dismiss, create_React_Dismiss());
		tasks.addTask(TaskIdentifier::React_FoundPlayer, create_React_FoundPlayer());
		tasks.addTask(TaskIdentifier::React_LostPlayer, create_React_LostPlayer());
		tasks.addTask(TaskIdentifier::React_Sight, create_React_Sight());
		tasks.addTask(TaskIdentifier::React_Sound, create_React_Sound());
		tasks.addTask(TaskIdentifier::TrackHead, create_TrackHead());*/
		tasks.addTask(TaskIdentifier::Null, create_Null());

		//copyConsiderations(availableUTasks, tasks);
		
		setupPredicates(tasks);

		return tasks;

	}
}
