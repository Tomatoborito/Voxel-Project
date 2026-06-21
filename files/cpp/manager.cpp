#include "manager.h"


void Manager::manageuploads(){
	
	deletechunks(false);

	if(vertspacelist.size() > 1){
		std::sort(vertspacelist.begin(), vertspacelist.end(),
			[](const startnend& a, const startnend& b){
				return a.start < b.start;
			});

		for(size_t i = 0; i + 1 < vertspacelist.size(); ){
			if(vertspacelist[i].end == vertspacelist[i + 1].start){
				vertspacelist[i].end = vertspacelist[i + 1].end;
				vertspacelist.erase(vertspacelist.begin() + i + 1);
			}
			else{
				++i;
			}
		}
	}

	for(auto it = chunkupdatelist.begin(); it != chunkupdatelist.end(); ++it){
		int directions = 0;
		int totalverts = 0;

		if(!it->second.empty){

			for(int i = 0; i < 6; i++){
				totalverts += it->second.vertexdata[i].size();
				if(!it->second.vertexdata[i].empty()){
					directions++;
				}
			}

			if(totalverts == 0){
			//	chunkeraselist.clear();
			//	chunkeraseset.clear();
				continue;
			}

			bool foundspace = false;
			int allocated_vert_start = 0;

			for(size_t m = 0; m < vertspacelist.size(); m++){
				auto& element = vertspacelist[m];

				if(!foundspace && (element.end - element.start) >= totalverts){

					int currentvertex = 0;
					allocated_vert_start = element.start;

					for(int i = 0; i < 6; i++){
						if(!it->second.vertexdata[i].empty()){

							// Vertex Buffer befüllen
							for(shaderdata elmnt : it->second.vertexdata[i]){
								render->vertexdata[element.start + currentvertex++] = elmnt;
							}

						}
					}

					// Chunk-Eigenschaften updaten
					it->second.start = element.start;
					it->second.end = element.start + totalverts;
					it->second.directions = directions;

					element.start += totalverts;
					foundspace = true;
					break;
				}
			}

			if(!foundspace){
				continue;
			}

			int current_vert_offset = 0;

			for(int i = 0; i < 6; i++){

				if(!it->second.vertexdata[i].empty()){

					int element = totalchunks++;

					// Indirect Buffer befüllen
					it->second.indirectoffset[i] = element;
					render->indirectdata[element] = it->second.indirectdata[i];
					render->indirectdata[element].firstInstance = allocated_vert_start + current_vert_offset;
					indirectpositions[element] = it->first;

					// Position Buffer befüllen
					render->positiondata[element] = {
						(uint32_t)it->second.vertexdata[i][0].chunkx,
						(uint32_t)it->second.vertexdata[i][0].chunky,
						(uint32_t)it->second.vertexdata[i][0].chunkz,
						(uint32_t)it->second.lod,
						(uint32_t)i
					};
					it->second.indirectoffset[i] = element;
					current_vert_offset += it->second.vertexdata[i].size();
				}
			}
		}

		ChunkMap::accessor acc;
		chunklist.insert(acc, it->first);
		acc->second.empty = it->second.empty;
		acc->second.lod = it->second.lod;

		if(!it->second.empty){
			acc->second.start = it->second.start;
			acc->second.end = it->second.end;
			acc->second.directions = it->second.directions;

			for(int i = 0; i < 6; i++){
				acc->second.indirectoffset[i] = it->second.indirectoffset[i];
				acc->second.indirectdata[i] = it->second.indirectdata[i];
				acc->second.vertexdata[i] = std::move(it->second.vertexdata[i]);
			}
		}

		acc.release();
	}
	chunkupdatelist.clear();

	std::erase_if(vertspacelist, [](startnend element){ return element.start >= element.end; });

	render->chunkcount = totalchunks;

	render->unmapandcopy();
}

void Manager::deletechunks(bool edit){
	
	for(auto chunk : chunkeraselist){
		ChunkMap::accessor it;
		if(chunklist.find(it, chunk.pos)){

			for(int d = 0; d < 6; d++){
				if(!it->second.vertexdata[d].empty()){
					totalchunks--;
					int indirect_idx = it->second.indirectoffset[d];
					if(indirect_idx != totalchunks){

						render->indirectdata[indirect_idx] = render->indirectdata[totalchunks];
						render->positiondata[indirect_idx] = render->positiondata[totalchunks];
						indirectpositions[indirect_idx] = indirectpositions[totalchunks];

						it.release();

						ChunkMap::accessor acc;
						auto key = indirectpositions[indirect_idx];
						if(chunklist.find(acc, key)){

							for(int dir = 0; dir < 6; dir++){
								if(!acc->second.vertexdata[dir].empty() && acc->second.indirectoffset[dir] == totalchunks){
									acc->second.indirectoffset[dir] = indirect_idx;
									break;
								}
							}
						}

						acc.release();

						chunklist.find(it, chunk.pos);
					}
					render->indirectdata[totalchunks] = { 0,0,0,0,0 };
					render->positiondata[totalchunks] = { 0,0,0,0,0 };
					indirectpositions[totalchunks] = { 0,0,0 };
				}
			}

			vertspacelist.push_back({ it->second.start, it->second.end });

			if(!chunk.edit){
				chunklist.erase(it);
			}
			else{
				it.release();
			}

		}
	}

	chunkeraselist.clear();
	chunkeraseset.clear();
}

void Manager::onInit(Render* renderer){
	render = renderer;
	vertspacelist.push_back({ 0,41000000 });
	indirectpositions.resize(1000000);

	vertexlist.resize(41000000);
	indirectlist.resize(1000000);
	positionlist.resize(1000000);

}