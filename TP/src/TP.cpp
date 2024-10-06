#include <iostream>

#include <Vroom/Core/Application.h>
#include <Vroom/Core/GameLayer.h>
#include <Vroom/Core/Window.h>

#include "MyScene.h"
#include "TriangulationScene.h"

int main(int argc, char** argv)
{
    vrm::Application app{argc, argv};

	auto& gameLayer = app.getGameLayer();
	
	// Create some triggers so we can interact with the application
	gameLayer.createTrigger("MoveForward")
		.bindInput(vrm::KeyCode::W);
	gameLayer.createTrigger("MoveBackward")
		.bindInput(vrm::KeyCode::S);
	gameLayer.createTrigger("MoveLeft")
		.bindInput(vrm::KeyCode::A);
	gameLayer.createTrigger("MoveRight")
		.bindInput(vrm::KeyCode::D);
	gameLayer.createTrigger("MoveUp")
		.bindInput(vrm::KeyCode::Space);
	gameLayer.createTrigger("MoveDown")
		.bindInput(vrm::KeyCode::LeftShift);
	gameLayer.createTrigger("MouseLeft")
		.bindInput(vrm::MouseCode::Left);
	
	gameLayer.createTrigger("KeyLeft")
		.bindInput(vrm::KeyCode::Left);
	gameLayer.createTrigger("KeyRight")
		.bindInput(vrm::KeyCode::Right);
	gameLayer.createTrigger("KeyUp")
		.bindInput(vrm::KeyCode::Up);
	gameLayer.createTrigger("KeyDown")
		.bindInput(vrm::KeyCode::Down);

	// Create some custom events, which are more general than triggers
	gameLayer.createCustomEvent("MouseRight")
		.bindInput(vrm::Event::Type::MousePressed, vrm::MouseCode::Right);
	gameLayer.createCustomEvent("MouseMoved")
		.bindInput(vrm::Event::Type::MouseMoved);
	gameLayer.createCustomEvent("Exit")
		.bindInput(vrm::Event::Type::KeyPressed, vrm::KeyCode::Escape)
		.bindInput(vrm::Event::Type::Exit)
		.bindCallback([&app](const vrm::Event&) { app.exit(); });
	
	// Load the custom scene
	gameLayer.loadScene<TriangulationScene>();

	// Run the application
	app.run();

	return 0;
}