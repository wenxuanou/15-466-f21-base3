//
//  MonkeyMode.cpp
//  
//
//  Created by owen ou on 2021/9/25.
//

#include "MonkeyMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>


#include <random>

GLuint playground_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > playground_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("playground.pnct"));
	playground_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > playground_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("playground.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = playground_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = playground_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));			//TODO: change music
});

MonkeyMode::MonkeyMode() : scene(*playground_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		
		if(transform.name == "Player") player = &transform;
		
		if(transform.name.find("Cube") != std::string::npos) {
			cubes.push_back(&transform);
		}
		
	}
	
	if (player == nullptr) throw std::runtime_error("Player not found.");
	
	player_base_rotation = player->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	player_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_player_position(), 10.0f);
}

MonkeyMode::~MonkeyMode() {
}

bool MonkeyMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			// press space
			jump.downs += 1;
			jump.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			// space release
			jump.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		// mouse rotate player
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
						
			return true;
		}
	}

	return false;
}

void MonkeyMode::update(float elapsed) {
	
	player_loop->set_position(get_player_position(), 1.0f/60.0f);
	
	//move player:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		move = glm::vec3(0.0f);
		float degree = 0.0f;
		if (left.pressed && !right.pressed) degree += 5.0f;
		if (!left.pressed && right.pressed) degree -= 5.0f;
		if (down.pressed && !up.pressed) move.y = 1.0f;
		if (!down.pressed && up.pressed) move.y = -1.0f;
		if(jump.pressed) {
//			canJump = false;
			v_up = 7.0f;
//			move.z = 1.0f;
		}

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec3(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;
		
		player->rotation = player_base_rotation * glm::angleAxis(glm::radians(degree), glm::vec3(0.0f, 0.0f, 1.0f));
		player_base_rotation = player->rotation;
				
	}
	
	{
		//collision
		//TODO: need better way iterate through instances
		for(int i = 0; i < cubes.size(); i++){
			glm::vec3 min = glm::max(cubes[i]->position - cubes[i]->scale, player->position - player->scale);
			glm::vec3 max = glm::min(cubes[i]->position + cubes[i]->scale, player->position + player->scale);

			
			glm::vec3 cubes_max = cubes[i]->position + cubes[i]->scale;
			glm::vec3 player_min = player->position - player->scale;
			
			// if not above this cube
			if(min.x > max.x || min.y > max.y){
				continue;
			}
			
			if(cubes_max.z < player_min.z){
				//above ground
				move.z = v_up * elapsed - 0.5 * 9.8 * elapsed * elapsed;
				v_up -= 9.8 * elapsed;
			}else{
				// hit bottom, bounce up
				v_up = std::abs(v_up) * 0.8f;
//				v_up = (std::abs(v_up) * 0.5f < 0.01f) ? 0.0f : std::abs(v_up) * 0.5f;
				move.z = v_up * elapsed - 0.5 * 9.8 * elapsed * elapsed;
//				move.z = (std::abs(move.z) < 0.01f) ? 0.0f : move.z;
//				canJump = true;
			}
		}
		 
	}
	
	{
		// take move
		glm::mat4x3 frame = player->make_local_to_world();
	//		glm::vec3 right = frame[0];
		glm::vec3 forward = frame[1];
		glm::vec3 up = frame[2];
		player->position += move.y * forward + move.z * up;
//		std::cout << v_up << std::endl;
	}
	
	
	
	
	{ //update listener to camera position:
		
		glm::mat4x3 frame = player->make_local_to_parent();
		glm::vec3 right = frame[0];
		glm::vec3 at = frame[3];
		Sound::listener.set_position_right(at, right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	jump.downs = 0;
}

void MonkeyMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

glm::vec3 MonkeyMode::get_player_position(){
	return player->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);	//TODO: check what this number is
}
