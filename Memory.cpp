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

#include "Memory.h"
#include <algorithm>
#include <functional>

namespace AI
{
	int FocusLocus::nextAvailableId = 0;

	void encodeEngramIfUnique(Stimulus stimulus, std::vector<Engram>& engrams)
	{		
		if (stimulus.type == StimulusType::Auditory)
		{
			bool heardBefore = false;

			for(auto& engram : engrams)
			{
				if (engram.type == EngramType::Heard)
				{					
					if (engram.stimulus.auditory.tag == stimulus.auditory.tag
						&& (engram.stimulus.x == stimulus.x
							&& engram.stimulus.y == stimulus.y))
					{
						heardBefore = true;						
					}					
				}
			}

			if (!heardBefore)
			{
				Engram engram;
				engram.type = EngramType::Heard;
				engram.age = 0;
				engram.stimulus = stimulus;

				engrams.push_back(engram);
			}
		}
		else 
		{
			bool seenBefore = false;

			for(auto& engram : engrams)
			{
				if (engram.type == EngramType::Saw)
				{
					if (engram.stimulus.visual.tag == stimulus.visual.tag
						&& (engram.stimulus.x == stimulus.x
							&& engram.stimulus.y == stimulus.y)
						&& engram.stimulus.visual.target == stimulus.visual.target)
					{
						seenBefore = true;
					}
				}
			}

			if (!seenBefore)
			{
				Engram engram;
				engram.type = EngramType::Saw;
				engram.age = 0;
				engram.stimulus = stimulus;

				engrams.push_back(engram);
			}
		}		
	}

	void incrementEngramAges(Memory& memory)
	{		
		std::for_each(begin(memory.shortTermMemory), end(memory.shortTermMemory),
			[](Engram& engram){engram.age++;});

		for(auto& locus : memory.focusLocus)
		{	
			std::for_each(begin(locus.engrams), end(locus.engrams),
				[](Engram& engram){engram.age++;});
		}		
	}

	void pruneOldEngramsImpl(std::vector<Engram>& engrams, int& focus, bool maintainOrder)
	{
		//A locus' engrams are sensitive to their order, so use a slower method to handle that case
		if (maintainOrder)
		{
			for (int i = 0; i < engrams.size(); i++)
			{
				if (engrams[i].age > MaxEngramAge)
				{
					engrams.erase(begin(engrams) + i);
					i--;
				}
			}
		}
		else
		{
			/*
			Order doesn't matter, so we can swap soon-to-be-removed engrams with the
			last engram to do a more efficient pop_back
			*/
			std::vector<int> oldIds;
			for(int i = 0; i < engrams.size(); i++)
			{
				if (engrams[i].age > MaxEngramAge
					&& focus != i)
				{
					oldIds.push_back(i);
				}
			}

			std::sort(begin(oldIds), end(oldIds), std::greater<int>());

			auto oldFocus = focus;
			int i = 0;
			for(auto id : oldIds)
			{
				std::swap(engrams[id], engrams.back());
				engrams.pop_back();
			
				if (id < oldFocus)
					focus--;

				i++;
			}
		}
		
	}

	void pruneOldEngrams(Memory& memory)
	{
		pruneOldEngramsImpl(memory.shortTermMemory, memory.focus, false);

		/*
		locii contain their own independent engrams, so they need to be pruned too
		*/
		int f = -1;
		int locusId = 0;
		std::vector<int> oldLocus;
		for (auto& locus : memory.focusLocus)
		{
			pruneOldEngramsImpl(locus.engrams, f, true);

			if (locus.engrams.empty())
			{
				oldLocus.push_back(locusId);
			}

			locusId++;
		}

		/*
		A locus that has no engrams(they expired) is invalid, so remove the empty locii 
		*/
		std::sort(begin(oldLocus), end(oldLocus), std::greater<int>());

		for(auto id : oldLocus)
		{
			std::swap(memory.focusLocus[id], memory.focusLocus.back());
			memory.focusLocus.pop_back();
		}
	}
}