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

#include "SoundMap.h"
#include <algorithm>

using namespace Math;

namespace AI
{
	inline bool operator==(const SoundWave& lhs, const SoundWave& rhs)
	{
		return lhs.id == rhs.id;
	}

	inline bool operator<(const SoundWave& lhs, const SoundWave& rhs)
	{
		return lhs.id < rhs.id;
	}

	SoundMap::SoundMap(Vector2<int> gridSize, float resolution)
		: gridSize{gridSize}, resolution{resolution}
	{
		unsigned int size = gridSize.x * gridSize.y;
		
		grid.resize(size);		
	}

	Vector2<int> calcGridCoordinates(Vector2<float> worldCoordinates, float gridResolution, Vector2<int> gridSize)
	{
		Vector2<int> gridCoordinates;
		gridCoordinates.x = clamp(roundToInt(worldCoordinates.x / gridResolution + gridSize.x / 2), 0, gridSize.x);
		gridCoordinates.y = clamp(roundToInt(worldCoordinates.y / gridResolution + gridSize.y / 2), 0, gridSize.y);
		
		return gridCoordinates;		
	}

	void SoundMap::injectSound(Vector2<float> originalPosition, float initialIntensity, AuditoryTag tag)
	{			
		SoundSource source;
		source.gridCoordinates = calcGridCoordinates(originalPosition, resolution, gridSize);
		source.originalPosition = originalPosition;
		source.id = nextSourceId;
		source.initialIntensity = initialIntensity;
		source.tag = tag;

		sources.push_back(source);
		nextSourceId++;
	}

	void clear(std::vector<Cell>& grid)
	{
		std::for_each(begin(grid), end(grid), [](Cell& cell){cell.numberOfWaves = 0;});		
	}
	
	inline void writeCell(Cell& cell, Vector2<int> gridCoordinates, float currentIntensity, const SoundSource& source)
	{
		int waveId = cell.numberOfWaves < Cell::MaxIntersectingWaves ? cell.numberOfWaves : Cell::MaxIntersectingWaves; 
		cell.intersectingWaves[waveId].gridCoordinates = gridCoordinates;
		cell.intersectingWaves[waveId].intensity = currentIntensity;
		cell.intersectingWaves[waveId].id = source.id;
		cell.intersectingWaves[waveId].tag = source.tag;
		cell.intersectingWaves[waveId].originalPosition = source.originalPosition;
		cell.numberOfWaves++;
	}

	void SoundMap::drawWave(SoundSource& source)
	{
		auto& gridCoordinates = source.gridCoordinates;
		int x = source.radius;
		int y = 0;
		int error = 1 - x;
		int maxSize = gridSize.x * gridSize.y;

		float invRadiusSquared = 1.f / (source.radius * source.radius);
		float currentIntensity = source.initialIntensity * invRadiusSquared;

		if (currentIntensity < 1.f)
			return;

		while (x >= y)
		{
			int id = clamp(x + gridCoordinates.x + (y + gridCoordinates.y) * gridSize.x, 0, maxSize);
			writeCell(grid[id], gridCoordinates, currentIntensity, source);

			id = clamp(y + gridCoordinates.x + (x + gridCoordinates.y) * gridSize.x, 0, maxSize);
			writeCell(grid[id], gridCoordinates, currentIntensity, source);

			id = clamp(-x + gridCoordinates.x + (y + gridCoordinates.y) * gridSize.x, 0, maxSize);
			writeCell(grid[id], gridCoordinates, currentIntensity, source);

			id = clamp(-y + gridCoordinates.x + (x + gridCoordinates.y) * gridSize.x, 0, maxSize);
			writeCell(grid[id], gridCoordinates, currentIntensity, source);
						
			id = clamp(-x + gridCoordinates.x + (-y + gridCoordinates.y) * gridSize.x, 0, maxSize);
			writeCell(grid[id], gridCoordinates, currentIntensity, source);

			id = clamp(-y + gridCoordinates.x + (-x + gridCoordinates.y) * gridSize.x, 0, maxSize);
			writeCell(grid[id], gridCoordinates, currentIntensity, source);

			id = clamp(x + gridCoordinates.x + (-y + gridCoordinates.y) * gridSize.x, 0, maxSize);
			writeCell(grid[id], gridCoordinates, currentIntensity, source);

			id = clamp(y + gridCoordinates.x + (-x + gridCoordinates.y) * gridSize.x, 0, maxSize);
			writeCell(grid[id], gridCoordinates, currentIntensity, source);
			
			y++;

			if (error < 0)
			{
				error += 2 * y + 1;
			}
			else
			{
				x--;
				error += 2 * (y - x) + 1;
			}
		}
	}

	void SoundMap::propagate()
	{
		clear(grid);
		
		for (auto& source : sources)
		{
			source.radius++;
			drawWave(source);
		}
	}

	std::vector<SoundWave> SoundMap::collectSounds(Math::Vector2<float> position, int range) const
	{
		std::vector<SoundWave> waves;
		
		auto gridCoordinates = calcGridCoordinates(position, resolution, gridSize);

		auto minX = clamp(gridCoordinates.x - range / 2, 0, gridSize.x);
		auto maxX = clamp(gridCoordinates.x + range / 2, 0, gridSize.x);
		auto minY = clamp(gridCoordinates.y - range / 2, 0, gridSize.y);
		auto maxY = clamp(gridCoordinates.y + range / 2, 0, gridSize.y);

		for (int j = minY; j < maxY; j++)
		{
			for (int i = minX; i < maxX; i++)
			{
				auto index = i + j * gridSize.x;
				auto count = grid[index].numberOfWaves;

				for (int k = 0; k < count; k++)
				{
					waves.push_back(grid[index].intersectingWaves[k]);
				}
			}
		}

		std::sort(begin(waves), end(waves));
		waves.erase(std::unique(begin(waves), end(waves)), end(waves));

		return waves;
	}
}