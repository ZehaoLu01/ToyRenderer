#pragma once
#include <vector>
#include <memory>
#include "Engine/Component.h"
#include "glad/glad.h"
#include "Resource/Texture.h"

namespace ToyEngine {
	using VertexDataElementType = float;
	using VertexData = std::vector<VertexDataElementType>;
	using VertexDataPtr = std::unique_ptr<VertexData>;

	using IndexDataElementType = unsigned int;
	using IndexData = std::vector<IndexDataElementType>;
	using IndexDataPtr = std::unique_ptr<IndexData>;
	

	class RenderComponent :public Component
	{
	public:
		// Inherited via Component
		virtual void tick() override;
		virtual void init() override;
		
		RenderComponent() = default;

		RenderComponent(VertexDataPtr&& vertexDataPtr, IndexDataPtr&& indicesPtr, std::shared_ptr<Texture> textureDataPtr, GLuint shaderProgramIndex)
			:mVertexDataPtr(std::move(vertexDataPtr)),
			mtextureDataPtr(textureDataPtr),
			mIndicesPtr(std::move(indicesPtr)),
			mShaderProgramIndex(shaderProgramIndex)
		{
			init();
			mTextureIndex = textureDataPtr->mTextureIndex;
		}
	private:
		VertexDataPtr mVertexDataPtr;

		GLuint mVBOIndex;
		GLuint mVAOIndex;
		GLuint mShaderProgramIndex;
		GLuint mEBOIndex;
		GLuint mTextureIndex;

		std::shared_ptr<Texture> mtextureDataPtr;

		IndexDataPtr mIndicesPtr;

		//material
	};

}

