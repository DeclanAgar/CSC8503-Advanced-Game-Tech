#include "Window.h"

#include "Debug.h"

#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"

#include "GameServer.h"
#include "GameClient.h"

#include "NavigationGrid.h"
#include "NavigationMesh.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"
#include "PushdownMachine.h"

#include "PushdownState.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"

using namespace NCL;
using namespace CSC8503;

#include <chrono>
#include <thread>
#include <sstream>

vector<Vector3> testNodes;
void TestPathfinding() {
	NavigationGrid grid("TestGrid1.txt");

	NavigationPath outPath;

	Vector3 startPos(10, 0, 10);
	Vector3 endPos(70, 0, 80);

	bool found = grid.FindPath(startPos, endPos, outPath);
	if (!found) {
		std::cout << "Path not found\n";
	}
	else {
		std::cout << "Path found\n";
	}

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes.push_back(pos);
	}
}

void DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];

		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}
}

void TestStateMachine() {
	StateMachine* testMachine = new StateMachine();
	int data = 0;

	State* A = new State([&](float dt)->void {
		std::cout << "I'm in state A!\n";
		data++;
		});

	State* B = new State([&](float dt)->void {
		std::cout << "I'm in state B!\n";
		data--;
		});

	StateTransition* stateAB = new StateTransition(A, B, [&](void)->bool {
		return data > 10;
		});

	StateTransition* stateBA = new StateTransition(B, A, [&](void)->bool {
		return data < 0;
		});

	testMachine->AddState(A);
	testMachine->AddState(B);
	testMachine->AddTransition(stateAB);
	testMachine->AddTransition(stateBA);

	for (int i = 0; i < 100; ++i) {
		testMachine->Update(1.0f);
	}

}

void TestBehaviourTree() {
	float behaviourTimer;
	float distanceToTarget;
	BehaviourAction* findKey = new BehaviourAction("Find Key", [&](float dt, BehaviourState state)->BehaviourState{
		if (state == Initialise) {
			std::cout << "Looking for a key!\n";
			behaviourTimer = rand() % 100;
			state = Ongoing;
		}
		else if (state == Ongoing) {
			behaviourTimer -= dt;
			if (behaviourTimer <= 0.0f) {
				std::cout << "Found a key!\n";
				return Success;
			}
		}
	return state;
	});

	BehaviourAction* goToRoom = new BehaviourAction("Go To Room", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Going to the loot room!\n";
		}
		else if (state == Ongoing) {
			distanceToTarget -= dt;
			if (distanceToTarget <= 0.0f) {
				std::cout << "Reached room!\n";
				return Success;
			}
		}
		return state;
	});

	BehaviourAction* openDoor = new BehaviourAction("Open Door", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Opening Door!\n";
			return Success;
			}
		return state;
		});

	BehaviourAction* lookForTreasure = new BehaviourAction("Look for Treasure", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for treasure!\n";
			return Ongoing;
		}
		else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "Found some treasure!\n";
				return Success;
			}
			std::cout << "No treasure in here...\n";
			return Failure;
		}
		return state;
		});

	BehaviourAction* lookForItems = new BehaviourAction("Look for Items", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for items!\n";
			return Ongoing;
		}
		else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "I found some items!\n";
				return Success;
			}
			std::cout << "No items in here...\n";
			return Failure;
		}
		return state;
	});

	BehaviourSequence* sequence = new BehaviourSequence("Room Sequence");
	sequence->AddChild(findKey);
	sequence->AddChild(goToRoom);
	sequence->AddChild(openDoor);

	BehaviourSelector* selection = new BehaviourSelector("Loot Selection");
	selection->AddChild(lookForTreasure);
	selection->AddChild(lookForItems);

	BehaviourSequence* rootSequence = new BehaviourSequence("Root Sequence");
	rootSequence->AddChild(sequence);
	rootSequence->AddChild(selection);

	for (int i = 0; i < 5; ++i) {
		rootSequence->Reset();
		behaviourTimer = 0.0f;
		distanceToTarget = rand() % 250;
		BehaviourState state = Ongoing;
		std::cout << "We're going on an adventure!\n";
	
		while (state == Ongoing) {
			state = rootSequence->Execute(1.0f);
		}
		if (state == Success) {
			std::cout << "What a successful adventure!\n";
		}
		else if (state == Failure) {
			std::cout << "What a waste of time!\n";
		}
	}
	std::cout << "All done!\n";

}

class TestPacketReceiver : public PacketReceiver {
public:
	TestPacketReceiver(string name) {
		this->name = name;
	}

	void RecievePacket(int type, GamePacket* payload, int source) {
		if (type == String_Message) {
			StringPacket* realPacket = (StringPacket*)payload;

			string msg = realPacket->GetStringFromData();

			std::cout << name << " recieved message: " << msg << std::endl;
		}
	}
protected:
	string name;
};

void TestNetworking() {
	/*NetworkBase::Initialise();

	TestPacketReceiver serverReceiver("Server");
	TestPacketReceiver clientReceiver("Client");

	int port = NetworkBase::GetDefaultPort();

	GameServer* server = new GameServer(port, 1);
	GameClient* client = new GameClient();

	server->RegisterPacketHandler(String_Message, &serverReceiver);
	client->RegisterPacketHandler(String_Message, &clientReceiver);

	bool canConnect = client->Connect(127, 0, 0, 1, port);

	for (int i = 0; i < 100; ++i) {
		server->SendGlobalPacket(StringPacket("Server says hello! " + std::to_string(i)));

		client->SendPacket(StringPacket("Client says hello! " + std::to_string(i)));

		server->UpdateServer();
		client->UpdateClient();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	NetworkBase::Destroy();*/
}

class PauseScreen : public PushdownState {
public:
	PauseScreen(Window* window, TutorialGame* game) {
		w = window;
		g = game;
	}
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		Debug::Print("Press U to unpause!", Vector2(5, 5), Debug::WHITE);
		Debug::Print("Press F1 to restart!", Vector2(5, 10), Debug::WHITE);

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::U)) {
			g->TogglePaused();
			return PushdownResult::Pop;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
			g->SetLevel(0);
			g->SetScore(0);
			g->SetWon(false);
			g->SetTimeLeft(60.0f);
			g->ResetBoostTimer();
			g->SetBonus(false);
			PushdownResult::Pop;
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Press U to unpause game!\n";
	}
protected:
	Window* w;
	TutorialGame* g;
};

class EoLScreen : public PushdownState {
public:
	EoLScreen(Window* window, TutorialGame* game) {
		w = window;
		g = game;
	}
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (g->GetWon()) {
			Debug::Print("You have won!", Vector2(5, 5), Debug::GREEN);
		}
		else {
			Debug::Print("You have lost!", Vector2(5, 5), Debug::RED);
		}
		Debug::Print("End Score:", Vector2(5, 10), Debug::WHITE);
		Debug::Print(std::to_string(g->GetScore()), Vector2(25, 10), Debug::WHITE);

		Debug::Print("Press R to restart level", Vector2(5, 15), Debug::WHITE);
		Debug::Print("Press F1 to return to menu", Vector2(5, 20), Debug::WHITE);

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::R)) {
			g->SetLevel(g->GetLevel());
			g->SetScore(0);
			g->SetWon(false);
			g->SetTimeLeft(60.0f);
			g->ResetBoostTimer();
			g->SetBonus(false);
			return PushdownResult::Pop;
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::F1)) {
			g->SetLevel(0);
			g->SetScore(0);
			g->SetWon(false);
			g->SetTimeLeft(60.0f);
			g->ResetBoostTimer();
			g->SetBonus(false);
			PushdownResult::Pop;
			return PushdownResult::Pop;
		}
	
		return PushdownResult::NoChange;
	}
protected:
	Window* w;
	TutorialGame* g;
};

class GameScreen : public PushdownState {
public:
	GameScreen(Window* window, TutorialGame* game) {
		w = window;
		g = game;
	}
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		Debug::Print("Press P to pause", Vector2(5, 5), Debug::WHITE);

		// if game level is 1 or 2, show menus including score, current powerup (if any)
		if (g->GetLevel() == 1 || g->GetLevel() == 2) {
			if (g->GetLevel() == 1) {
				g->SetMaxScore(4);
			}

			if (g->GetBonus()) {
				Debug::Print("Speed Boost!", Vector2(5, 10), Debug::GREEN);
			}

			Debug::Print("Score:", Vector2(65, 5), Debug::WHITE);
			Debug::Print(std::to_string(g->GetScore()), Vector2(77, 5), Debug::GREEN);
			Debug::Print("Time:", Vector2(65, 10), Debug::WHITE);
			Debug::Print(std::to_string(g->GetTimeLeft()), Vector2(77, 10), Debug::RED);

			if (g->GetEndLevel()) {
				g->TogglePaused();
				*newState = new EoLScreen(w, g);
				return PushdownResult::Push;
			}

		}
		if (g->GetLevel() == 3) {
			Debug::Print("Press 1 to toggle pathfinding test!", Vector2(5, 15), Debug::WHITE);
			Debug::Print("Press 2 to toggle behaviour tree test! (Console)", Vector2(5, 20), Debug::WHITE);
			if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM1)) {
				pathfind = !pathfind;
			}
			if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM2)) {
				behaviourTree = !behaviourTree;
			}
			if (pathfind) {
				DisplayPathfinding();
			}
			if (behaviourTree) {
				TestBehaviourTree();
				behaviourTree = !behaviourTree;
			}
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
			g->TogglePaused();
			*newState = new PauseScreen(w, g);
			return PushdownResult::Push;
		}
		return PushdownResult::NoChange;
	};
	void OnAwake() override {
		std::cout << "Playing Game!";
	}
protected:
	Window* w;
	TutorialGame* g;

	bool pathfind = false;
	bool behaviourTree = false;
};

class IntroScreen : public PushdownState {
public:
	IntroScreen(Window* window, TutorialGame* game) {
		w = window;
		g = game;
	}
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		Debug::Print("Welcome to my game!", Vector2(5, 5), Debug::WHITE);
		Debug::Print("Press 1 to load Goat Imitator 2022!", Vector2(5, 10), Debug::WHITE);
		Debug::Print("Press 2 to load Uncooperative Goose Game!", Vector2(5, 15), Debug::WHITE);
		Debug::Print("Press 3 to load the debug world!", Vector2(5, 20), Debug::WHITE);
		Debug::Print("Press Escape to quit!", Vector2(5, 25), Debug::WHITE);

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM1)) {
			g->TogglePaused();
			g->SetLevel(1);
			*newState = new GameScreen(w, g);
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM2)) {
			g->TogglePaused();
			g->SetLevel(2);
			*newState = new GameScreen(w, g);
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM3)) {
			g->TogglePaused();
			g->SetLevel(3);
			*newState = new GameScreen(w, g);
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	};
	void OnAwake() override {}
protected:
	Window* w;
	TutorialGame* g;
};

void TestPushdownAutomata(Window* w, TutorialGame* g) {
	PushdownMachine machine(new IntroScreen(w, g));
	TestPathfinding();

	TestStateMachine();

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		float dt = w->GetTimer()->GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!
		if (!machine.Update(dt)) {
			return;
		}
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			w->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::T)) {
			w->SetWindowPosition(0, 0);
		}
		g->UpdateGame(dt);

		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));
	}
}

/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/
int main() {
	Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 720);

	if (!w->HasInitialised()) {
		return -1;
	}	

	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);

	TutorialGame* g = new TutorialGame();

	//TestNetworking();

	TestPushdownAutomata(w, g);

	Window::DestroyGameWindow();
}