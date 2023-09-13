#pragma once
#include "Engine/Scene.h"
#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <memory>
#include <imgui.h>
#include <Engine/Component.h>
#include <imgui_internal.h>
#include <UI/Controller/SceneHierarchyController.h>
namespace ui {
	class SceneHierarchyPanel
	{
		public:
			SceneHierarchyPanel() = default;
			SceneHierarchyPanel(std::shared_ptr<ToyEngine::MyScene> context) {
				SetContext(context);
			}
			SceneHierarchyPanel(std::shared_ptr<ToyEngine::MyScene> context, std::shared_ptr<SceneHierarchyController> controller):mController(controller) {
				SetContext(context);
			}
			SceneHierarchyPanel(std::shared_ptr<ToyEngine::MyScene> context, std::shared_ptr<SceneHierarchyController> controller, std::function<void(entt::entity)> onSelect) :SceneHierarchyPanel(context,controller){
				mSelectEntityCallback = onSelect;
			}


			void tick();

			entt::entity getSelectedEntity() const { return mSelectionContext; }

			void SetSelectedEntity(entt::entity entity) {
				mSelectionContext = entity;
			}

			void setOnSelectCallBack(std::function<void(entt::entity)>);

		private:
			void SetContext(const std::shared_ptr<ToyEngine::MyScene> scene) {
					mScene = scene;
			}

		private:
			entt::entity mSelectionContext=entt::null;
			std::shared_ptr<ToyEngine::MyScene> mScene;
			std::shared_ptr<SceneHierarchyController> mController;
			std::function<void(entt::entity)> mSelectEntityCallback;
	};
}
