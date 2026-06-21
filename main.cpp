#include <chrono>
#include <FastNoise/FastNoise.h>
#include "FastNoise/NodeEditorIpc_C.h"
#include <iostream>

#include "render.h"
#include "input.h"
#include "generator.h"
#include "appcontext.h"
#include <windows.h>

#include <glm/gtx/string_cast.hpp>

//auto start = std::chrono::high_resolution_clock::now();
//auto end = std::chrono::high_resolution_clock::now();
//std::chrono::duration<double, std::milli> diff = end - start;
//std::cout << "time: " << diff.count() << "ms\n";


static ImVec2 WorldToScreen(glm::vec3 worldPos, int width, int height, glm::mat4x4 projectionmatrix, glm::mat4x4 viewmatrix){
	glm::vec4 clipPos = projectionmatrix * viewmatrix * glm::vec4(worldPos, 1.0f);

	if(clipPos.w <= 0.0f) return ImVec2(-1, -1); // Hinter der Kamera

	glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

	float screenX = (ndc.x + 1.0f) / 2.0f * width;
	float screenY = (1.0f - ndc.y) / 2.0f * height;

	return ImVec2(screenX, screenY);
}

int main(int, char**){

	bool drawDebugLine = false;
	glm::vec3 debugLineStart;
	glm::vec3 debugLineEnd;

	Render render;
	Inputs input;
	Generator generator;

	generator.ipcContext = fnEditorIpcSetup(true);
	if(!fnEditorIpcStartNodeEditor(nullptr, true, true)){
		DWORD err = GetLastError();
		std::cout << "Editor failed! Windows Error Code: " << err << std::endl;
		LPVOID lpMsgBuf {};
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
		std::cout << "Fehlerdetails: " << (char*)lpMsgBuf << std::endl;
		LocalFree(lpMsgBuf);
	}

	render.onInit();
	input.onInit(render.window);

	//ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	//draw_list->AddLine(ImVec2(10, 10), ImVec2(100, 100), IM_COL32(255, 0, 0, 255));

	AppContext ctx{ &render, &input };
	glfwSetWindowUserPointer(render.window, &ctx);

	generator.lodcount = 4; // 20
	generator.renderdistance = 6; // 10

	generator.IndirectBuffer = render.IndirectBuffer;
	generator.onInit(render.device);
	generator.manager.onInit(&render);

	bool threads_were_working = false;
	bool needs_buffer_update = false;
	bool updatingbuffer = false;
	bool updatecuzedit = false;

	while(render.isrunning()){

		//	auto start = std::chrono::high_resolution_clock::now();
		generator.nodeeditorfunction();
		render.cull = input.cull;


		bool threads_are_working = (generator.threadsworking.load() > 0);
		generator.arethreadsworking = threads_are_working;

		if(!threads_are_working && threads_were_working){

			needs_buffer_update = true;
		}
		threads_were_working = threads_are_working;

		if(input.generate){
			generator.playerchunkpos = glm::ivec3(glm::floor(input.cameraPos / 62.0f));
		}
		if((generator.playerchunkpos != generator.lastplayerchunkpos) && generator.gen && !threads_are_working && !updatingbuffer){
			generator.managechunks();
			generator.lastplayerchunkpos = generator.playerchunkpos;
			//needs_buffer_update = true;
		}
		if(updatecuzedit){
			updatecuzedit = false;
			generator.manageeditchunks();
		}

		if(needs_buffer_update && generator.threadsworking.load() == 0 && !render.ismapped){
			//std::cout << generator.totalfacecount << "\n";
			//ismapped = false;
			updatingbuffer = true;
			render.ismapped = render.mapstagingbuffer();
		}

		if(input.breackvoxels && !threads_are_working && !updatingbuffer){
			
			auto start = std::chrono::high_resolution_clock::now();

			input.breackvoxels = false;

			glm::ivec3 chunkpos = glm::floor(input.cameraPos / 62.0f);

			glm::vec3 dir = input.forward;

			glm::vec3 rayPos = input.cameraPos;
			float stepSize = 0.5f; 
			int maxSteps = 400;   

			for(int i = 0; i < maxSteps; ++i){
				rayPos += dir * stepSize;

				glm::ivec3 worldVoxelPos = glm::floor(rayPos);
				glm::ivec3 currentchunkpos = glm::floor(rayPos / 62.0f);
				int radius = 20;

				Manager::ChunkMap::accessor acc;
				if(generator.manager.chunklist.find(acc, { currentchunkpos.x, currentchunkpos.y, currentchunkpos.z })){
					glm::vec3 localPos = glm::floor(rayPos - (glm::vec3(currentchunkpos) * 62.0f));
					
					int x = static_cast<int>(localPos.x) + 1;
					int y = static_cast<int>(localPos.y) + 1;
					int z = static_cast<int>(localPos.z) + 1;

					if(x >= 1 && x < 63 &&
						y >= 1 && y < 63 &&
						z >= 1 && z < 63){
						//int voxelIndex = z * 64 * 64 + y * 64 + x;

						if((acc->second.solidairx[z * 64 + y] & (1ULL << x)) != 0 || i > 2){
							drawDebugLine = true;

							
							int minx = floor((x - radius) / 62.0);
							int maxx = floor((x + radius) / 62.0);
							int miny = floor((y - radius) / 62.0);
							int maxy = floor((y + radius) / 62.0);
							int minz = floor((z - radius) / 62.0);
							int maxz = floor((z + radius) / 62.0);

							for(int cx = minx; cx <= maxx; cx++){
								for(int cy = miny; cy <= maxy; cy++){
									for(int cz = minz; cz <= maxz; cz++){

										glm::ivec3 neighborchunk = currentchunkpos + glm::ivec3(cx, cy, cz);

										if(neighborchunk != currentchunkpos){
											Manager::ChunkMap::accessor check;
											if(!generator.manager.chunklist.find(check, neighborchunk)){
												check.release();
												continue;
											}
											else{
												check.release();
											}
										}

										bool update = false;
										Generator::editmap::accessor it;

										if(!generator.editlist.find(it, neighborchunk)){
											generator.editlist.insert(it, neighborchunk);
											it->second.materials = std::vector<int8_t>(64 * 64 * 64, -1);
											it->second.solidair = std::vector<int>(64 * 64 * 64, -1);											
										}

										auto circle = [&](std::vector<int8_t>& materials, std::vector<int>& solidair, glm::ivec3 chunkpos) -> bool{
											bool yup = false;

											const glm::ivec3 voxelspace = chunkpos * 62;
											const glm::ivec3 localCenter = worldVoxelPos - voxelspace;// +glm::ivec3(1, 1, 1);
											const int squarerad = radius * radius;

											int startx = std::max(0, localCenter.x - radius);
											int endx = std::min(63, localCenter.x + radius);
											int starty = std::max(0, localCenter.y - radius);
											int endy = std::min(63, localCenter.y + radius);
											int startz = std::max(0, localCenter.z - radius);
											int endz = std::min(63, localCenter.z + radius);

											for(int zz = startz; zz <= endz; zz++){
												int diff_z = (voxelspace.z + zz - 1) - worldVoxelPos.z;
												int squarediffz = diff_z * diff_z;
												int idxz = zz * 64 * 64;

												for(int yy = starty; yy <= endy; yy++){
													int diff_y = (voxelspace.y + yy - 1) - worldVoxelPos.y;
													int squareddiffy = diff_y * diff_y;
													int squareddiffzy = squarediffz + squareddiffy;

													if(squareddiffzy > squarerad) continue;

													int idxyz = idxz + yy * 64;
													int basediff_x = voxelspace.x - 1 - worldVoxelPos.x;

													for(int xx = startx; xx <= endx; xx++){
														int diff_x = basediff_x + xx;
														int squareddiffx = diff_x * diff_x;

														if(squareddiffzy + squareddiffx <= squarerad){
															int idx = idxyz + xx;

															if(materials[idx] != 2){
																materials[idx] = 2;
																yup = true;
															}
															if(solidair[idx] != 1){
																solidair[idx] = 0;
																yup = true;
															}
														}
													}
												}
											}
											return yup;
										};										

										circle(it->second.materials, it->second.solidair, neighborchunk);

										it.release();

									}
								}
							}

							updatecuzedit = true;

							debugLineStart = input.cameraPos + (dir * 0.1f);
							debugLineEnd = rayPos;
							

							acc.release();
							break;
						}
					}
					acc.release();
				} else{
					std::cout << "nochunkfound\n";
				}
			
			}
			/*
			bool hit = false;
			glm::vec3 raypos = input.cameraPos;
			while(!hit){
				raypos += ;
			}*/		
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> diff = end - start;
			//std::cout << "time: " << diff.count() << "ms\n";
		}		

		input.onFrame(render.deltaTime);
		render.matuniforms.viewMatrix = input.uniforms.viewMatrix;
		render.matuniforms.modelMatrix = glm::mat4(1.0f);
		render.uniforms.camerapos = input.cameraPos;

		render.prepareimgui();
		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

		ImVec2 screen_start = WorldToScreen(debugLineStart, render.width, render.height, render.matuniforms.projectionMatrix, render.matuniforms.viewMatrix);
		ImVec2 screen_end = WorldToScreen(debugLineEnd, render.width, render.height, render.matuniforms.projectionMatrix, render.matuniforms.viewMatrix);

		draw_list->AddLine(screen_start, screen_end, IM_COL32(255, 0, 0, 255), 2.0f);
		draw_list->AddCircle(screen_end, 5.0f, IM_COL32(0, 255, 0, 255), 12, 2.0f);

		bool frame = render.prepareFrame();
					

		if(render.ismapped && render.indirectdata && render.vertexdata && render.positiondata){
			generator.manager.manageuploads();

			updatingbuffer = false;
			needs_buffer_update = false;
		}
				
		if(frame){
			//render.computeframe();
			render.beginrenderpass();
			//render.renderPass.MultiDrawIndirect(render.indirectBuffer, 0, render.computeshaderdata.size());
			//render.renderPass.Draw(3, generator.total, 0,0);
			render.renderPass.MultiDrawIndexedIndirect(render.IndirectBuffer, 0, render.chunkcount);
			render.onFrameend();
		}
		render.instance.ProcessEvents();

		//std::cout << "\rculling:" << input.cull << "    generating: " << input.generate << "    posx: " << input.cameraPos.x / 64 << "    posy: " << input.cameraPos.y / 64 << "    posz: " << input.cameraPos.z / 64;
		//std::cout << generator.total << "\n";

		//	auto end = std::chrono::high_resolution_clock::now();
	//std::chrono::duration<double, std::milli> diff = end - start;
	//std::cout << "time: " << diff.count() << "ms\n";
	}

	render.onFinish();

	if(generator.ipcContext){
		fnEditorIpcRelease(generator.ipcContext);
		generator.ipcContext = nullptr;
	}

}
