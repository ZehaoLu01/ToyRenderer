#pragma once
#include<memory>
#include<vector>
#include"Resource/stb_image.h"
#include "glad/glad.h"
#include <string>
#include <memory>
namespace ToyEngine {

	enum TextureType {
		Ambient,
		Specular,
		Diffuse,
		Color,
		Normal,
		Height,
		UNKNOWN
	};

	// Right now only support unsigned char data.
	using TextureDataType = unsigned char;
	using std::shared_ptr;

	class Texture
	{
		friend class RenderComponent;
		const int INVALID_ID = -1;
	public:
		Texture() = default;

		Texture(std::string path, TextureType type, bool flip) :mPath(path), mTextureType(type) {
			loadFromPath(flip);
		}

		Texture(std::string path, TextureType type, stbi_uc const* buffer, int len, bool flip):mPath(path),mTextureType(type) {
			loadFromBuf(buffer, len, flip);
		}

		Texture(const Texture& other);
		Texture& operator=(const Texture other);

		~Texture() = default;

		std::string getTypeName();

		std::string getPath() const {
			return mPath;
		}

		GLuint getTextureIndex() const {
			return mTextureIndex;
		}
		unsigned int getWidth() const {
			return mWidth;
		}
		unsigned int getHeight() const {
			return mHeight;
		}

		bool isValid() const {
			return mTextureIndex != INVALID_ID;
		}

		operator bool() const {
			return isValid();
		}

		void setType(TextureType type) {
			mTextureType = type;
		}

		TextureType getTextureType() {
			return mTextureType;
		}

	private:
		GLenum convertChannelsToFormat(unsigned int channels);

		void loadFromPath(bool flip);
		void loadFromBuf(stbi_uc const* buffer, int len, bool shouldFlip);

		int mWidth=-1;
		int mHeight=-1;
		
		// the format(color chanals) we want to store.
		GLenum mInternalFormat = GL_RGBA;
		
		// the mipmap level for which we want to create a texture for if you want to set each mipmap level manually
		unsigned int mMipmapLevel = 0;
		
		// the format(color chanals) of the source image
		GLenum mSourceFormat = GL_RGBA;

		GLuint mTextureIndex=INVALID_ID;

		TextureType mTextureType;

		std::string mPath;
	};
}


