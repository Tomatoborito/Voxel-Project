#pragma once

struct positions{
	uint32_t x;
	uint32_t y;
	uint32_t z;
	uint32_t lod;
	uint32_t direction;
};

struct shaderdata{
	uint32_t face;
	uint32_t chunkx;
	uint32_t chunky;
	uint32_t chunkz;
	uint32_t widthheight;
	uint32_t normal;
	uint32_t color;
};

struct DrawIndirectArgs{
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstindex;
	uint32_t firstVertex;
	uint32_t firstInstance;
};

struct chunkinformation{
	int indirectoffset[6] = {};
	int lod = 0;
	bool empty = false;
	std::vector<shaderdata> vertexdata[6];
	DrawIndirectArgs indirectdata[6];
	int start = 0;
	int end = 0;
	int directions = 0;
	std::vector<uint64_t> solidairx = std::vector<uint64_t>(64 * 64, 0ULL);
	std::vector<uint64_t> solidairy = std::vector<uint64_t>(64 * 64, 0ULL);
	std::vector<int8_t> materialsx = std::vector<int8_t>(64 * 64 * 64);
	std::vector<int8_t> materialsy = std::vector<int8_t>(64 * 64 * 64);
};

struct chunkupdatedata{
	int indirectoffset[6] = {};
	int lod = 0;
	bool empty = false;
	std::vector<shaderdata> vertexdata[6];
	DrawIndirectArgs indirectdata[6];
	int start = 0;
	int end = 0;
	int directions = 0;
};