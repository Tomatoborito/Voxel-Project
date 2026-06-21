#include "generator.h"
#include <bit>

void Generator::onInit(Device indevice){

	device = indevice;

	perlin = FastNoise::New<FastNoise::Perlin>();
	perlin->SetScale(50);
	//perlin->SetOutputMax(1.9);
	//perlin->SetOutputMin(0);

	currentgenerator = perlin;

	//chunklist.reserve(renderdistance * renderdistance * renderdistance);

}

template<int axis, bool front> void Generator::findfaces(int dir, int& localtotal, std::vector<uint64_t>* chunkrow, int lod, std::vector<glm::vec3>& normals, const std::vector<int8_t>& materials){
	auto& facedata = threadlocalfacedata.local();
	auto& currentchunkpos = threadlocalcurrentchunkpos.local();
	auto& facerow = threadlocalfacerow.local();

	const uint64_t paddingMask = 0x7FFFFFFFFFFFFFFEULL;
	for(int y = 1; y < 63; y++){
		for(int z = 1; z < 63; z++){

			if constexpr(axis != 2){ // X Y
				if constexpr(!front){
					facerow[z * 64 + y] = (*chunkrow)[z * 64 + y] & ~(*chunkrow)[z * 64 + y + 1];
				}
				else{
					facerow[z * 64 + y] = (*chunkrow)[z * 64 + y] & ~(*chunkrow)[z * 64 + y - 1];
				}
			}
			else{ // Z
				if constexpr(!front){
					facerow[z * 64 + y] = (*chunkrow)[z * 64 + y] & ~(*chunkrow)[(z + 1) * 64 + y];
				}
				else{
					facerow[z * 64 + y] = (*chunkrow)[z * 64 + y] & ~(*chunkrow)[(z - 1) * 64 + y];
				}
			}

		}
	}

	for(int x = 1; x < 63; x++){
		for(int y = 1; y < 63; y++){
			for(int z = 1; z < 63; z++){

				if(((*chunkrow)[z * 64 + y] & (1ULL << x)) != 0){
					for(int xx = -1; xx <= 1; xx++){
						for(int yy = -1; yy <= 1; yy++){
							for(int zz = -1; zz <= 1; zz++){
								if(((*chunkrow)[(z + zz) * 64 + (y + yy)] & (1ULL << (x + xx))) == 0){
									if constexpr(axis == 0){
										normals[x * 64 * 64 + z * 64 + y] += glm::vec3(yy, xx, zz);
									}
									else{
										normals[x * 64 * 64 + z * 64 + y] += glm::vec3(xx, yy, zz);
									}
								}
							}
						}
					}
				}

			}
		}
	}

	for(int y = 1; y < 63; y++){
		for(int z = 1; z < 63; z++){
			facerow[z * 64 + y] &= paddingMask;
		}
	}

	for(int y = 1; y < 63; y++){
		for(int z = 1; z < 63; z++){
			while(facerow[z * 64 + y] != 0){ // checken ob normal gleich

				int real_x;
				int real_y;
				int real_z;
				int widthheight;
				int offset = std::countr_zero(facerow[z * 64 + y]);
				glm::ivec3 currentnormal = normals[offset * 64 * 64 + z * 64 + y];
				glm::ivec3 currentcolor = colormappings[materials[z * 64 * 64 + y * 64 + offset]];
				int width = std::min(63, (int)std::countr_one(facerow[z * 64 + y] >> offset));
				int height = 1;
				int realwidth = 0;

				for(int i = 0; i < width; i++){
					if(currentnormal == glm::ivec3(normals[(offset + i) * 64 * 64 + z * 64 + y])){
						realwidth++;
					}
					else{
						break;
					}
				}

				uint64_t mask = (realwidth == 64) ? ~0ULL : ((1ULL << realwidth) - 1);
				mask = mask << offset;

				facerow[z * 64 + y] = ~mask & facerow[z * 64 + y];

				if constexpr(axis == 0){ // X
					while(z + height < 63){
						bool normalMatch = true;
						for(int i = 0; i < realwidth; i++){
							if(currentnormal != glm::ivec3(normals[(offset + i) * 64 * 64 + (z + height) * 64 + y])){
								normalMatch = false;
								break;
							}
						}

						if(mask == (mask & facerow[(z + height) * 64 + y]) && normalMatch){
							facerow[(z + height) * 64 + y] = ~mask & facerow[(z + height) * 64 + y];
							height++;
						}
						else break;
					}

					real_x = y;
					real_y = offset;
					real_z = z;


					widthheight = (height & 63) | ((realwidth & 63) << 6u);
				}
				else if constexpr(axis == 1){ // Y
					while(z + height < 63){
						bool normalMatch = true;
						for(int i = 0; i < realwidth; i++){
							if(currentnormal != glm::ivec3(normals[(offset + i) * 64 * 64 + (z + height) * 64 + y])){
								normalMatch = false;
								break;
							}
						}

						if(mask == (mask & facerow[(z + height) * 64 + y]) && normalMatch){
							facerow[(z + height) * 64 + y] = ~mask & facerow[(z + height) * 64 + y];
							height++;
						}
						else break;
					}

					real_x = offset;
					real_y = y;
					real_z = z;


					widthheight = (realwidth & 63) | ((height & 63) << 6u);
				}
				else{ // Z
					while(y + height < 63){
						bool normalMatch = true;
						for(int i = 0; i < realwidth; i++){
							if(currentnormal != glm::ivec3(normals[(offset + i) * 64 * 64 + z * 64 + (y + height)])){
								normalMatch = false;
								break;
							}
						}

						if(mask == (mask & facerow[z * 64 + (y + height)]) && normalMatch){
							facerow[z * 64 + (y + height)] = ~mask & facerow[z * 64 + (y + height)];
							height++;
						}
						else break;
					}

					real_x = offset;
					real_y = y;
					real_z = z;

					widthheight = (realwidth & 63) | ((height & 63) << 6u);
				}

				widthheight |= lod << 12;

				int currentfacedata = (real_x & 63u) | ((real_y & 63u) << 6u) | ((real_z & 63u) << 12u);
				currentfacedata = (currentfacedata & ~(7u << 18)) | ((dir & 7u) << 18);
				localtotal++;
				totalfacecount++;
				uint32_t normal = (currentnormal.x & 0x3FF) | ((currentnormal.y & 0x3FF) << 10u) | ((currentnormal.z & 0x3FF) << 20u);

				uint32_t voxelcolor = (uint32_t)currentcolor.r | ((uint32_t)currentcolor.g << 8u) | ((uint32_t)currentcolor.b << 16u);
				facedata.push_back({ (uint32_t)currentfacedata, (uint32_t)currentchunkpos.x, (uint32_t)currentchunkpos.y, (uint32_t)currentchunkpos.z, (uint32_t)widthheight, normal, voxelcolor });

			}
		}
	}

}

bool Generator::generatedata(glm::vec3 pos, int lod, FastNoise::SmartNode<> currentgenerator, eraseinformation& editdata, bool edit){
	//auto& xPositions = threadlocalxPositions.local();
	//auto& yPositions = threadlocalyPositions.local();
	//auto& zPositions = threadlocalzPositions.local();

	/*
	auto& downsampled = threadlocaldownsampled.local();
	downsampled.resize(64 * 64 * 64);

	currentgenerator->GenUniformGrid3D(
		downsampled.data(),
		pos.x * 62, pos.y * 62, pos.z * 62,
		16, 16, 16,
		lod * 4, lod * 4, lod * 4,
		1227
	);

	int cnt = 0;
	for(size_t i = 0; i < downsampled.size(); ++i){
		if(downsampled[i] > 0.0f){
			cnt++;
		}
		else{
			cnt--;
		}

	}

	if(cnt == downsampled.size() || cnt == -downsampled.size()){
		return false;
	}*/

	Manager::ChunkMap::accessor acc;

	if(edit){
		manager.chunklist.find(acc, pos);

		if(editdata.materials.size() == acc->second.materialsx.size()){
			for(int i = 0; i < acc->second.materialsx.size(); i++){
				if(editdata.materials[i] != -1){
					acc->second.materialsx[i] = editdata.materials[i];
				}
			}
		}

		if(editdata.solidair.size() == 64 * 64 * 64){
			for(int i = 0; i < editdata.solidair.size(); i++){
				if(editdata.solidair[i] != -1){
					int x = i % 64;
					int y = (i / 64) % 64;
					int z = i / (64 * 64);

					if(editdata.solidair[i] == 1){
						acc->second.solidairx[z * 64 + y] |= (1ULL << x);
						acc->second.solidairy[z * 64 + x] |= (1ULL << y);
					}
					else{
						acc->second.solidairx[z * 64 + y] &= ~(1ULL << x);
						acc->second.solidairy[z * 64 + x] &= ~(1ULL << y);
					}
				}
			}
		}

	}
	else{
		manager.chunklist.insert(acc, pos);
		acc->second.lod = lod;
		auto& noisedatax = threadlocalnoisedatax.local();
		noisedatax.resize(64 * 64 * 64);

		currentgenerator->GenUniformGrid3D(
			noisedatax.data(),
			pos.x * 62, pos.y * 62, pos.z * 62,
			64, 64, 64,
			lod, lod, lod,
			1227
		);

		/*currentgenerator->GenPositionArray3D(
			noisedatax.data(),
			(int)xPositions.size(),
			xPositions.data(), xPositions.data(), zPositions.data(),
			pos.x * 62, pos.y * 62, pos.z * 62,
			1337
		);*/

		//for(int i = 0; i < noisedatax.size(); i++){
		//	if(editdata[i] != -1){
		//		noisedatax[i] = editdata[i];
		//	}
		//}


		//acc->second.solidairx.resize(noisedatax.size());
		//acc->second.solidairy.resize(noisedatax.size());

		int count = 0;

		//acc->second.materials = noisedatax;

		float* src = noisedatax.data();

		std::fill(acc->second.solidairx.begin(), acc->second.solidairx.end(), 0ULL);
		std::fill(acc->second.solidairy.begin(), acc->second.solidairy.end(), 0ULL);

		for(size_t i = 0; i < noisedatax.size(); ++i){
			if(src[i] < 0.0f){
				int x = i % 64;
				int y = (i / 64) % 64;
				int z = i / (64 * 64);

				// Bits in den jeweiligen 2D-Flächen setzen
				acc->second.solidairx[z * 64 + y] |= (1ULL << x);
				acc->second.solidairy[z * 64 + x] |= (1ULL << y);
				count++;
			}
			else{
				count--;
			}
		
		}

		if(-count == noisedatax.size() || count == noisedatax.size()){
			return false;
		}

	}
	constexpr int DIM = 64;
	constexpr int BLOCK_SIZE = 8;

	//for(int z0 = 0; z0 < DIM; z0 += BLOCK_SIZE){
	//	for(int y0 = 0; y0 < DIM; y0 += BLOCK_SIZE){
	//		for(int x0 = 0; x0 < DIM; x0 += BLOCK_SIZE){

	//			for(int z = z0; z < z0 + BLOCK_SIZE; ++z){
	//				for(int y = y0; y < y0 + BLOCK_SIZE; ++y){
	//					for(int x = x0; x < x0 + BLOCK_SIZE; ++x){
	//						acc->second.solidairy[y + (x * DIM) + (z * DIM * DIM)] = acc->second.solidairx[x + (y * DIM) + (z * DIM * DIM)];
	//					}
	//				}
	//			}
	//		}
	//	}
	//}


	for(int z0 = 0; z0 < DIM; z0 += BLOCK_SIZE){
		for(int y0 = 0; y0 < DIM; y0 += BLOCK_SIZE){
			for(int x0 = 0; x0 < DIM; x0 += BLOCK_SIZE){

				for(int z = z0; z < z0 + BLOCK_SIZE; ++z){
					for(int y = y0; y < y0 + BLOCK_SIZE; ++y){
						for(int x = x0; x < x0 + BLOCK_SIZE; ++x){
							acc->second.materialsy[y + (x * DIM) + (z * DIM * DIM)] = acc->second.materialsx[x + (y * DIM) + (z * DIM * DIM)];
						}
					}
				}
			}
		}
	}

	return true;
}
void Generator::generatemesh(glm::ivec3 chunk, int lod){
	auto& facedata = threadlocalfacedata.local();
	auto& chunkrowx = threadlocalchunkrowx.local();
	auto& chunkrowy = threadlocalchunkrowy.local();
	auto& currentchunkpos = threadlocalcurrentchunkpos.local();
	auto& normalsscratch = threadlocalnormals.local();
	auto& materialsxscratch = threadlocalmaterialsx.local();
	auto& materialsyscratch = threadlocalmaterialsy.local();

	currentchunkpos = chunk;

	int localtotal = 0;
	facedata.clear();

	std::vector<shaderdata> vertexdata[6];
	DrawIndirectArgs indirectdata[6];
	int indirect[6];
	int size[6];

	for(auto& a : chunkrowx){ a = 0; }
	for(auto& a : chunkrowy){ a = 0; }

	// Lock auf den Chunk wird nur so kurz wie möglich gehalten: Daten
	// rauskopieren, dann sofort freigeben - bevor die teure findfaces()
	// Berechnung läuft. Das reduziert die Lock-Contention zwischen
	// parallel laufenden Chunk-Tasks deutlich (siehe Problem 3).
	{
		Manager::ChunkMap::accessor it;
		manager.chunklist.find(it, chunk);

		//simd::ComputechunkSimd(it->second.solidairx.data(), &chunkrowx, 0);
		//simd::ComputechunkSimd(it->second.solidairy.data(), &chunkrowy, 0);

		chunkrowx = it->second.solidairx;
		chunkrowy = it->second.solidairy;

		materialsxscratch = it->second.materialsx;
		materialsyscratch = it->second.materialsy;
	}

	int totalsize = 0;

	auto push = [&](int num){
		size[num] = localtotal * sizeof(shaderdata);
		totalsize += localtotal * sizeof(shaderdata);
		indirect[num] = indirecttotal.fetch_add(1);

		vertexdata[num] = facedata;
		indirectdata[num] = DrawIndirectArgs{ 4, (uint32_t)localtotal, 0, 0, (uint32_t)0 };
		};

	// X
	localtotal = 0;
	facedata.clear();
	std::fill(normalsscratch.begin(), normalsscratch.end(), glm::vec3(0.0f));
	findfaces<0, 1>(0, localtotal, &chunkrowy, lod, normalsscratch, materialsyscratch);
	push(0);

	localtotal = 0;
	facedata.clear();
	std::fill(normalsscratch.begin(), normalsscratch.end(), glm::vec3(0.0f));
	findfaces<0, 0>(1, localtotal, &chunkrowy, lod, normalsscratch, materialsyscratch);
	push(1);

	// Y
	localtotal = 0;
	facedata.clear();
	std::fill(normalsscratch.begin(), normalsscratch.end(), glm::vec3(0.0f));
	findfaces<1, 1>(2, localtotal, &chunkrowx, lod, normalsscratch, materialsxscratch);
	push(2);

	localtotal = 0;
	facedata.clear();
	std::fill(normalsscratch.begin(), normalsscratch.end(), glm::vec3(0.0f));
	findfaces<1, 0>(3, localtotal, &chunkrowx, lod, normalsscratch, materialsxscratch);
	push(3);

	// Z
	localtotal = 0;
	facedata.clear();
	std::fill(normalsscratch.begin(), normalsscratch.end(), glm::vec3(0.0f));
	findfaces<2, 1>(4, localtotal, &chunkrowx, lod, normalsscratch, materialsxscratch);
	push(4);

	localtotal = 0;
	facedata.clear();
	std::fill(normalsscratch.begin(), normalsscratch.end(), glm::vec3(0.0f));
	findfaces<2, 0>(5, localtotal, &chunkrowx, lod, normalsscratch, materialsxscratch);
	push(5);

	Manager::ChunkUpdateMap::accessor acc;
	manager.chunkupdatelist.insert(acc, chunk);

	for(int i = 0; i < 6; ++i){
		acc->second.indirectoffset[i] = indirect[i];
		acc->second.indirectdata[i] = indirectdata[i];
		acc->second.vertexdata[i] = std::move(vertexdata[i]);
	}
	acc->second.lod = lod;
	acc->second.empty = false;
	acc.release();
}
void Generator::managechunks(){

	glm::ivec3 localplayerchunkpos = playerchunkpos;

	auto snap = [](glm::ivec3 pos, int nextlod, bool max){
		glm::vec3 p = glm::vec3(pos) / float(nextlod);

		if(max){
			return glm::ivec3(glm::ceil(p) * float(nextlod));
		}
		return glm::ivec3(glm::floor(p) * float(nextlod));
		};


	tbb::concurrent_vector<glm::ivec3> min(lodcount);
	tbb::concurrent_vector<glm::ivec3> max(lodcount);


	for(int i = 0; i < lodcount; i++){
		int ipow = 1 << i;
		int snapto = (i + 1 < lodcount) ? (1 << (i + 1)) : ipow;
		int lodrenderdistance = renderdistance * ipow;
		min[i] = snap(localplayerchunkpos - glm::ivec3(lodrenderdistance), snapto, false);
		max[i] = snap(localplayerchunkpos + glm::ivec3(lodrenderdistance), snapto, true);
	}



	for(auto it = manager.chunklist.begin(); it != manager.chunklist.end(); ){
		glm::ivec3 pos = glm::ivec3(it->first);
		int lastlod = it->second.lod;

		int expectedlod = -1;

		for(int i = 0; i < lodcount; i++){

			if(pos.x >= min[i].x && pos.x < max[i].x &&
				pos.y >= min[i].y && pos.y < max[i].y &&
				pos.z >= min[i].z && pos.z < max[i].z){

				bool inInnerLod = false;
				if(i > 0){
					if(pos.x >= min[i - 1].x && pos.x < max[i - 1].x &&
						pos.y >= min[i - 1].y && pos.y < max[i - 1].y &&
						pos.z >= min[i - 1].z && pos.z < max[i - 1].z){
						inInnerLod = true;
					}
				}

				if(!inInnerLod){
					expectedlod = (1 << i);
					break;
				}
			}
		}

		if(expectedlod != lastlod){
			auto key_to_erase = it->first;
			++it;
			manager.chunkeraselist.push_back({key_to_erase, false});
			manager.chunkeraseset.insert(key_to_erase);
		}
		else{
			++it;
		}
	}

	for(size_t i = 0; i < lodcount; i++){

		int ipow = 1 << i;

		for(int x = min[i].x; x < max[i].x; x += ipow){
			for(int y = min[i].y; y < max[i].y; y += ipow){
				for(int z = min[i].z; z < max[i].z; z += ipow){

					if(i > 0){
						if(x >= min[i - 1].x && x < max[i - 1].x &&
							y >= min[i - 1].y && y < max[i - 1].y &&
							z >= min[i - 1].z && z < max[i - 1].z){
							continue;
						}

					}

					Manager::ChunkMap::accessor acc;
					
					if(!manager.chunklist.find(acc, { x, y, z }) || manager.chunkeraseset.find({ x,y,z }) != manager.chunkeraseset.end()){
						acc.release();

						threadsworking.fetch_add(1);
						g.run([this, x, y, z, i, ipow]{

							eraseinformation editdta{};
							if(generatedata({ x, y, z }, ipow, currentgenerator, editdta, false)){

								generatemesh({ x, y, z }, ipow);

							}
							else{
								Manager::ChunkUpdateMap::accessor acc;
								manager.chunkupdatelist.insert(acc, { x, y, z });
								acc->second.empty = true;
								acc->second.lod = ipow;
								acc.release();
							}
							threadsworking.fetch_sub(1);

							});

					}


				}

			}

		}
	}
}
void Generator::manageeditchunks(){

	for(auto it = editlist.begin(); it != editlist.end(); ++it){
		glm::ivec3 xyz = it->first;
			
		manager.chunkeraselist.push_back({{ xyz.x,xyz.y,xyz.z}, true });
		manager.chunkeraseset.insert({ xyz.x,xyz.y,xyz.z });
	}

	for(auto it = editlist.begin(); it != editlist.end(); ++it){
		eraseinformation editdta = it->second;

		Manager::ChunkMap::accessor acc;
		manager.chunklist.find(acc, it->first);
		int ipow = acc->second.lod;
		acc.release();

		threadsworking.fetch_add(1);

		glm::ivec3 xyz = it->first;

		g.run([this, xyz, ipow, editdta]{

			eraseinformation localeditdta = editdta;

			if(generatedata({ xyz.x, xyz.y, xyz.z }, ipow, currentgenerator, localeditdta, true)){

				generatemesh({ xyz.x, xyz.y, xyz.z }, ipow);

			}
			else{
				Manager::ChunkUpdateMap::accessor acc;
				manager.chunkupdatelist.insert(acc, { xyz.x, xyz.y, xyz.z });
				acc->second.empty = true;
				acc->second.lod = ipow;
				acc.release();
			}
			threadsworking.fetch_sub(1);
			});

	
	}

	editlist.clear();
}

void Generator::nodeeditorfunction(){
	int msgType = fnEditorIpcPollMessage(ipcContext, receiveBuffer, sizeof(receiveBuffer));

	if(msgType == FASTNOISE_EDITORIPC_MSG_SELECTED_NODE){

		g.wait();

		auto newgenerator = FastNoise::NewFromEncodedNodeTree(receiveBuffer);
		if(newgenerator){
			currentgenerator = newgenerator;
			regenerateworld();
		}
	}
	else if(msgType == FASTNOISE_EDITORIPC_MSG_BUFFER_TOO_SMALL){
		std::cout << ("Received node tree too large for buffer");
	}

}

void Generator::regenerateworld(){

	manager.chunkeraselist.clear();
	manager.chunkeraseset.clear();
	manager.chunkupdatelist.clear();
	manager.chunklist.clear();

	manager.totalchunks = 0;
	indirecttotal = 0;

	manager.vertspacelist.clear();
	manager.vertspacelist.push_back({ 0, 41000000 });

	//for(auto element : manager.chunklist){
		//manager.chunkeraselist.push_back(element.first);
		//manager.chunkeraseset.insert(element.first);
	//}
	managechunks();
	gen = true;
}


glm::ivec3 Generator::mapto3d(int pos){
	int x = pos % 64;
	int	y = (pos / 64) % 64;
	int	z = pos / (64 * 64);
	return glm::ivec3(x, y, z);
}

bool Generator::isinrange(glm::ivec3 current, int x, int y, int z){
	int nx = current.x + x;
	int ny = current.y + y;
	int nz = current.z + z;

	return (nx > 0 && nx < 64 &&
		ny > 0 && ny < 64 &&
		nz > 0 && nz < 64);

}

inline float lerp(float a, float b, float t){
	return a + t * (b - a);
}

void upscaleTrilinear(const std::vector<float>& src, int srcSize, std::vector<float>& dest, int destSize){
	float scale = static_cast<float>(srcSize - 1) / (destSize - 1);

	for(int z = 0; z < destSize; ++z){
		float fz = z * scale;
		int z0 = static_cast<int>(fz);
		int z1 = std::min(z0 + 1, srcSize - 1);
		float weightZ = fz - z0;

		for(int y = 0; y < destSize; ++y){
			float fy = y * scale;
			int y0 = static_cast<int>(fy);
			int y1 = std::min(y0 + 1, srcSize - 1);
			float weightY = fy - y0;

			for(int x = 0; x < destSize; ++x){
				float fx = x * scale;
				int x0 = static_cast<int>(fx);
				int x1 = std::min(x0 + 1, srcSize - 1);
				float weightX = fx - x0;

				// Indizes im 1D-Array berechnen (Z * N*N + Y * N + X)
				auto getIdx = [&](int ix, int iy, int iz){
					return iz * (srcSize * srcSize) + iy * srcSize + ix;
					};

				// Die 8 benachbarten Voxel holen
				float c000 = src[getIdx(x0, y0, z0)];
				float c100 = src[getIdx(x1, y0, z0)];
				float c010 = src[getIdx(x0, y1, z0)];
				float c110 = src[getIdx(x1, y1, z0)];
				float c001 = src[getIdx(x0, y0, z1)];
				float c101 = src[getIdx(x1, y0, z1)];
				float c011 = src[getIdx(x0, y1, z1)];
				float c111 = src[getIdx(x1, y1, z1)];

				// Trilineare Interpolation (7 Lerps)
				float i1 = lerp(c000, c100, weightX);
				float i2 = lerp(c001, c101, weightX);
				float j1 = lerp(c010, c110, weightX);
				float j2 = lerp(c011, c111, weightX);

				float w1 = lerp(i1, j1, weightY);
				float w2 = lerp(i2, j2, weightY);

				float result = lerp(w1, w2, weightZ);

				dest[z * (destSize * destSize) + y * destSize + x] = result;
			}
		}
	}
}