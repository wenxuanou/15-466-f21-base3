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
	std::vector< Scene::Transform* > cubes;
	
	//player monkey
	Scene::Transform *player = nullptr;
	glm::quat player_base_rotation;
	bool onGround = true;			// cannot jump when in air
	glm::vec3 move = glm::vec3(0.0f);
	float v_up = 0.0f;				// vertical velocity of player
	
	glm::vec3 get_player_position();

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > player_loop;
	float timeStamp = 0.0f;		//record how much time has pass since play start, in seconds
	float soundLength = 0.0f;	//total length of sound, in seconds
	float totalAvgPower = 0.0f;	//approx totoal average power
	float cubeCD = 1.0f;
	float cubeChangeRecord = 0.0f;
	
	
	//camera:
	Scene::Camera *camera = nullptr;
	
	//light:
	Scene::Light *light = nullptr;
};

