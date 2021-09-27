//
//  MonkeyMode.hpp
//  
//
//  Created by owen ou on 2021/9/25.
//

#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct MonkeyMode : Mode {
	MonkeyMode();
	virtual ~MonkeyMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, jump;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;
	
	//cube info
	std::vector< Scene::Transform* > cubes = {nullptr};
	
	//player monkey
	Scene::Transform *player = nullptr;
	glm::quat player_base_rotation;
	bool canJump = true;			// cannot jump when in air
	
	glm::vec3 get_player_position();

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > player_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

};

