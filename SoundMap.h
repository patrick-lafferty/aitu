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

#include <vector>
#include <map>
#include "Memory.h"
#include "Math.h"

/*
I wanted to try a semi realistic model for sounds:

Sounds are injected into a 2D grid (SoundMap), then each frame they propagate out from their 
initial location in a wave (rectangle) with decreasing intensity. Waves can be absorbed (decreases intensity)
or blocked entirely per-cell, by looking at the cell the wave is expanding into and seeing if
an occluder exists there. So part of the wave can be diminished or blocked but the remaining cells
comprising the wave can continue forward.

AI Characters listen to the world by querying the sound map for waves intersecting a radius around the
character's location. There is a limit to the number of different sound waves stored per cell,
to represent how too many noises would make it too hard to distinguish sounds from eachother.
*/

namespace AI
{
	struct BlockedRange
	{
	};

	struct SoundWave
	{
		Math::Vector2<int> gridCoordinates; //integer coordinates in the grid
		Math::Vector2<float> originalPosition; //actual world position
		int id; //id of SoundSource of this wave
		float intensity; //loudness of wave
		AuditoryTag tag;
	};

	inline bool operator==(const SoundWave& lhs, const SoundWave& rhs);
	inline bool operator<(const SoundWave& lhs, const SoundWave& rhs);

	struct SoundSource
	{
		Math::Vector2<int> gridCoordinates;
		Math::Vector2<float> originalPosition;
		int radius {1};
		int id; //unique id
		float initialIntensity {0.f}; //how loud the source was before it propagated
		std::vector<BlockedRange> blockedRanges;
		AuditoryTag tag;
	};

	struct Cell
	{
		//how much the cell absorbs sounds that pass it
		float damping {1.f};		
		
		//Only track 10 different waves in each cell, the rest are considered white noise
		static const int MaxIntersectingWaves {10};
		SoundWave intersectingWaves[MaxIntersectingWaves + 1];
		int numberOfWaves {0};
	};

	class SoundMap
	{
	public:

		explicit SoundMap(Math::Vector2<int> gridSize, float resolution);
		SoundMap() : SoundMap({100, 100}, 1.f) {}

		void injectSound(Math::Vector2<float> originalPosition, float initialIntensity, AuditoryTag tag);

		/*
		Each call will clear the sound map and draw the sound waves at their current radius
		Older radii will not be stored
		*/
		void propagate();

		/*
		Collects all waves within range cells of position
		*/
		std::vector<SoundWave> collectSounds(Math::Vector2<float> position, int range) const;

		std::vector<Cell>& getGrid() {return grid;}

	private:

		void drawWave(SoundSource& source);
		
		Math::Vector2<int> gridSize;		
		float resolution; //how many world units are in one cell

		std::vector<Cell> grid;
		std::vector<SoundSource> sources;

		int nextSourceId {0};
	};
}