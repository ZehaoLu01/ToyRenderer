// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <iostream>
#include <sstream>
#include <fstream>
#include <Renderer/RenderSystem.h>
#include <Resource/StbImageLoader.h>
#include <Resource/Texture.h>
#include <Resource/stb_image.h>
#include <glm/gtc/type_ptr.hpp>
#include <Engine/Component.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <entt/entity/registry.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "UI/View/ImGuiManager.h"
#include <Renderer/Line.h>
#include <UI/Controller/InspectorPanelController.h>
#include <Utils/Logger.h>
#include <Utils/RenderHelper.h>

#define SELF_ROTATION 0
#define NUM_OF_TEXTURE_TYPE 3

namespace ToyEngine {
	RenderSystem RenderSystem::instance = RenderSystem();

	const glm::vec3 LIGHT_BULB_POSITION(5.0f, 5.0f, 5.0f);

	const glm::vec3 PHONG_TESTING_POSITION(0.f, 0.f, 2.f);
	const glm::vec3 PHONG_AMBIENT_COLOR(0.2f, 0.2f, 0.2f);
	const glm::vec3 PHONG_DIFFUSE_COLOR(1.0f, 0.0f, 0.0f);
	const glm::vec3 PHONG_SPECULAR_COLOR(1.0f, 1.0f, 1.0f);



	const glm::vec3 BLINN_PHONG_TESTING_POSITION(2.f, 0.f, 0.f);
	const glm::vec3 BLINN_PHONG_AMBIENT_COLOR(0.2f, 0.2f, 0.2f);
	const glm::vec3 BLINN_PHONG_DIFFUSE_COLOR(0.0f, 1.0f, 0.0f);
	const glm::vec3 BLINN_PHONG_SPECULAR_COLOR(1.0f, 1.0f, 1.0f);


	GLenum convertChannelsToFormat(unsigned int channels) {
		GLenum format = GL_NONE;
		if (channels == 1)
			format = GL_RED;
		else if (channels == 3)
			format = GL_RGB;
		else if (channels == 4)
			format = GL_RGBA;
		return format;
	}

	void RenderSystem::afterDraw()
	{
		glfwSwapBuffers(mWindow.get());
		glfwPollEvents();
	}

	void RenderSystem::preDraw()
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void RenderSystem::drawGridLine()
	{
		glm::mat4 projection = glm::perspective(glm::radians(mCamera->mZoom), 1920.0f / 1080.0f, 0.1f, 100.0f);

		glUseProgram(mGridShader->ID);
		glm::highp_mat4 mvp = projection * mCamera->GetViewMatrix();
		glUniformMatrix4fv(glGetUniformLocation(mGridShader->ID, "MVP"), 1, GL_FALSE, &mvp[0][0]);
		glUniform3fv(glGetUniformLocation(mGridShader->ID, "color"), 1, &mGridLineColor[0]);
		glBindVertexArray(mGridVAOIndex);
		glDrawArrays(GL_LINES, 0, mGridPoints.size());

	}

	void RenderSystem::drawCoordinateIndicator(glm::vec3 position)
	{
		glm::mat4 projection = glm::perspective(glm::radians(mCamera->mZoom), 1920.0f / 1080.0f, 0.1f, 100.0f);

		Line lineX = Line(position, position + glm::vec3(1, 0, 0));
		lineX.setMVP(projection * mCamera->GetViewMatrix());
		lineX.setColor(vec3(255, 0, 0));
		lineX.draw();
		Line lineY = Line(position, position + glm::vec3(0, 1, 0));
		lineY.setMVP(projection * mCamera->GetViewMatrix());
		lineY.setColor(vec3(0, 0, 255));
		lineY.draw();
		Line lineZ = Line(position, position + glm::vec3(0, 0, 1));
		lineZ.setMVP(projection * mCamera->GetViewMatrix());
		lineZ.setColor(vec3(0, 255, 0));
		lineZ.draw();
	}

	void RenderSystem::drawMesh(const TransformComponent& transform, const MeshComponent& mesh, MaterialComponent material)
	{	
		try {
			mesh.shader->use();

			glActiveTexture(GL_TEXTURE0);

			if (!material.diffuseTextures.empty()) {
				// bind diffuse map
				//TODO: Use multiple textures
				glBindTexture(GL_TEXTURE_2D, material.diffuseTextures[0].getTextureIndex());
			}
			else {
				glBindTexture(GL_TEXTURE_2D, mMissingTextureDiffuse.getTextureIndex());
				//throw(std::overflow_error("Attempting to access the first diffuse map but there is no diffuse texture."));
			}

			glActiveTexture(GL_TEXTURE1);
			if (material.specularTexture.isValid()) {
				// bind specular map
				glBindTexture(GL_TEXTURE_2D, material.specularTexture.getTextureIndex());
			}
			else {
				glBindTexture(GL_TEXTURE_2D, mMissingTextureSpecular.getTextureIndex());
				//throw(std::overflow_error("Attempting to bind invalid specular map."));
			}


			// bind texture maps
			mesh.shader->setUniform("material.diffuse", 0);
			mesh.shader->setUniform("material.specular", 1);
			mesh.shader->setUniform("material.shininess", material.shininess);

			glActiveTexture(GL_TEXTURE0);

			applyLighting(mesh.shader.get());

			auto model = glm::mat4(1.0f);

			glm::vec3 worldPos = transform.getWorldPos();
			glm::vec3 worldRot = transform.getWorldRotation();
			glm::vec3 worldScale = transform.getWorldScale();

			if (SELF_ROTATION) {
				// rotation need to be improved
				auto model_rotate = glm::rotate(model, (float)glfwGetTime() * glm::radians(40.0f), glm::vec3(0.5f, 1.0f, 0.0f));
				auto model_translate = glm::translate(model, worldPos);
				model = model_translate * model_rotate;
			}
			else {
				auto model_translate = glm::translate(model, worldPos);
				auto model_rotate = glm::rotate(glm::mat4(1.0f), glm::radians(worldRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
				model_rotate = glm::rotate(model_rotate, glm::radians(worldRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
				model_rotate = glm::rotate(model_rotate, glm::radians(worldRot.z), glm::vec3(0.0f, 0.0f, 1.0f));

				model = glm::scale(model_translate * model_rotate, worldScale);
			}
			mesh.shader->setUniform("model", model);

			auto view = glm::mat4(1.0f);
			// note that we're translating the scene in the reverse direction of where we want to move
			view = mCamera->GetViewMatrix();
			//view = glm::translate(view, glm::vec3(0.0f, 0.0f, -10.0f));
			mesh.shader->setUniform("view", view);
			mesh.shader->setUniform("viewPos", mCamera->Position);

			mesh.shader->setUniform("normalMat", glm::transpose(glm::inverse(view * model)));

			auto projection = glm::mat4(1);
			projection = glm::perspective(glm::radians(mCamera->mZoom), 1920.0f / 1080.0f, 0.1f, 100.0f);
			mesh.shader->setUniform("projection", projection);

			glBindVertexArray(mesh.VAOIndex);
			glDrawElements(GL_TRIANGLES, mesh.vertexSize, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);

			// Clear texture binding so that current binding will not affect other meshes.
			for (int i = 0; i < 5; i++)
			{
				glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
		catch (std::overflow_error err) {
			Logger::DEBUG_ERROR(err.what());
		}
	}

	void RenderSystem::drawImGuiManager()
	{
		ui::ImGuiManager::getInstance().tick();
	}

	void RenderSystem::drawPointLight()
	{
		mLightCubeShader->use();

		auto view = mCamera->GetViewMatrix();
		//view = glm::translate(view, glm::vec3(0.0f, 0.0f, -10.0f));
		mLightCubeShader->setUniform("view", view);

		auto projection = glm::mat4(1);
		projection = glm::perspective(glm::radians(mCamera->mZoom), 1920.0f / 1080.0f, 0.1f, 100.0f);
		
		mLightCubeShader->setUniform("projection", projection);
		mLightCubeShader->setUniform("view", view);
		auto lightEntities = mScene->getLightEntities();
		std::vector<entt::entity> pointLights = std::get<1>(lightEntities);
		for (entt::entity entity : pointLights) {
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, mScene->getRegistry().get<TransformComponent>(entity).localPos);
			model = glm::scale(model, glm::vec3(0.2f)); // smaller cube
			mLightCubeShader->setUniform("model", model);
			mScene->getRegistry().get<LightComponent>(entity).draw();
		}
	}

	void RenderSystem::initGrid()
	{
		mGridShader = std::make_shared<Shader>("Shaders/GridVertex.glsl", "Shaders/GridFragment.glsl");

		const int gridWidth = 500;
		const int gridHeight = 500;
		constexpr int pointNum = 2 * (gridWidth + 1 + gridHeight + 1);
		mGridPoints.reserve(pointNum * 3);
		for (int i = -gridWidth / 2; i < gridWidth / 2 + 1; i++) {
			mGridPoints.push_back(i);
			mGridPoints.push_back(0);
			mGridPoints.push_back(-gridWidth / 2);
			mGridPoints.push_back(i);
			mGridPoints.push_back(0);
			mGridPoints.push_back(gridWidth / 2);
		}
		for (int i = -gridWidth / 2; i < gridWidth / 2 + 1; i++) {
			mGridPoints.push_back(-gridWidth / 2);
			mGridPoints.push_back(0);
			mGridPoints.push_back(i);
			mGridPoints.push_back(gridWidth / 2);
			mGridPoints.push_back(0);
			mGridPoints.push_back(i);
		}

		glGenVertexArrays(1, &mGridVAOIndex);
		glGenBuffers(1, &mGridVBOIndex);
		glBindVertexArray(mGridVAOIndex);
		glBindBuffer(GL_ARRAY_BUFFER, mGridVBOIndex);
		glBufferData(GL_ARRAY_BUFFER, mGridPoints.size() * sizeof(float), mGridPoints.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
	}

	// Setup active shader, camera, window.
	void RenderSystem::init(WindowPtr window, std::shared_ptr<Camera> camera, std::shared_ptr<Scene> scene) {
		mWindow = window;
		mCamera = camera;
		mScene = scene;
		glEnable(GL_DEPTH_TEST);

		initGrid();

		//ImGui
		setupImGUI();
		ui::ImGuiManager::getInstance().setupControllers(scene);

		mLightCubeShader = std::make_shared<Shader>("Shaders/lightingShader.vert", "Shaders/lightingShader.frag");

		mMissingTextureDiffuse = Texture("Resources\\Images\\missing_texture_diffuse.png", ToyEngine::TextureType::Diffuse, false);
		mMissingTextureSpecular = Texture("Resources\\Images\\missing_texture_specular.png", ToyEngine::TextureType::Specular, false);

		mSkyBox = SkyBox({
				   "Resources/Images/skybox/right.jpg",
				   "Resources/Images/skybox/left.jpg",
				   "Resources/Images/skybox/top.jpg",
				   "Resources/Images/skybox/bottom.jpg",
				   "Resources/Images/skybox/front.jpg",
				   "Resources/Images/skybox/back.jpg"
			}, camera);
	}

	void RenderSystem::setupImGUI()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(mWindow.get(), true);
		ImGui_ImplOpenGL3_Init("#version 130");
	}

	entt::entity RenderSystem::loadModel(std::string path, std::string modelName, entt::registry& registry, entt::entity parent)
	{
		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);

		// TODO: What if the model has already been loaded before?

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
			return entt::null;
		}

		std::vector<Texture> textures(scene->mNumMaterials * NUM_OF_TEXTURE_TYPE);

		//TODO: change to path
		std::string directory = path;

		entt::entity entity = registry.create();

		auto& parentTransform = registry.get<TransformComponent>(parent);
		auto& newTrasnform = registry.emplace<TransformComponent>(entity);
		newTrasnform.addParentTransform(parentTransform);

		auto child = processNode(scene->mRootNode, scene, registry, entity, textures, directory);
		//TODO: PRE, NEXT
		auto& relation = registry.emplace<RelationComponent>(entity, parent, std::list<entt::entity>());
		if (modelName.size() == 0) {
			registry.emplace<TagComponent>(entity, "default model");
		}
		else {
			registry.emplace<TagComponent>(entity, modelName);
		}
		relation.children.push_back(child);
		return entity;
	}

	// Get ambient/diffuse/specualr color from material
	aiColor4D RenderSystem::getColorFromMaterialOfType(const aiTextureType type, const aiMaterial* const pMaterial)
	{
		aiColor4D aiColor = aiColor4D(1.0, 1.0, 1.0, 1.0);

		if (type == aiTextureType_DIFFUSE) {
			if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) != aiReturn_SUCCESS) {
				aiColor = aiColor4D(1.0, 1.0, 1.0, 1.0);
				Logger::DEBUG_ERROR("Error getting color for material of type " + std::to_string(type));
			}
		}
		else if (type == aiTextureType_SPECULAR) {
			if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, aiColor) != aiReturn_SUCCESS) {
				aiColor = aiColor4D(1.0, 1.0, 1.0, 1.0);
				Logger::DEBUG_ERROR("Error getting color for material of type " + std::to_string(type));

			}
		}
		else if (type == aiTextureType_AMBIENT) {
			if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, aiColor) != aiReturn_SUCCESS) {
				aiColor = aiColor4D(1.0, 1.0, 1.0, 1.0);
				Logger::DEBUG_ERROR("Error getting color for material of type " + std::to_string(type));
			}
		}
		else if (type == aiTextureType_HEIGHT) {
			aiColor = aiColor4D(0.0, 0.0, 0.0, 0.0);
		}
		else if (type == aiTextureType_NORMALS) {
			aiColor = aiColor4D(0.0, 0.0, 0.0, 0.0);
		}
		else {
			Logger::DEBUG_ERROR("Unable to extract color from texture type" + std::to_string(type));
		}
		return aiColor;
	}


	void RenderSystem::setupTextureOfType(entt::entity entity, aiTextureType type, aiMaterial* const& pMaterial, const std::string& directory, const aiScene* scene)
	{
		Logger::DEBUG_INFO("Start setup textures of type: " + RenderHelper::getTextureTypeString(type));
		float shininess = 20.f;
		if (AI_SUCCESS != aiGetMaterialFloat(pMaterial, AI_MATKEY_SHININESS, &shininess)) {
			shininess = 20.f;
		}
		
		const aiTexture* assimpTexture = nullptr;

		aiColor4D aiColor = getColorFromMaterialOfType(type, pMaterial);
		glm::vec4 color = { aiColor.r, aiColor.g, aiColor.b, aiColor.a };

		Logger::DEBUG_INFO("Have " + std::to_string(pMaterial->GetTextureCount(type))+ " " + RenderHelper::getTextureTypeString(type) + " textures in total");
		for (int i = 0; i < pMaterial->GetTextureCount(type); i++) {
			aiString path;
			if (pMaterial->GetTexture(type, i, &path, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
				std::string p(path.data);
				Texture texture;
				bool isEmbedded = false;
				if (auto assimpTexture = scene->GetEmbeddedTexture(path.C_Str())) {
					// embedded texture
					Logger::DEBUG_INFO("The" + RenderHelper::getTextureTypeString(type) + " texture " + std::to_string(i) + " is an embedded texture.");

					isEmbedded = true;

					// add texture embedded into model file
					auto buffer = reinterpret_cast<unsigned char*>(assimpTexture->pcData);
					int len = assimpTexture->mHeight == 0 ? static_cast<int>(assimpTexture->mWidth) 
														  : static_cast<int>(assimpTexture->mWidth * assimpTexture->mHeight);
						
					texture = Texture(p, RenderHelper::ConvertTextureType(type), buffer, len, false);

					if (!texture.isValid()) {
						Logger::DEBUG_WARNING("Texture with path: " + std::string(path.C_Str()) + " is not loaded properly.");
					}
				}
				else {
					// regular texture file
					isEmbedded = false;

					std::string texturePath = std::filesystem::path(directory).parent_path().append(path.C_Str()).string();
					
					Logger::DEBUG_INFO("The " + RenderHelper::getTextureTypeString(type) + " texture " + std::to_string(i) + " is a regular texture.");
					Logger::DEBUG_INFO("Texture Path: " + texturePath);

					// add texture stored in an external image file
					if (!rm.getTexture(texturePath).isValid()) {
						Logger::DEBUG_INFO("Texture path not found in resource manager.");
						texture = Texture(texturePath, RenderHelper::ConvertTextureType(type), false);
						rm.addTexture(texturePath, texture);
					}
					else {
						Logger::DEBUG_INFO("Texture path found in resource manager. Use the texture from resource manager");

						texture = rm.getTexture(texturePath);
						if (!texture.isValid()) {
							Logger::DEBUG_WARNING("Texture with path: " + std::string(path.C_Str()) + " is not loaded properly.");
						}
					}
				}

				if (!mScene->getRegistry().try_get<MaterialComponent>(entity)) {
					mScene->getRegistry().emplace<MaterialComponent>(entity);
				}

				MaterialComponent& materialComp = mScene->getRegistry().get<MaterialComponent>(entity);
				
				// Construct material component
				materialComp.isEmbedded = isEmbedded;
				materialComp.shininess = shininess;
				if (type == aiTextureType_DIFFUSE) {
					materialComp.diffuseTextures.push_back(texture);
				}
				else if (type == aiTextureType_SPECULAR) {
					materialComp.specularTexture = texture;
				}
				else if (type == aiTextureType_HEIGHT) {
					materialComp.heightTexture = texture;
				}
				else if (type == aiTextureType_NORMALS) {
					materialComp.normalTexture = texture;
				}
				else if (type == aiTextureType_AMBIENT) {
					materialComp.ambientTexture = texture;
				}
				else {
					Logger::DEBUG_ERROR("Unable to attach texture to Material component from the Assimp material.");
				}
			}
		}

		// Set colors
		if (!mScene->getRegistry().try_get<MaterialComponent>(entity)) {
			mScene->getRegistry().emplace<MaterialComponent>(entity);
		}

		if (type == aiTextureType_DIFFUSE) {
			mScene->getRegistry().get<MaterialComponent>(entity).diffuseColor = color;
		}
		else if (type == aiTextureType_SPECULAR) {
			mScene->getRegistry().get<MaterialComponent>(entity).specularColor = color;
		}
		else if (type == aiTextureType_AMBIENT) {
			mScene->getRegistry().get<MaterialComponent>(entity).ambientColor = color;
		}
	}

	void RenderSystem::bindSiblings(entt::registry& registry, entt::entity curr, entt::entity& prev)
	{
		if (prev != entt::null) {
			auto childRelation = registry.get<RelationComponent>(curr);
			auto prevRelation = registry.get<RelationComponent>(prev);

			registry.get<RelationComponent>(curr).prev = prev;
			registry.get<RelationComponent>(prev).next = curr;
		}
		prev = curr;
	}

	entt::entity RenderSystem::processNode(aiNode* node, const aiScene* scene, entt::registry& registry, entt::entity parent, std::vector<Texture>& textures, const string& directory)
	{
		auto& parentTransform = registry.get<TransformComponent>(parent);

		entt::entity entity = registry.create();
		
		std::shared_ptr<std::vector<Vertex>> verticesPtr;
		std::shared_ptr<std::vector<unsigned int>> indicesPtr;
		auto& relation = registry.emplace<RelationComponent>(entity, parent, std::list<entt::entity>());
		auto& tag = registry.emplace<TagComponent>(entity, std::string(node->mName.C_Str()));
		auto& transform = registry.emplace<TransformComponent>(entity);
		transform.addParentTransform(parentTransform);
		

		entt::entity prev = entt::null;

		// process all the node's meshes (if any)
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			auto child = processMesh(mesh, scene, registry, entity, textures, directory);

			//bindSiblings(registry, child, prev);

			//prev = child;
			relation.children.push_back(child);
		}

		// then do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			auto child = processNode(node->mChildren[i], scene, registry, entity, textures, directory);

			//bindSiblings(registry, child, prev);

			//prev = child;
			relation.children.push_back(child);
		}
		return entity;
	}

	entt::entity RenderSystem::processMesh(aiMesh* mesh, const aiScene* scene, entt::registry& registry, entt::entity parent, std::vector<Texture>& textures, const string& directory)
	{
		auto& parenTransform = registry.get<TransformComponent>(parent);

		entt::entity entity =  registry.create();

		std::vector<Vertex> vertices;
		vector<unsigned int> indices;

		bool hasNormal = false;
		bool hasTexture = false;

		// walk through each of the mesh's vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			// positions
			vertex.Position.x = mesh->mVertices[i].x;
			vertex.Position.y = mesh->mVertices[i].y;
			vertex.Position.z = mesh->mVertices[i].z;
			// normals
			if (mesh->HasNormals())
			{
				vertex.Normal.x = mesh->mNormals[i].x;
				vertex.Normal.y = mesh->mNormals[i].y;
				vertex.Normal.z = mesh->mNormals[i].z;
			}
			// texture coordinates
			if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				glm::vec2 vec;
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vertex.TexCoords.x = mesh->mTextureCoords[0][i].x;
				vertex.TexCoords.y = mesh->mTextureCoords[0][i].y;
				// tangent
				vertex.Tangent.x = mesh->mTangents[i].x;
				vertex.Tangent.y = mesh->mTangents[i].y;
				vertex.Tangent.z = mesh->mTangents[i].z;
				// bitangent
				vertex.Bitangent.x = mesh->mBitangents[i].x;
				vertex.Bitangent.y = mesh->mBitangents[i].y;
				vertex.Bitangent.z = mesh->mBitangents[i].z;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}

		// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		
		// process material
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
			// RN we only want to get ambient diffuse and specualr texture
			setupTextureOfType(entity, aiTextureType_AMBIENT, mat, directory, scene);
			setupTextureOfType(entity, aiTextureType_DIFFUSE, mat, directory, scene);
			setupTextureOfType(entity, aiTextureType_SPECULAR, mat, directory, scene);
			setupTextureOfType(entity, aiTextureType_HEIGHT, mat, directory, scene);
			setupTextureOfType(entity, aiTextureType_NORMALS, mat, directory, scene);

		}

		// TODO USE ACTIVE SHADER
		std::shared_ptr<Shader> simpleMeshShader = std::make_shared<Shader>("Shaders/simpleMeshShader.vert", "Shaders/simpleMeshShader.frag");

		auto& transformComp = registry.emplace<TransformComponent>(entity);
		transformComp.addParentTransform(parenTransform);
		auto& meshComp = registry.emplace<MeshComponent>(entity, vertices, indices , simpleMeshShader, hasNormal, hasTexture);
		auto& relationComp = registry.emplace<RelationComponent>(entity, parent, std::list<entt::entity>());

		if (std::string(mesh->mName.C_Str()).size()) {
			registry.emplace<TagComponent>(entity, std::string(mesh->mName.C_Str()));
		}
		else {
			registry.emplace<TagComponent>(entity, "unnamed mesh");
		}

		return entity;
	}

	void RenderSystem::applyLighting(Shader* shader) {
		entt::registry& registry = mScene->getRegistry();
		
		auto lightEntities = mScene->getLightEntities();
		std::vector<entt::entity> directionalLights = std::get<0>(lightEntities);
		std::vector<entt::entity> pointLights = std::get<1>(lightEntities);
		std::vector<entt::entity> spotLights = std::get<2>(lightEntities);

		// define current number of point lights
		shader->setUniform("numberOfDirLights", (int)directionalLights.size());
		shader->setUniform("numberOfPointLights", (int)pointLights.size());
		shader->setUniform("numberOfSpotLights", (int)spotLights.size());

		for (int i = 0; i < directionalLights.size(); i++) {
			entt::entity lightEntity = directionalLights.at(i);
			LightComponent lightComponent = registry.get<LightComponent>(lightEntity);
			std::string prefix = "dirLights[" + std::to_string(i) + "]";

			// This line maybe buggy!!
			shader->setUniform(prefix + ".direction", registry.get<TransformComponent>(lightEntity).rotation_eular);   // TODO: instead of direction, use entity rotation
			shader->setUniform(prefix + ".ambient", lightComponent.ambient);
			shader->setUniform(prefix + ".diffuse", lightComponent.diffuse);
			shader->setUniform(prefix + ".specular", lightComponent.specular);
		}

		for (int i = 0; i < pointLights.size(); i++) {
			entt::entity lightEntity = pointLights.at(i);
			LightComponent lightComponent = registry.get<LightComponent>(lightEntity);
			std::string prefix = "pointLights[" + std::to_string(i) + "]";
			shader->setUniform(prefix + ".position", registry.get<TransformComponent>(lightEntity).localPos);
			shader->setUniform(prefix + ".ambient", lightComponent.ambient);
			shader->setUniform(prefix + ".diffuse", lightComponent.diffuse);
			shader->setUniform(prefix + ".specular", lightComponent.specular);
			shader->setUniform(prefix + ".constant", lightComponent.constant);
			shader->setUniform(prefix + ".linear", lightComponent.linear);
			shader->setUniform(prefix + ".quadratic", lightComponent.quadratic);
		}

		for (int i = 0; i < spotLights.size(); i++) {
			entt::entity lightEntity = spotLights.at(i);
			LightComponent lightComponent = registry.get<LightComponent>(lightEntity);
			std::string prefix = "spotLights[" + std::to_string(i) + "]";
			shader->setUniform(prefix + ".position", registry.get<TransformComponent>(lightEntity).localPos);
			shader->setUniform(prefix + ".direction", registry.get<TransformComponent>(lightEntity).front());  // TODO: consider changing this to .rotation
			shader->setUniform(prefix + ".ambient", lightComponent.ambient);
			shader->setUniform(prefix + ".diffuse", lightComponent.diffuse);
			shader->setUniform(prefix + ".specular", lightComponent.specular);
			shader->setUniform(prefix + ".constant", lightComponent.constant);
			shader->setUniform(prefix + ".linear", lightComponent.linear);
			shader->setUniform(prefix + ".quadratic", lightComponent.quadratic);
			shader->setUniform(prefix + ".cutOff", glm::cos(glm::radians(lightComponent.cutOff)));
			shader->setUniform(prefix + ".outerCutOff", glm::cos(glm::radians(lightComponent.outerCutOff)));
		}
	}
}
