#pragma once

#include <dawn/webgpu_cpp.h>
#define GLM_FORCE_CONSTEXPR
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <FastNoise/FastNoise.h>
#include "FastNoise/NodeEditorIpc_C.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <bit>
#include <bitset>
#include <oneapi/tbb.h>
#include "simd.h"
#include <glm/gtx/component_wise.hpp>
#include "common.h"
#include "manager.h"

#include "appcontext.h"
//#include "computepipeline.h"

using namespace wgpu;

class Generator {

public:
	Buffer IndirectBuffer;



	struct erasedinformation {
		int size;
		int offset;
		int lod;
	};

	struct eraseinformation{
		std::vector<int8_t> materials;
		std::vector<int> solidair;
		
	};

	void onInit(Device device);
	void generatemesh(glm::ivec3 pos, int lod);
	void managechunks();
	glm::ivec3 mapto3d(int pos);
	bool isinrange(glm::ivec3 current, int x, int y, int z);
	void manageeditchunks();
	bool generatedata(glm::vec3 pos, int lod, FastNoise::SmartNode<> currentgenerator, eraseinformation& editdata, bool edit);
	glm::vec3 getcolorfromindex(int index);

	template <int axis, bool front> void findfaces(int dir, int& localtotal, std::vector<uint64_t>* chunkrow, int lod, std::vector<glm::vec3>& normals, const std::vector<int8_t>& materials);

	int total = 0;
	std::atomic<int> indirecttotal = 0;

	std::atomic<int> threadsworking = 0;
	bool arethreadsworking = false;

	FastNoise::SmartNode<FastNoise::Perlin> perlin;
	FastNoise::SmartNode<> currentgenerator;
	int renderdistance;

	std::atomic<int> offset = 0;

	glm::ivec3 playerchunkpos;
	glm::ivec3 lastplayerchunkpos = glm::vec3(20000,0,0);

	std::vector<float> xPositions;
	std::vector<float> zPositions;

	std::vector<float> createInitialX() {
		std::vector<float> v(64 * 64 * 64);
		for (int z = 0; z < 64; z++) {
			for(int y = 0; y < 64; y++) {
				for(int x = 0; x < 64; x++) {
					v[z * 64 * 64 + y * 64 + x] = (float)x;
				}
			}
		}
		return v;
	}

	std::vector<float> createInitialY() {
		std::vector<float> v(64 * 64 * 64);
		for(int z = 0; z < 64; z++) {
			for(int y = 0; y < 64; y++) {
				for(int x = 0; x < 64; x++) {
					v[z * 64 * 64 + y * 64 + x] = (float)y;
				}
			}
		}
		return v;
	}

	std::vector<float> createInitialZ() {
		std::vector<float> v(64 * 64 * 64);
		for(int z = 0; z < 64; z++) {
			for(int y = 0; y < 64; y++) {
				for(int x = 0; x < 64; x++) {
					v[z * 64 * 64 + y * 64 + x] = (float)z;
				}
			}
		}
		return v;
	}

	//tbb::enumerable_thread_specific<int> threadlocalcurrentchunkpos{};
	tbb::enumerable_thread_specific<glm::ivec3> threadlocalcurrentchunkpos{};
	tbb::enumerable_thread_specific<std::vector<uint64_t>> threadlocalchunkrowx{ std::vector<uint64_t>(64 * 64) };
	tbb::enumerable_thread_specific<std::vector<uint64_t>> threadlocalchunkrowy{ std::vector<uint64_t>(64 * 64) };
	tbb::enumerable_thread_specific<std::vector<uint64_t>> threadlocalchunkrowz{ std::vector<uint64_t>(64 * 64) };
	tbb::enumerable_thread_specific<std::vector<uint64_t>> threadlocalfacerow{ std::vector<uint64_t>(64 * 64) };
	tbb::enumerable_thread_specific<std::vector<shaderdata>> threadlocalfacedata{};
	tbb::enumerable_thread_specific<std::vector<float>> threadlocalxPositions{ createInitialX() };
	tbb::enumerable_thread_specific<std::vector<float>> threadlocalzPositions{ createInitialZ() };
	tbb::enumerable_thread_specific<std::vector<float>> threadlocalyPositions{ createInitialY() };
	tbb::enumerable_thread_specific<std::vector<float>> threadlocalnoisedatax{ std::vector<float>(64 * 64 * 64) };
	tbb::enumerable_thread_specific<std::vector<float>> threadlocalnoisedatay{ std::vector<float>(64 * 64 * 64) };
	tbb::enumerable_thread_specific<std::vector<float>> threadlocalnoisedataz{ std::vector<float>(64 * 64 * 64) };
	tbb::enumerable_thread_specific<std::vector<float>> threadlocaldownsampled{ std::vector<float>(64 * 64 * 64) };
	tbb::enumerable_thread_specific<std::vector<glm::vec3>> threadlocalnormals{ std::vector<glm::vec3>(64 * 64 * 64) };
	tbb::enumerable_thread_specific<std::vector<int8_t>> threadlocalmaterialsx{ std::vector<int8_t>(64 * 64 * 64) };
	tbb::enumerable_thread_specific<std::vector<int8_t>> threadlocalmaterialsy{ std::vector<int8_t>(64 * 64 * 64) };
	//std::vector<chunkinformation> chunkslist;

	tbb::task_group g;
	bool gen = false;

	Manager manager;
	
	typedef tbb::concurrent_hash_map<glm::ivec3, eraseinformation> editmap;
	editmap editlist;

	int lodcount;

	std::atomic<int> totalfacecount;


	Device device;

	void* ipcContext;
	char receiveBuffer[64 * 1024];

	void nodeeditorfunction();
	void regenerateworld();

	static constexpr glm::vec3 colormappings[] = {
		{255  , 0, 0},
		{0  , 255, 0},
		{255, 255, 0},
		{255, 255, 0},
		{255, 255, 0},
		{255, 255, 0},
		{255, 255, 0},
		{255, 255, 0},
		{255, 255, 0},
		{255, 255, 0},
		{255, 255, 0},
	};


};

