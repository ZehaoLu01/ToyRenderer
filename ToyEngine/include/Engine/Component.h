#pragma once
#include <memory>
#include <string>
#include <vector>
#include "glm/glm.hpp"
#include <glad/glad.h>
#include <entt/entity/registry.hpp>
#include <stdexcept>
#include <Resource/Texture.h>
#include <Renderer/Shader.h>


namespace ToyEngine {

    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
    };

	struct MeshComponent {
		GLuint VBOIndex;
		GLuint VAOIndex;
		GLuint EBOIndex;

        std::shared_ptr<Shader> shader;

        size_t vertexSize = 0;

        bool hasNormal = false;
        bool hasTexture = false;

		// Vertex data includes coordinate, normal and 
        MeshComponent(VertexDataPtr vertexDataPtr, IndexDataPtr indicesPtr, std::shared_ptr<Shader> shaderInput, bool hasNormal = true, bool hasTexture = true) :shader(shaderInput), hasNormal(hasNormal),hasTexture(hasTexture) {
            try {
                //if (!hasNormal || !hasTexture) {
               //    throw std::invalid_argument("vertex data without texture or normal is not support currrently");
               //}

                vertexSize = indicesPtr->size();

                // generate 1 Vertex Array Object
                // Used to remember subsequent vertex attribute calls.
                // Stores:
                // 1. Calls to glEnableVertexAttribArray or glDisableVertexAttribArray.
                // 2. Vertex attribute configurations via glVertexAttribPointer.
                // 3. Vertex buffer objects associated with vertex attributes by calls to glVertexAttribPointer.
                // 4. The last EBO that gets bound while a VAO is bound.
                glGenVertexArrays(1, &VAOIndex);

                // generate 1 Vertex Buffer Object
                // VBO stores a large number of vertices in the GPU's memory.
                glGenBuffers(1, &VBOIndex);

                // EBO stores indices that OpenGL uses to decide what vertices to draw.
                glGenBuffers(1, &EBOIndex);

                // Any subsequent vertex attribute calls from this on will be stored inside the VAO
                glBindVertexArray(VAOIndex);

                glBindBuffer(GL_ARRAY_BUFFER, VBOIndex);
                glBufferData(GL_ARRAY_BUFFER, vertexDataPtr->size() * sizeof(VertexDataElementType), vertexDataPtr->data(), GL_STATIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOIndex);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesPtr->size() * sizeof(IndexDataElementType), indicesPtr->data(), GL_STATIC_DRAW);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
                glEnableVertexAttribArray(3);
                glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
                glEnableVertexAttribArray(4);


                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glBindVertexArray(0);
            }
            catch (...) {
                std::cerr << "Something went wrong when creating Mesh Component!!!" << std::endl;
            }
           
		}
	};

	struct TransformComponent {
		glm::vec3 localPos = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 rotation_eular = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

        // Reference is to avoid expensive update operations on parent transforms
        bool isReference = false;
        const TransformComponent* referencedTransform=nullptr;

        TransformComponent() = default;
        TransformComponent(glm::vec3 pos, glm::vec3 rotation, glm::vec3 scaleInput) :
            localPos(pos), rotation_eular(rotation), scale(scaleInput)
        {
        };

        // TODO: This may be error prone. Think about how to adjust this.
         void addParentTransform(const TransformComponent& other){
            isReference = true;
            referencedTransform = &other;
        };

        glm::vec3 getWorldPos() const {
            const TransformComponent* current = this;
            glm::vec3 worldPos = localPos;
            while (current->isReference) {
                current = current->referencedTransform;
                worldPos += current->localPos;
            }
            return worldPos;
        }

        glm::vec3 getWorldRotation() const{
            const TransformComponent* current = this;
            glm::vec3 worldRot = rotation_eular;
            while (current->isReference) {
                current = current->referencedTransform;
                worldRot += current->rotation_eular;
            }
            return worldRot;
        }

        glm::vec3 getWorldScale() const{
            const TransformComponent* current = this;
            glm::vec3 worldScale = scale;
            while (current->isReference) {
                current = current->referencedTransform;
                worldScale *= current->scale;
            }
            return worldScale;
        }
	};

    struct MaterialComponent {
        std::vector<Texture> textures;
        MaterialComponent(std::vector<Texture> textures) :textures(textures) {};
    };

    struct RelationComponent {
        entt::entity parent;
        entt::entity next=entt::null;
        entt::entity prev=entt::null;

        //TODO: don't use shared_ptr
        std::shared_ptr<std::vector<entt::entity>> children;
        RelationComponent(entt::entity parent, std::shared_ptr<std::vector<entt::entity>> children) :parent(parent), children(children) {};
        RelationComponent(entt::entity parent, std::shared_ptr<std::vector<entt::entity>> children, entt::entity prev, entt::entity next) :parent(parent), children(children),prev(prev), next(next) {};

        RelationComponent() = default;
    };

    struct TagComponent {
        std::string name;
        TagComponent() = default;
        TagComponent(const std::string& input) : name(input) {};
    };
}