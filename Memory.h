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

/*
Contains types involved with an AI Character's memory

Memory is split into sensory(Stimuli) and short-term (Engrams).
-Sensory memory is filled/cleared each frame
-stimuli are potentially promoted to short term memory if they contain new information

*/

#include <vector>

namespace AI
{
	/*
	Auditory/Visual Tags describe what the source of a stimulus was
	*/
	enum class AuditoryTag
	{
		ObjectDropped, 
		Footsteps
	};

	enum class VisualTag
	{
		Person,
		Player
	};

	enum class StimulusType {Auditory, Visual};

	/*
	A stimulus is some event that was seen or heard by an AI character
	*/
	struct Stimulus
	{
		//location where the stimulus was observed
		float x, y;

		//how significant the stimulus is (how loud a sound was or how close and in focus a sight was)
		float intensity;

		struct Auditory
		{
			AuditoryTag tag;
		};

		struct Visual
		{
			VisualTag tag;
			class Actor* target;
		};

		union
		{
			Auditory auditory;
			Visual visual;			
		};

		StimulusType type;
	};

	enum class EngramType {Heard, Saw};

	//how many frames an engram lasts before being purged.
	//at a fixed 30Hz update rate this is 10 seconds
	const int MaxEngramAge = 300;

	//An engram wraps a stimulus and tracks how old it is, ie
	//how long its been since the stimulus occurred
	struct Engram
	{		
		Stimulus stimulus;
		EngramType type;
		int age;
	};
	
	enum class FocusLocusType
	{
		Area,
		Path
	};

	/*
	A FocusLocus defines a Locus that an AI Character is Focusing on.
	It groups a set of engrams that appear to be related to eachother.
	They can either be all relatively close by to a central point making it an 
	Area, or they can be temporally related and occurred along a path

	TODO: paths
	*/
	struct FocusLocus
	{
		FocusLocus() : type(FocusLocusType::Area) {}
		FocusLocus(Engram first)
		{
			engrams.push_back(first);
			id = nextAvailableId++;
		}

		std::vector<Engram> engrams;

		struct Area
		{
			float radius;
		};

		struct Path
		{				
		};

		union
		{
			Area area;
			Path path;
		};

		FocusLocusType type;
		unsigned int id {0};
		static int nextAvailableId;
		
		
		float currentImportance;
	};

	const int MaxSensoryStimuli = 10;
	struct Memory
	{
		//should they be split by stimulus type?
		/*
		if theres too many auditory stimuli then it makes sense to limit
		visual stimuli you can notice, but that doens't mean you go blind
		*/
		//Stimulus sensoryMemory[MaxSensoryStimuli];
		//Engram shortTermMemory[10];
		std::vector<Stimulus> sensoryMemory;
		std::vector<Engram> shortTermMemory;

		int focus {-1};

		std::vector<FocusLocus> focusLocus;
	};

	/*checks to see if a stimulus is new and if so, stores it in short term memory.
	this is done to provide goldfish syndrome:

	"Oh a castle! I should remember that. Hm some rocks. Oh a castle..."
	*/
	void encodeEngramIfUnique(Stimulus stimulus, std::vector<Engram>& engrams);
	
	void incrementEngramAges(Memory& memory);

	/*
	after a certain amount of time engrams should be forgotten. its not practical to
	remember every single thing that ever happened to a character.
	*/
	void pruneOldEngrams(Memory& memory);	
}
