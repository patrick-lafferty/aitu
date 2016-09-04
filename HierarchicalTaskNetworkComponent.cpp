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

#include "HierarchicalTaskNetworkComponent.h"
#include "Math.h"
#include <algorithm>
#include "SoundMap.h"
#include "log.h"

using namespace AI;
using namespace Math;

#define SHOW_PLANNER_INFORMATION_MESSAGES

HierarchicalTaskNetworkComponent::HierarchicalTaskNetworkComponent(Actor* owner)
	: Component(owner)
{
	detailSightRadius = 800.f;
	motionSightRadius = 1600.f;

	detailAngle = 20.f;
	motionAngle = detailAngle * 2.f;

	state.current.vectors[WorldStateIdentifier::CurrentPosition] = {};
	state.current.vectors[WorldStateIdentifier::PlayerPosition] = {1000, 1000, 1000};

	planPath.resize(5);
}

void HierarchicalTaskNetworkComponent::initializeComponent()
{
	auto world = getWorld();
	auto gameMode = world->getAuthGameMode();
	gameMode->registerForFixedTicks(this);
	
	worldQuerySystem.setup(world, owner);

	auto& tasks = gameMode->getAvailableTasks();

	for (auto& kvp : tasks.considerations)
	{
		utilityScores.push_back({tasks.tasks[kvp.first].debugName.c_str(), std::vector<float>(5)});
	}

	if (tasks.considerations.size() < 5)
	{
		for (int j = tasks.considerations.size(); j < 5; j++)
		{
			utilityScores.push_back({"", std::vector<float>(5)});
		}
	}

	state.current.values[WorldStateIdentifier::Stance] = static_cast<float>(Stance::Standing);
	
	state.valueTrackers.emplace(WorldStateIdentifier::Alertness, ValueOverTimeTracker{});
	state.valueTrackers[WorldStateIdentifier::Alertness].durationExceedsExtension =
		[](auto& tracker) {return tracker.durationBelowValue > (tracker.maxValue / 8.f);};

	state.valueTrackers.emplace(WorldStateIdentifier::Curiosity, ValueOverTimeTracker{});
	state.valueTrackers[WorldStateIdentifier::Curiosity].durationExceedsExtension =
		[](auto& tracker) {return tracker.durationBelowValue > (tracker.maxValue / 8.f);};

	state.valueTrackers.emplace(WorldStateIdentifier::PlayerIdentified, ValueOverTimeTracker{});

	state.barker = barker;
}

void HierarchicalTaskNetworkComponent::joinConversation()
{
	executeFinally(planner.plan, state);
	auto& tasks = static_cast<GameMode*>(getWorld()->getAuthGameMode())->getAvailableTasks();

	currentGoal = TaskIdentifier::JoinConversation;

#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
	log("[", owner->getName(), "] ", "New goal: ", tasks.tasks[currentGoal].debugName);
#endif

	updateTaskHistory();		

	createPlan(currentGoal);

	currentGoalName = tasks.tasks[currentGoal].debugName.c_str();
}

void HierarchicalTaskNetworkComponent::updateHUD_PlanPath()
{
	auto& tasks = getWorld()->getAuthGameMode()->getAvailableTasks();

	for (std::size_t i = 0; i < 5u; i++)
	{
		if (i < planner.plan.planPath.size())
		{
			planPath[i] = tasks.tasks[planner.plan.planPath[i]].debugName.c_str(); 
		}
		else
		{
			planPath[i] = "";
		}
	}

	planProgress = (1 + planner.plan.currentPathVertex - begin(planner.plan.planPath)) / 5.f;
}

void HierarchicalTaskNetworkComponent::fixedTick(float dt)
{
	auto playerIdentified = state.current.flags[WorldStateIdentifier::PlayerIdentified];

	sense();
	perceive();
	purgeMemoryOfIgnoredActors();
	produceFacts();

	tickEmotionalState(dt);

	Math::Vector3 eyePosition;
	//FRotator rotation;
	//AIOwner->GetCharacter()->GetActorEyesViewPoint(eyePosition, rotation);	
	state.current.vectors[WorldStateIdentifier::CurrentPosition] = owner->GetActorLocation(); //eyePosition;

	state.current.vectors[WorldStateIdentifier::CurrentPosition_Feet] = owner->GetActorLocation();
	//state.current.vectors[WorldStateIdentifier::CurrentPosition_Feet].z -= Cast<AAICharacter>(AIOwner->GetPawn())->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	auto player = GameplayStatics::GetPlayerCharacter(getWorld(), 0);
	if (player != nullptr)
	{
		//player->GetActorEyesViewPoint(eyePosition, rotation);

		if (state.current.flags[WorldStateIdentifier::PlayerIdentified])
		{
			state.current.vectors[WorldStateIdentifier::PlayerPosition] = eyePosition;
			state.produceFactVector(ConsumableFact::Player_LastKnownLocation, eyePosition);	
		}
	}	

	decide();
	performTask(dt);

	auto gameMode = getWorld()->getAuthGameMode();
	auto& tasks = gameMode->getAvailableTasks();
	
	evaluatePlan(planner.plan, state, worldQuerySystem, planner.parameters, tasks.satisfiablePredicates);		

	if (isBeingDebugViewed)
	{
		updateHUD_PlanPath();	
	}	

	state.valueTrackers[WorldStateIdentifier::PlayerIdentified].update(state.current.flags[WorldStateIdentifier::PlayerIdentified], dt);
	if (!playerIdentified && state.current.flags[WorldStateIdentifier::PlayerIdentified])
	{
		state.produceFactFlag(ConsumableFact::PlayerRecentlyIdentified, true);
	}

	barker.update(dt);
}

void HierarchicalTaskNetworkComponent::tickEmotionalState(float dt)
{
	auto& alertness = state.current.values[WorldStateIdentifier::Alertness];
	alertness = clamp(alertness - 0.09f, 0.f, 100.f);

	auto& boredom = state.current.values[WorldStateIdentifier::Boredom];
	boredom = clamp(boredom + 0.01f, 0.f, 100.f);

	auto& alertnessTracker = state.valueTrackers[WorldStateIdentifier::Alertness];
	auto& curiosityTracker = state.valueTrackers[WorldStateIdentifier::Curiosity];
	
	alertnessTracker.update(alertness, dt);
	curiosityTracker.update(state.current.values[WorldStateIdentifier::Curiosity], dt);
	
	if (alertnessTracker.durationExceedsExtension(alertnessTracker)
		&& curiosityTracker.durationExceedsExtension(curiosityTracker))
	{
		state.produceFactFlag(ConsumableFact::LostLead, true);
	}
	
	return;
}

void HierarchicalTaskNetworkComponent::sense()
{
	state.memory.sensoryMemory.clear();

	incrementEngramAges(state.memory);
	pruneOldEngrams(state.memory);

	senseVisual();
	senseAuditory();
}

void HierarchicalTaskNetworkComponent::senseVisual()
{
	/*std::vector<FOverlapResult> results;
	
	Math::Vector3 eyePosition;
	FRotator rotation;
	auto character = Cast<AAICharacter>(AIOwner->GetCharacter());
	rotation = (character->GetMesh()->GetBoneQuaternion(FName("head")) * FQuat(FRotator(0, 90, 0))).Rotator();//.Rotator().Add(90, 0, 0);
	eyePosition = character->GetMesh()->GetBoneLocation(FName("head"));
	
	auto params = FCollisionQueryParams(FName(TEXT("senseVision")), false);
	params.AddIgnoredActor(character);

	auto world = getWorld();

	auto objectParams = FCollisionObjectQueryParams(ECC_TO_BITFIELD(ECC_Pawn));
	auto hit = world->OverlapMultiByObjectType(results, eyePosition, rotation.Quaternion(), objectParams, FCollisionShape::MakeSphere(motionSightRadius), params);//, FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects));

	auto forward = character->GetActorForwardVector();
	forward = rotation.Vector();

	if (showVisualDebug)
	{
		//detail cone
		DrawDebugCone(world, eyePosition, forward, detailSightRadius / cosf(FMath::DegreesToRadians(detailAngle)), FMath::DegreesToRadians(detailAngle), FMath::DegreesToRadians(detailAngle), 1 / 30.f, FColor::Blue);

		//motion cone
		DrawDebugCone(world, eyePosition, forward, motionSightRadius / cosf(FMath::DegreesToRadians(motionAngle)), FMath::DegreesToRadians(motionAngle), FMath::DegreesToRadians(motionAngle), 1 / 30.f, FColor::Green);
	}

	auto playerWasIdentified = state.current.flags[WorldStateIdentifier::PlayerIdentified]; 
	state.current.flags[WorldStateIdentifier::PlayerIdentified] = false;
		
	if (hit)
	{
		for (auto& result : results)
		{
			Math::Vector3 actorPosition;
			FRotator b;
			result.Actor->GetActorEyesViewPoint(actorPosition, b);
			auto dir = actorPosition - eyePosition;

			float distance = 0.f;
			dir.ToDirectionAndLength(dir, distance);

			auto angle = dot(forward, dir);

			if (angle >= cosf(FMath::DegreesToRadians(motionAngle)))
			{
				auto& alertness = state.current.values[WorldStateIdentifier::Alertness];

				auto player = UGameplayStatics::GetPlayerCharacter(getWorld(), 0);

				auto canIdentify = 
					distance <= (detailSightRadius * alertness / 100.f * 3.f)
					&& (angle >= cosf(FMath::DegreesToRadians(detailAngle)) || playerWasIdentified)
					&& result.Actor == player;
			
				FHitResult traceResult;			
				hit = world->LineTraceSingleByChannel(traceResult, eyePosition, actorPosition, ECollisionChannel::ECC_Pawn, params);			

				if (hit && traceResult.Actor == result.Actor && result.Actor == player)
				{			
					state.current.flags[WorldStateIdentifier::PlayerIdentified] = canIdentify;

					auto vel = traceResult.Actor->GetVelocity();
					vel.normalize();
					state.produceFactVector(ConsumableFact::Player_ForwardVector, vel);

					auto hitLocation = traceResult.Actor->GetActorLocation();
					Stimulus stimulus;
					stimulus.type = StimulusType::Visual;

				
					//TODO: not sure if it should filter based on player or not, chosing not might give some interesting emergent gameplay
					if (traceResult.Actor == player && false)
					{
						stimulus.visual.tag = VisualTag::Player;
					}
					else
					{
						stimulus.visual.tag = VisualTag::Person;
					}
					stimulus.visual.target = result.Actor.Get();
					stimulus.visual.x = hitLocation.x;
					stimulus.visual.y = hitLocation.y;
					stimulus.visual.intensity = angle * FMath::Max(detailSightRadius - distance, 0.f) / detailSightRadius * 100.f;
				
					state.memory.sensoryMemory.push_back(stimulus);

					encodeEngramIfUnique(stimulus, state.memory.shortTermMemory);

					if (canIdentify || alertness > 80.f)
					{
						alertness = 100.f;

					}
					else
					{
						alertness += 0.8f;

						if (alertness > 100.f)
							alertness = 100.f;
					}
				}
			}		
		}
	}*/
}

void HierarchicalTaskNetworkComponent::senseAuditory()
{
	auto& soundMap = getWorld()->getAuthGameMode()->getSoundMap();

	auto location = state.current.vectors[WorldStateIdentifier::CurrentPosition];

	auto range = 10.f;
	auto sounds = soundMap.collectSounds({location.x, location.y}, range);
	
	const float MINIMUM_INTENSITY_HACK = 10.f;

	for (auto& wave : sounds)
	{
		if (wave.intensity < MINIMUM_INTENSITY_HACK)
			continue;

		Stimulus stimulus;
		stimulus.type = StimulusType::Auditory;
		stimulus.intensity = wave.intensity;
		stimulus.x = wave.originalPosition.x;
		stimulus.y = wave.originalPosition.y;
		stimulus.auditory.tag = wave.tag;

		state.memory.sensoryMemory.push_back(stimulus);

		encodeEngramIfUnique(stimulus, state.memory.shortTermMemory);

		//auto& alertness = state.current.values[WorldStateIdentifier::Alertness];
		//alertness = FMath::Clamp(alertness + stimulus.auditory.intensity / 100.f, 0.f, 60.f);
		auto& curiosity = state.current.values[WorldStateIdentifier::Curiosity];
		curiosity = clamp(curiosity + stimulus.intensity / 10.f, 0.f, 100.f);
	}	
}

void HierarchicalTaskNetworkComponent::perceive()
{
	perceiveVisual();
	perceiveAuditory();
}

void HierarchicalTaskNetworkComponent::perceiveVisual()
{
	for (auto& engram : state.memory.shortTermMemory)
	{
		if (engram.type != EngramType::Saw)
			continue;

		if (engram.age > 0)
			continue;

		if (engram.stimulus.visual.tag == VisualTag::Person
			&& state.current.flags[WorldStateIdentifier::PlayerIdentified])		
			continue;

		if (std::find(begin(state.actorsToIgnore), end(state.actorsToIgnore), engram.stimulus.visual.target)
			== end(state.actorsToIgnore))
		{
			addEngramToLocus(engram);
		}		
	}	
}

void HierarchicalTaskNetworkComponent::perceiveAuditory()
{
	for(auto& engram : state.memory.shortTermMemory)
	{
		if (engram.type != EngramType::Heard)
			continue;

		if (engram.age > 0)
			continue;

		addEngramToLocus(engram);
	}
}

void HierarchicalTaskNetworkComponent::addEngramToLocus(Engram& engram)
{
	//group engrams by distance
	/*
	stored as a path, compare distance of each new engram to the endpoint of each focusLocus,
	if close enough then add to the path, if not then create new focus locus
	*/
	const float maxDistance = 50.f * 50.f; //anything within 10 cm is considered part of this locus
	const float minDistance = 2.f * 2.f; //anything closer than 2cm is ignored as a duplicate
	const int maxTimeDelta = 30;

	std::vector<FocusLocus> newLoci;
	std::vector<int> updatedLociIndices;

	if (!state.memory.focusLocus.empty())
	{	
		bool foundLocus = false;
		unsigned int minCount = 0;

		int id = 0;
		for(auto& locus : state.memory.focusLocus)
		{
			float distanceSquared {0.f};
			int timeSince {0};

			if ((engram.type == EngramType::Heard && locus.engrams.back().type == EngramType::Heard)
				|| (engram.type == EngramType::Saw && locus.engrams.back().type == EngramType::Saw))
			{
				auto& stimulus = locus.engrams.back().stimulus;
				distanceSquared = Math::distanceSquared({stimulus.x, stimulus.y, 0.f}, {engram.stimulus.x, engram.stimulus.y, 0.f});
				timeSince = locus.engrams.back().age;
			}				
			else
			{
				id++;
				continue;
			}

			if (distanceSquared > minDistance
				&& distanceSquared <= maxDistance
				&& timeSince < maxTimeDelta)
			{
				locus.engrams.push_back(engram);
				foundLocus = true;

				updatedLociIndices.push_back(id);

				break;
			}
			else if (distanceSquared <= minDistance)
			{
				minCount++;
			}

			id++;
		}	

		if (!foundLocus)
		{
			if (newLoci.empty() && minCount != state.memory.focusLocus.size())
			{
				newLoci.push_back(FocusLocus(engram));
				updatedLociIndices.push_back(state.memory.focusLocus.size());
			}
			else
			{
				int id = 0;
				for (auto& loc : newLoci)
				{
					float distanceSquared {0.f};
					int timeSince {0};

					if (engram.type == EngramType::Heard && loc.engrams.back().type == EngramType::Heard)
					{
						auto& heard = loc.engrams.back().stimulus;
						distanceSquared = Math::distanceSquared({heard.x, heard.y, 0.f}, {engram.stimulus.x, engram.stimulus.y, 0.f});
						timeSince = loc.engrams.back().age;
					}	
					else if (engram.type == EngramType::Saw && loc.engrams.back().type == EngramType::Saw)
					{
						auto& saw = loc.engrams.back().stimulus;
						distanceSquared = Math::distanceSquared({saw.x, saw.y, 0.f}, {engram.stimulus.x, engram.stimulus.y, 0.f});
						timeSince = loc.engrams.back().age;
					}
					else
					{
						id++;
						continue;
					}

					if (distanceSquared > minDistance
						&& distanceSquared <= maxDistance
						&& timeSince < maxTimeDelta)
					{
						loc.engrams.push_back(engram);
						foundLocus = true;
						updatedLociIndices.push_back(state.memory.focusLocus.size() + id);
						break; //should find closest match
					}

					id++;
				}

				if (!foundLocus && minCount != state.memory.focusLocus.size())
				{
					newLoci.push_back(FocusLocus(engram));
					updatedLociIndices.push_back(state.memory.focusLocus.size() + newLoci.size() - 1);
				}
			}
		}
	}
	else
	{
		state.memory.focusLocus.push_back(FocusLocus(engram));
		updatedLociIndices.push_back(0);
	}	

	state.memory.focusLocus.insert(end(state.memory.focusLocus), begin(newLoci), end(newLoci));

	recalculateLoci(updatedLociIndices);
}

/*
New engrams were added to some FocusLocus' which may change them from being area loci to path loci 
*/
void HierarchicalTaskNetworkComponent::recalculateLoci(std::vector<int> updatedLociIndices)
{
	const float maxDistanceToOrigin = 100.f * 100.f;
	
	for(auto id : updatedLociIndices)
	{
		//if all engrams are within a certain radius of the starting one, its an area loci
		//otherwise, its path
		auto& locus = state.memory.focusLocus[id];
		auto& origin = locus.engrams[0];
		unsigned int pastDistanceCount = 0;
		float maxDistance = maxDistanceToOrigin;
		float averageIntensity = 0.f;
		float maxIntensity = 0.f;
		float averageSpeed = 0.f;

		int i = 0;
		for (auto& engram : locus.engrams)
		{
			auto distance = distanceSquared({origin.stimulus.x, origin.stimulus.y, 0.f}, {engram.stimulus.x, engram.stimulus.y, 0.f});

			if (distance > maxDistanceToOrigin)
			{
				pastDistanceCount++;
			}

			maxDistance = fmax(maxDistance, distance);

			maxIntensity = fmax(maxIntensity, engram.stimulus.intensity);
			averageIntensity += engram.stimulus.intensity;

			if (i > 0)
			{
				auto speed = 
					distanceSquared(
						{locus.engrams[i].stimulus.x, locus.engrams[i].stimulus.y, 0.f},
						{locus.engrams[i - 1].stimulus.x, locus.engrams[i - 1].stimulus.y, 0.f})
					/ fmax(locus.engrams[i - 1].age - locus.engrams[i].age, 1);

				averageSpeed += speed;
			}

			i++;
		}

		averageIntensity /= locus.engrams.size();
		averageSpeed /= (locus.engrams.size() - 1);

		if (pastDistanceCount * 2u >= locus.engrams.size())
		{
			locus.type = FocusLocusType::Path;
			locus.currentImportance = averageIntensity + averageSpeed;
		}
		else
		{
			locus.type = FocusLocusType::Area;
			locus.area.radius = 100.f;
			locus.currentImportance = maxIntensity;
		}	
	}
}

/*
Scan our memory to see if there is some fact we can produce. Eg if the character
recently heard a noise somewhere, produce a NoiseDisturbance fact that can be consumed
and acted on.
*/
void HierarchicalTaskNetworkComponent::produceFacts()
{
	if (state.memory.focusLocus.empty())
		return;

	float maxImportance = 0.f;
	int locusId = 0;
	int i = 0;
	for(auto& locus : state.memory.focusLocus)
	{
		if (locus.currentImportance > maxImportance)
		{
			maxImportance = locus.currentImportance;
			locusId = i;
		}
		
		i++;
	}

	ConsumableFact fact;
	if (state.memory.focusLocus[locusId].engrams[0].type == EngramType::Heard)
	{
		fact = ConsumableFact::NoiseDisturbance;
	}
	else if (state.memory.focusLocus[locusId].engrams[0].type == EngramType::Saw)
	{
		fact = ConsumableFact::Glimpse;
	}
	else
	{
		return;
	}

	auto& factValue = state.facts.values[fact];

	if (factValue.consumed)
	{
		auto oldId = roundToInt(factValue.value);

		if (oldId != state.memory.focusLocus[locusId].id)
		{
			state.produceFactValue(fact, state.memory.focusLocus[locusId].id);
		}
	}
	else
	{
		unsigned int oldId = roundToInt(factValue.value);
		auto it = std::find_if(begin(state.memory.focusLocus), end(state.memory.focusLocus), 
			[=](const FocusLocus& f) {return f.id == oldId;});

		if (it != end(state.memory.focusLocus))
		{
			auto importanceRatio = maxImportance / state.memory.focusLocus[oldId].currentImportance;

			if (importanceRatio > 1.5f)
			{
				//50% higher importance -> switch to that once
				factValue.value = state.memory.focusLocus[locusId].id;
			}
		}
		else
		{
			//its been deleted
			factValue.value = state.memory.focusLocus[locusId].id;
		}		
	}	
}

/*
alleviates goldfish syndrome. Originally when AI conversations were implemented,
character Bob would notice Mark, walk up to and greet him, then initiate conversation.
But since Bob's eyes kept noticing Mark, the conversation would cut off and he would
greet him again. "Oh hi Mark, blah blah... Oh hi Mark" etc. So to fix this,
anytime some characters are interacting with eachother for some length of time,
have each one ignore the other so they continue with their original plan.
*/
void HierarchicalTaskNetworkComponent::purgeMemoryOfIgnoredActors()
{
	for (auto ignoredActor : state.actorsToIgnore)
	{
		int stimulusId = 0;
		std::vector<int> ids;
		for (auto& stimulus : state.memory.sensoryMemory)
		{
			if (stimulus.type == StimulusType::Visual)
			{
				if (stimulus.visual.target == ignoredActor)
				{
					ids.push_back(stimulusId);
				}
			}

			stimulusId++;
		}

		std::sort(begin(ids), end(ids), std::greater<int>());

		for(auto id : ids)
		{
			std::swap(state.memory.sensoryMemory[id], state.memory.sensoryMemory.back());
			state.memory.sensoryMemory.pop_back();
		}

		int locusId = 0;
		ids.clear();
		for(auto& locus : state.memory.focusLocus)
		{
			for (auto& engram : locus.engrams)
			{
				if (engram.type == EngramType::Saw)
				{
					if (engram.stimulus.visual.target == ignoredActor)
					{
						ids.push_back(locusId);
						break;
					}
				}
			}

			locusId++;
		}

		std::sort(begin(ids), end(ids), std::greater<int>());

		for(auto id : ids)
		{
			std::swap(state.memory.focusLocus[id], state.memory.focusLocus.back());
			state.memory.focusLocus.pop_back();
		}
	}	
}

int countDistanceSince(std::deque<TaskIdentifier>& history, TaskIdentifier task)
{
	for (int i = history.size() - 1; i >= 0; i--)
	{
		if (history[i] == task)
		{
			return history.size() - i;
		}
	}

	return 10;
}

float evaluateLogistic(float x, FunctionDefinition f)
{
	return f.logistic.L / (1 + exp(-f.logistic.k * (x - f.logistic.x0)));
}

float evaluateQuadratic(float x, FunctionDefinition f)
{
	return f.quadratic.a * x * x + f.quadratic.b * x + f.quadratic.c;
}

float evaluateExponential(float x, FunctionDefinition f)
{
	return pow(f.exponential.a, x);
}

float evaluateGaussian(float x, FunctionDefinition f)
{
	auto b = x - f.gaussian.b;
	auto c = 2 * f.gaussian.c;
	return f.gaussian.a * exp(-(b * b) / (c * c));
}

float evaluateStep(float x, FunctionDefinition f)
{
	return x >= f.step.crossover;
}

float evaluateLinear(float x, FunctionDefinition f)
{
	return x;
}

typedef float (*Evaluator)(float x, FunctionDefinition function);

static Evaluator evaluators[] = 
{
	&evaluateLogistic,
	&evaluateQuadratic,
	&evaluateExponential,
	&evaluateGaussian,
	&evaluateStep,
	&evaluateLinear
};

TaskIdentifier HierarchicalTaskNetworkComponent::evaluateNeeds()
{
	TaskIdentifier goal = TaskIdentifier::Null;
	
	//TODO: should have a fallback mechanism, if the best goal's preconditions are false, try the next best one
	auto& tasks = getWorld()->getAuthGameMode()->getAvailableTasks();
	utilities.clear();

	int i = 0;
	for (auto& taskConsideration : tasks.considerations)
	{
		float scores[5] = {1.f, 1.f, 1.f, 1.f, 1.f};
		float subsetMax = 0.f;
		bool usingSubsetMax[5] = {false, false, false, false, false};

		int j = 0;
		for (auto& consideration : taskConsideration.second)
		{
			float x = 0.f;

			switch(consideration.type)
			{
				case ConsiderationType::Repeat
					:
				{
					x = countDistanceSince(taskHistory, consideration.repeat.identifier) / 10.f;
					break;
				}
				case ConsiderationType::Scalar
					:
				{
					x = state.current.values[consideration.scalar.identifier];
					x /= 100.f;
					break;
				}
				case ConsiderationType::Flag
					:
				{
					x = state.current.flags[consideration.flag.identifier];					
					x = consideration.flag.negate ? !x : x;
					break;
				}
				case ConsiderationType::Distance
					:
				{
					auto from = state.current.vectors[consideration.distance.from];
					auto to = state.current.vectors[consideration.distance.to];

					x = distanceSquared(from, to);
					x /= 1000000.f;
					break;
				}
				case ConsiderationType::ConsumableFlag
					:
				{
					x = !state.facts.flags[consideration.consumableFlag.fact].consumed;
					break;
				}
				case ConsiderationType::ConsumableValue
					:
				{
					x = !state.facts.values[consideration.consumableValue.fact].consumed;
					break;
				}
				case ConsiderationType::ConsumableVector
					:
				{
					x = !state.facts.vectors[consideration.consumableVector.fact].consumed;
					break;
				}
				case ConsiderationType::AuditoryStimulus
					:
				{
					/*for (auto& engram : state.memory.shortTermMemory)
					{
						if (engram.type == EngramType::Heard)
						{
							x = 1.f;
							break;
						}
					}*/

					/*for (auto& locus : state.memory.focusLocus)
					{
						if (!consumedLoci[locus.id])
						{

						}
					}*/

					break;
				}
				case ConsiderationType::Predicate
					:
				{
					x = consideration.predicateFunction(state);
					break;
				}
				case ConsiderationType::ValueOverTimeTracker
					:
				{
					auto& tracker = state.valueTrackers[consideration.valueTracker.identifier];

					switch (consideration.valueTracker.trackerProperty)
					{
						case ValueOverTimeTracker_Property::MaxValue
							:
						{
							x = tracker.maxValue;
							break;
						}
						case ValueOverTimeTracker_Property::DurationBelowValue
							:
						{
							x = tracker.durationBelowValue;
							break;
						}
						case ValueOverTimeTracker_Property::Reacted
							:
						{
							x = tracker.reacted;
							break;
						}
						case ValueOverTimeTracker_Property::DurationExceedsExtension
							:
						{
							if (tracker.durationExceedsExtension)
							{
								x = tracker.durationExceedsExtension(tracker);
							}	
							break;						
						}						
					}

					break;
				}
				case ConsiderationType::LocusImportance
					:
				{
					auto& fact = state.facts.values[consideration.locusImportance.factContainingLocusId];

					if (!fact.consumed)
					{
						//TODO: should divide by 100 for now, have a general scale thing to 0-1 for future						
						auto locus = std::find_if(begin(state.memory.focusLocus), end(state.memory.focusLocus), 
							[id=roundToInt(fact.value)](auto& l){return l.id == id;});

						if (locus != end(state.memory.focusLocus))
						{
							x = locus->currentImportance;
						}
					}

					break;
				}
				case ConsiderationType::LocusAge
					:
				{
					auto& fact = state.facts.values[consideration.locusAge.factContainingLocusId];

					if (!fact.consumed)
					{
						//TODO: should divide by 100 for now, have a general scale thing to 0-1 for future
						auto locus = std::find_if(begin(state.memory.focusLocus), end(state.memory.focusLocus), 
							[id=roundToInt(fact.value)](auto& l){return l.id == id;});

						if (locus != end(state.memory.focusLocus))
						{
							x = static_cast<float>(locus->engrams.back().age) / MaxEngramAge;
						}
					}

					break;
				}
			}

			x = consideration.function.horizontalStretch * (x + consideration.function.horizontalOffset);
			float weight = evaluators[static_cast<int>(consideration.function.type)](x, consideration.function);					
			weight = consideration.function.verticalStretch * weight + consideration.function.verticalOffset;

			usingSubsetMax[consideration.group] = consideration.useSubsetMax;

			scores[consideration.group] *= weight;
	
			j++;
		}

		bool shouldMultiply {false};
		for (int subset = 0; subset < 5; subset++)
		{
			if (usingSubsetMax[subset])
			{
				subsetMax = std::max(subsetMax, scores[subset]);
				shouldMultiply = true;
			}
		}

		float score = scores[0];
		if (shouldMultiply)
		{
			score *= subsetMax;
		}

		utilities.push_back({taskConsideration.first, score});
			
		i++;
	}

	std::sort(begin(utilities), end(utilities), [](const Utility& l, const Utility& r)
	{
		return l.score < r.score;
	});

	if (!utilities.empty())
	{
		goal = utilities.back().task;
	}

	utilityIterator = begin(utilities);

	return goal;
}

void HierarchicalTaskNetworkComponent::createPlan(TaskIdentifier goal)
{
	auto& tasks = getWorld()->getAuthGameMode()->getAvailableTasks();
	planner.plan = generatePlan(tasks.tasks[goal], state, worldQuerySystem, planner.parameters, tasks);

	if (planner.plan.failed)
	{
		fixFailedPlan(planner.plan, state, planner.parameters, tasks);

		if (planner.plan.failed)
		{
			createPlan(TaskIdentifier::Null);
		}
	}

	planner.plan.start();
	updateHUD_PlanPath();
}

void HierarchicalTaskNetworkComponent::updateTaskHistory()
{
	if (taskHistory.size() < 10)
	{
		taskHistory.push_back(currentGoal);
	}
	else
	{
		taskHistory.pop_front();
		taskHistory.push_back(currentGoal);
	}
}

bool isReaction(TaskIdentifier id)
{
	return id == TaskIdentifier::React_Dismiss
		|| id == TaskIdentifier::React_FoundPlayer
		|| id == TaskIdentifier::React_LostPlayer
		|| id == TaskIdentifier::React_Sight
		|| id == TaskIdentifier::React_Sound;
}

void HierarchicalTaskNetworkComponent::decide()
{
	auto& tasks = getWorld()->getAuthGameMode()->getAvailableTasks();

	if (planner.plan.finished || planner.plan.tasks.empty())
	{
		executeFinally(planner.plan, state);

		currentGoal = evaluateNeeds();

#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
		log("[", owner->getName(), "] ", "New goal: ", tasks.tasks[currentGoal].debugName);
#endif

		updateTaskHistory();		

		createPlan(currentGoal);

		currentGoalName = tasks.tasks[currentGoal].debugName.c_str();
	}	
	else if (planner.plan.failed)
	{
		fixFailedPlan(planner.plan, state, planner.parameters, tasks);

		if (planner.plan.failed)
		{
			utilityIterator++;

			if (utilityIterator != end(utilities) && utilityIterator->score > 0.f)
			{
				createPlan(utilityIterator->task);
				currentGoalName = tasks.tasks[utilityIterator->task].debugName.c_str();
			}
			else
			{
				createPlan(TaskIdentifier::Null);
				currentGoalName = tasks.tasks[currentGoal].debugName.c_str();
			}				
		}
	}	
	else
	{
		bool abortGoal {false};
		TaskIdentifier goal {TaskIdentifier::Null};

		if (planner.plan.currentTask->action != Action::PlayMontage
			&& frameCount == 30
			&& !isReaction(currentGoal)
			&& false //TODO: consider removing the once-a-second task switch check with a emotions based check
			) //check once a second			
			//don't look for a better goal if we're currently playing a montage
		{
			//check if something higher priority can be done if we're just waiting
			goal = evaluateNeeds();

			if (goal != currentGoal)
			{
				abortGoal = true;

#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
				log("[", owner->getName(), "] ", "New goal: ", tasks.tasks[goal].debugName);
#endif

			}
		}
		else if (!state.memory.sensoryMemory.empty())
		{
			goal = evaluateNeeds();

			if (isReaction(goal))
			{
				if ((isReaction(currentGoal) && currentGoal != goal)
					|| !isReaction(currentGoal))
				{
#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
					log("[", owner->getName(), "] ", "Aborted goal: ", tasks.tasks[currentGoal].debugName, " for reaction: ", tasks.tasks[goal].debugName);
#endif

					abortGoal = true;
				}
			}
		}

		if (abortGoal)
		{
			executeFinally(planner.plan, state);

			currentGoal = goal;
			
			updateTaskHistory();			

			createPlan(currentGoal);
			/*AIOwner->StopMovement();
			AIOwner->ClearFocus(EAIFocusPriority::Gameplay);
			*/

			currentGoalName = tasks.tasks[currentGoal].debugName.c_str();
		}
	}				
	
	if (!planner.plan.failed && (!planner.plan.finished || currentGoal != TaskIdentifier::Null))
	{
		currentTaskName = std::string(planner.plan.currentTask->debugName.c_str());
	}

	if (planner.plan.finished && currentGoal != TaskIdentifier::Null
		&& planner.plan.tasks[0].identifier != TaskIdentifier::Null
		&& planner.plan.tasks.size() > 1)
	{
		decide();
	}	

	frameCount++;

	if (frameCount > 30)
	{
		frameCount = 0;
	}
}

void HierarchicalTaskNetworkComponent::performTask(float dt)
{
	if (planner.plan.failed || planner.plan.finished)
		return;

	if (planner.plan.currentTask->loop)
	{		
		planner.plan.currentTask->loop(state, worldQuerySystem, *planner.plan.currentTask);
	}

	switch(planner.plan.currentTask->action)
	{
		case Action::SelectDestination
			:
		{
			/*auto navSystem = UNavigationSystem::GetCurrent(getWorld());
			FNavLocation location;

			switch(static_cast<DestinationType>(static_cast<int>(planner.plan.currentTask->parameters.values[0])))
			{
				case DestinationType::RandomLocation
					:
				{
					auto currentPosition = state.current.vectors[WorldStateIdentifier::CurrentPosition];
					navSystem->GetRandomReachablePointInRadius(currentPosition, 1000, location);
					break;
				}
				case DestinationType::RandomStack
					:
				{
					auto& stacks = static_cast<APlayGameState*>(getWorld()->GetGameState())->getStacks();
					auto randomIndex = std::uniform_int_distribution<int32>{0, static_cast<int>(stacks.size()) - 1}(eng);
					
					auto stackPosition = stacks[randomIndex]->GetActorLocation();
					auto forward = stacks[randomIndex]->GetActorForwardVector() * -100.f;
					navSystem->ProjectPointToNavigation(stackPosition + forward, location);
					DrawDebugBox(getWorld(), stackPosition + forward, {100.f, 100.f, 100.f}, FColor::White, true);

					break;
				}
			}			
				
			state.current.vectors[WorldStateIdentifier::Destination] = location.Location;
			*/
			break;
		}
		case Action::MoveToPlayer
			:
		{
			/*auto player = static_cast<AWoodenSphereCharacter*>(getWorld()->GetFirstPlayerController()->GetCharacter());

			auto& currentPosition = state.current.vectors.at(WorldStateIdentifier::CurrentPosition);
			auto& playerPosition = state.current.vectors.at(WorldStateIdentifier::PlayerPosition);

			auto maxDist = 1000000.f;
			auto alpha = FMath::Min(FMath::Max(distanceSquared(currentPosition, playerPosition), MinDistanceToPlayer), maxDist - MinDistanceToPlayer) / (maxDist - MinDistanceToPlayer);
			float speed = 0.f;

			if (alpha >= 1.f)
			{
				speed = 600.f;
			}
			else if (alpha >= 0.75)
			{
				speed = 500.f;
			}
			else if (alpha >= 0.25)
			{
				speed = 300.f;
			}
			else
			{
				speed = 100.f;
			}

			AIOwner->SetFocus(player);
			AIOwner->GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = FMath::InterpEaseInOut(100, 600, alpha, 1.f);
			AIOwner->MoveToLocation(state.current.vectors[WorldStateIdentifier::PlayerPosition], 2);
			*/
			break;
		}
		case Action::MoveToDestination
			:
		{			
			//TODO: should only move once per task, use the same method as should PlayAnimation play (check a float value)
			/*AIOwner->ClearFocus(EAIFocusPriority::Gameplay);
			AIOwner->GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = planner.plan.currentTask->parameters.values[0];
			
			//if (planner.plan.currentTask->parameters.values[1] == 1.f)
			{
				auto& args = planner.plan.currentTask->parameters.vectors[0];
				AIOwner->MoveToLocation(state.current.vectors[WorldStateIdentifier::Destination], args.x, args.y > 0.f, args.z > 0.f);//2);
				planner.plan.currentTask->parameters.values[1] = 0.f;
			}		

			auto colour = FColor::Yellow;

			if (!planner.plan.currentTask->parameters.vectors.empty())
			{
				//colour = FColor(LinearColor(planner.plan.currentTask->parameters.vectors[0]));
				colour = LinearColor(planner.plan.currentTask->parameters.vectors[0]).ToFColor(true); 
			}
			
			if (planner.plan.currentTask->parameters.values[1] == 1.f)
			{
				DrawDebugSphere(getWorld(), state.current.vectors[WorldStateIdentifier::Destination], 50, 16, colour, false, 60);
				planner.plan.currentTask->parameters.values[1] = 0.f;
			}
			*/
			break;
		}
		case Action::MoveToActor
			:
		{			
			//TODO: should only move once per task, use the same method as should PlayAnimation play (check a float value)
			/*AIOwner->ClearFocus(EAIFocusPriority::Gameplay);
			AIOwner->GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = planner.plan.currentTask->parameters.values[0];
			
			if (planner.plan.currentTask->parameters.values[1] == 1.f)
			{
				auto& args = planner.plan.currentTask->parameters.vectors[0];
				
				auto conversationPartner = Cast<AAICharacter>(state.blackboard->GetValueAsObject({*UEnum::GetValueAsString(TEXT("/Script/WoodenSphere.EBlackboardKey"), EBlackboardKey::ConversationPartner)}));

				AIOwner->MoveToActor(conversationPartner, args.x, args.y > 0.f, args.z > 0.f);//2);
				planner.plan.currentTask->parameters.values[1] = 0.f;
			}*/							

			break;
		}
		case Action::StopMoving
			:
		{
			//AIOwner->StopMovement();
			
			break;
		}		
		case Action::Wait
			:
		{
			//planner.plan.currentTask->parameters.values[0] += dt;
			planner.plan.currentTask->parameters.vectors[0].x += dt;
			break;
		}
		case Action::BotherPlayer
			:
		{
			/*auto player = static_cast<AWoodenSphereCharacter*>(getWorld()->GetFirstPlayerController()->GetCharacter());

			if (!player) return;

			player->stress+=25;
			player->TakeDamage(0.1f, {}, GetOwner()->GetInstigatorController(), GetOwner());
			*/
			break;
		}
		case Action::Bark
			:
		{			
			auto bark = static_cast<Bark>(static_cast<int>(planner.plan.currentTask->parameters.values[0]));
			barker.bark(getWorld()->getAuthGameMode()->getBarkString(bark));

			break;
		}
		case Action::PlayMontage
			:
		{
			/*auto animInstance = AIOwner->GetCharacter()->GetMesh()->AnimScriptInstance;
			auto& parameters = planner.plan.currentTask->parameters;

			switch(static_cast<Montage>(static_cast<int>(planner.plan.currentTask->parameters.values[2])))
			{
				case Montage::Stare
					:
				{
					auto stareMontage = static_cast<AAICharacter*>(AIOwner->GetCharacter())->stareMontage;

					if (!parameters.values[1]) //3
					{
						parameters.values[1] = 1; //3
						animInstance->Montage_Play(stareMontage);
					}
					else if(!animInstance->Montage_IsPlaying(stareMontage))
					{
						//parameters.values[0] = 1;	
						parameters.vectors[0].x = 1;
					}		
					else if (state.current.flags[WorldStateIdentifier::PlayerIdentified])
					{
						animInstance->Montage_SetNextSection({"StareLoop"}, {"PlayerIdentified"});
					}
					else if (parameters.values[2] > 5.f) //4
					{
						animInstance->Montage_SetNextSection({"StareLoop"}, {"PlayerNotIdentified"});
					}

					break;
				}
				case Montage::LookAround
					:
				{
					auto lookAroundMontage = static_cast<AAICharacter*>(AIOwner->GetCharacter())->lookAroundMontage;

					if (!parameters.values[1]) //3
					{
						parameters.values[1] = 1;
						animInstance->Montage_Play(lookAroundMontage);
					}
					else if(!animInstance->Montage_IsPlaying(lookAroundMontage))
					{
						//parameters.values[0] = 1;	
						parameters.vectors[0].x = 1;
					}		
					else if (state.current.flags[WorldStateIdentifier::PlayerIdentified])
					{
						animInstance->Montage_SetNextSection({"StareLoop"}, {"PlayerIdentified"});
						animInstance->Montage_JumpToSectionsEnd({"Default"});
					}					

					break;
				}
			}

			parameters.values[2] += dt; //4
*/
			break;
		}
		case Action::PlayAnimation
			:
		{
			auto& parameters = planner.plan.currentTask->parameters;

			/*auto character = Cast<AAICharacter>(AIOwner->GetCharacter());
			character->isSitting = !character->isSitting;//true;*/
			/*auto skeletalMeshComponent = character->FindComponentByClass<USkeletalMeshComponent>();

			if (parameters.values[1] == static_cast<float>(EAnimationPlayerState::NeedsToStart))
			{
				parameters.values[1] = static_cast<float>(EAnimationPlayerState::HasStarted);
				EAnimationId animationId {static_cast<EAnimationId>(roundToInt(parameters.values[0]))};
				auto animation = character->animations.FindByPredicate([=](auto& anim) {return anim.id == animationId;});
				skeletalMeshComponent->PlayAnimation(animation->animation, false);
				//parameters.values[1] = skeletalMeshComponent->GetSingleNodeInstance()->GetLength();				

				auto l = parameters.vectors[0].y = skeletalMeshComponent->GetSingleNodeInstance()->GetLength();	
				if (parameters.values[3] == static_cast<float>(EAnimationType::BlendSpace))
				{
					skeletalMeshComponent->GetSingleNodeInstance()->SetBlendSpaceInput(parameters.vectors[2]);
				}
				parameters.vectors[0].y = skeletalMeshComponent->GetSingleNodeInstance()->GetLength();				
			}

			if (parameters.values[3] == static_cast<float>(EAnimationType::BlendSpace))
			{
				skeletalMeshComponent->GetSingleNodeInstance()->SetBlendSpaceInput(parameters.vectors[2]);
			}		*/			

			parameters.values[2] += dt;
			//parameters.values[0] += dt;
			parameters.vectors[0].x += dt;

			break;
		}
		case Action::FocusPlayer
			:
		{
			/*auto player = static_cast<AWoodenSphereCharacter*>(getWorld()->GetFirstPlayerController()->GetCharacter());

			AIOwner->SetFocus(player);
			*/
			break;
		}
		case Action::LookAt
			:
		{
			/*auto lookAt = planner.plan.currentTask->parameters.vectors[1];
			auto needToCalc = planner.plan.currentTask->parameters.values[0];

			if (needToCalc == 0.f)
			{				
				auto rotation = (GetOwner()->GetActorLocation() - lookAt).Rotation();
				auto rotator = rotation;
				rotator.Pitch = 0;
				rotator.Roll = 0;
				lookAt = rotator.Vector();
			
				planner.plan.currentTask->parameters.values[0] = 1.f;
			}

			auto& interpDt = planner.plan.currentTask->parameters.vectors[0].x;//values[0];
			
			auto v = lookAt - GetOwner()->GetActorLocation();
			v.normalize();
			auto dot = dot(v, GetOwner()->GetActorForwardVector());

			if (FMath::IsNearlyEqual(dot, -1.f))
			{
				interpDt = 1;
			}
			else
			{
				FRotator r;
				r.yaw = 1 * dot > 1 ? 1 : -1;
				r.Roll = 0;
				r.Pitch = 0;
				GetOwner()->AddActorWorldRotation(r);
				AIOwner->GetPawn()->AddActorWorldRotation(r);
			}
			*/
			break;
		}
	}
}

#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
#undef SHOW_PLANNER_INFORMATION_MESSAGES
#endif