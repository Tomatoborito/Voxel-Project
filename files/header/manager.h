#pragma once

#include <dawn/webgpu_cpp.h>
#define GLM_FORCE_CONSTEXPR
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <oneapi/tbb.h>
#include <unordered_map>
#include <unordered_set>

#include "render.h"

#include "common.h"
#include "appcontext.h"


using namespace wgpu;

class Manager {

	struct startnend{
		int start;
		int end;
	};

	struct eraseliststruct {
		glm::ivec3 pos;
		bool edit;
	};

	public:
		void manageuploads();
		void deletechunks(bool edit);
		void onInit(Render* renderer);

		typedef tbb::concurrent_hash_map<glm::ivec3, chunkinformation> ChunkMap;
		ChunkMap chunklist;

		typedef tbb::concurrent_hash_map<glm::ivec3, chunkupdatedata> ChunkUpdateMap;
		ChunkUpdateMap chunkupdatelist;

		std::vector<eraseliststruct> chunkeraselist;
		std::unordered_set<glm::ivec3> chunkeraseset;

		int totalchunks = 0;

		std::vector<shaderdata> vertexlist;
		std::vector<DrawIndirectArgs> indirectlist;
		std::vector<positions> positionlist;
		std::vector<startnend> vertspacelist;
		std::vector<int> indirectspacelist;
		std::vector<glm::ivec3> indirectpositions;

		Render* render = nullptr;
};