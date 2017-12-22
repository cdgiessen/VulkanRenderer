#include "TerrainGenerator.h"

#include <iostream>

TerrainGenerator::TerrainGenerator(int numCells, int splatMapSize, glm::vec3 pos, glm::vec3 size)
{
	double freq = 0.001;
	double pers = 0.4;
	int octaves = 5;
	
	//rainFallMap.SetFrequency(freq);
	//rainFallMap.SetPersistence(pers);
	//rainFallMap.SetOctaveCount(1);

	//temperatureMap.SetFrequency(freq*0.5);
	//temperatureMap.SetPersistence(pers);
	//temperatureMap.SetOctaveCount(octaves);

	//elevationMap.SetFrequency(freq);
	//elevationMap.SetPersistence(pers);
	//elevationMap.SetOctaveCount(octaves);

	//biomeSelector;


	//module::RidgedMulti mountainTerrain;
	//mountainTerrain.SetFrequency(freq * 0.25);
	//mountainTerrain.SetOctaveCount(octaves);
	
	//module::Perlin hillTerrain
	//hillTerrain.SetFrequency(freq * 1.15);
	//hillTerrain.SetOctaveCount(octaves);

	//module::Billow baseFlatTerrain;
	//baseFlatTerrain.SetFrequency(freq * 0.8);
	//baseFlatTerrain.SetOctaveCount(octaves);
	
	//module::ScaleBias flatTerrain;
	//flatTerrainScaleBias.SetSourceModule(0, baseFlatTerrain);
	//flatTerrainScaleBias.SetScale(0.1);
	//flatTerrainScaleBias.SetBias(0.1);

	//module::ScaleBias hillTerrainScaleBias;
	//hillTerrainScaleBias.SetSourceModule(0, hillTerrain);
	//hillTerrainScaleBias.SetScale(0.2);
	//hillTerrainScaleBias.SetBias(0);

	//module::Perlin terrainType;
	//hillMountainType.SetFrequency(freq * 0.1);
	//hillMountainType.SetPersistence(pers);
	//hillMountainType.SetOctaveCount(octaves);

	//module::Perlin flatHillMountainType
	//flatHillMountainType.SetFrequency(freq * 0.2);
	//flatHillMountainType.SetPersistence(pers);
	//flatHillMountainType.SetOctaveCount(octaves);
	//
	////terrain selector
	//mountainHillSelector.SetSourceModule(0, hillTerrainScaleBias);
	//mountainHillSelector.SetSourceModule(1, mountainTerrain);
	//mountainHillSelector.SetControlModule(hillMountainType);
	//mountainHillSelector.SetBounds(0, 1000);
	//mountainHillSelector.SetEdgeFalloff(0.125);
	//
	//hillMountainFlatSelector.SetSourceModule(0, mountainHillSelector);
	//hillMountainFlatSelector.SetSourceModule(1, flatTerrainScaleBias);
	//hillMountainFlatSelector.SetControlModule(flatHillMountainType);
	//hillMountainFlatSelector.SetBounds(-0.05, 1000);
	//hillMountainFlatSelector.SetEdgeFalloff(0.125);

	//turbulence
	//finalTerrain.SetSourceModule(0, hillMountainFlatSelector);
	//finalTerrain.SetFrequency(8.0);
	//finalTerrain.SetPower(0.125);
	//
	//heightMapBuilder.SetSourceModule(finalTerrain);
	//heightMapBuilder.SetDestNoiseMap(heightMap);
	//heightMapBuilder.SetDestSize(splatMapSize, splatMapSize);
	//heightMapBuilder.SetBounds(pos.x, pos.x + size.x + size.x / (float)splatMapSize, pos.z, pos.z + size.z + size.z / (float)splatMapSize);
	//heightMapBuilder.Build();

	/*
	"DeadSpruceTreeTrunk.png",
	"Sand.png",
	"SpruceTreeLeaf.png",
	"Rock.png",
	"Snow.png",
	*/
	
	//renderer.SetSourceNoiseMap(heightMap);
	//renderer.SetDestImage(image);
	//renderer.ClearGradient();
	//
	//renderer.AddGradientPoint(-1.000, utils::Color(255, 0, 0, 32));
	//renderer.AddGradientPoint(0.0000, utils::Color(255, 0, 0, 255));
	//renderer.AddGradientPoint(0.1000, utils::Color(0, 255, 0, 255));
	//renderer.AddGradientPoint(0.4000, utils::Color(0, 255, 0, 255));
	//renderer.AddGradientPoint(0.5000, utils::Color(255, 0, 0, 255));
	//renderer.AddGradientPoint(0.6000, utils::Color(0, 0, 255, 255));
	//renderer.AddGradientPoint(0.8000, utils::Color(0, 0, 255, 255));
	//renderer.AddGradientPoint(1.0000, utils::Color(0, 0, 0, 255));
	//
	//renderer.Render();

	//nodeGraph = new NewNodeGraph::TerGenNodeGraph(1337, 100, glm::vec3(0,0,0), 1.0f);
	//
	//nodeGraph->BuildNoiseGraph();

}


TerrainGenerator::~TerrainGenerator()
{
}

//utils::Image* TerrainGenerator::getImagePtr() {
//	return &image;
//}

float TerrainGenerator::SampleHeight(float x, float y, float z) {
	//return nodeGraph->SampleHeight(x, y, z);
	//return hillTerrain.GetValue(x, y, z);
	//return finalTerrain.GetValue(x, y, z);
	return 0.0f;
}

float TerrainGenerator::GetBiomeColor(int channel, float rainVal, float tempVal, float elevation) {
	int moisture = (int)(rainVal * 6);
	int temperature = (int)(tempVal * 4);

	if (elevation < 0.01) {
		return seaFloot.GetColorChannel(channel);
	}
	else if (elevation > 0.95) {
		snowCap.GetColorChannel(channel);
	}

	return temp_rain_chart[temperature][moisture].GetColorChannel(channel);
}

float TerrainGenerator::SampleColor(int channel, float x, float y, float z) {
	return 1.0f;
	//return GetBiomeColor(channel, (rainFallMap.GetValue(x, y, z) + 1.0) / 2.0, (temperatureMap.GetValue(x, y, z) + 1.0) / 2.0, SampleHeight(x,y,z));
}





FastTerrainGenerator::FastTerrainGenerator(int splatMapSize, int numCells, glm::vec3 pos, glm::vec3 size)
{
	nodeGraph = new NewNodeGraph::TerGenNodeGraph(1337, 100, glm::i32vec2(0,0), 1.0f);

	nodeGraph->BuildNoiseGraph();
}


FastTerrainGenerator::~FastTerrainGenerator()
{
	FastNoiseSIMD::FreeNoiseSet(noiseSet);
}

