#pragma once
#include <random>

#include "Ray.h"
#include "CollisionDetection.h"
#include "QuadTree.h"
namespace NCL {
		class Camera;
		using Maths::Ray;
	namespace CSC8503 {
		class GameObject;
		class Constraint;

		typedef std::function<void(GameObject*)> GameObjectFunc;
		typedef std::vector<GameObject*>::const_iterator GameObjectIterator;

		class GameWorld	{
		public:
			GameWorld();
			~GameWorld();

			void Clear();
			void ClearAndErase();

			void AddGameObject(GameObject* o);
			void RemoveGameObject(GameObject* o, bool andDelete = false);

			void AddConstraint(Constraint* c);
			void RemoveConstraint(Constraint* c, bool andDelete = false);

			Camera* GetMainCamera() const {
				return mainCamera;
			}

			void ShuffleConstraints(bool state) {
				shuffleConstraints = state;
			}

			void ShuffleObjects(bool state) {
				shuffleObjects = state;
			}

			bool Raycast(Ray& r, RayCollision& closestCollision, bool closestObject = false, GameObject* ignore = nullptr) const;

			virtual void UpdateWorld(float dt);

			void OperateOnContents(GameObjectFunc f);

			void GetObjectIterators(
				GameObjectIterator& first,
				GameObjectIterator& last) const;

			void GetConstraintIterators(
				std::vector<Constraint*>::const_iterator& first,
				std::vector<Constraint*>::const_iterator& last) const;

			int GetWorldStateID() const {
				return worldStateCounter;
			}

			void IncrementScore() {
				scoreCounter++;
			}

			int GetScore() {
				return scoreCounter;
			}

			void SetScore(int s) {
				scoreCounter = s;
			}

			void ToggleSpeedBoost() {
				speedBoost = !speedBoost;
			}

			bool GetSpeedBoost() {
				return speedBoost;
			}

		protected:
			std::vector<GameObject*> gameObjects;
			std::vector<Constraint*> constraints;
			
			enum LayerMask {
				UNASSIGNED	= 0,
				TERRAIN		= 1,
				COLLECTABLE	= 2,
				PLAYER		= 3,
				ENEMY		= 4,
			};

			LayerMask layer;

			Camera* mainCamera;

			bool shuffleConstraints;
			bool shuffleObjects;
			int		worldIDCounter;
			int		worldStateCounter;
			int		scoreCounter = 0;
			bool	speedBoost = false;
		};
	}
}

