#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>`
#include <iostream>
#include <vector>
#include <string>
#include <utility/stb_image.h>
#include <utility/Shader.h> // Include your Shader class header

class Model {
public:
	Model(const char* path) {
		loadModel(path);
	}

	void render(Shader& shader) {
		for (unsigned int i = 0; i < meshes.size(); ++i) {
			meshes[i].render(shader);
		}
	}

private:
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoords;
	};

	struct Texture {
		GLuint id;
		std::string type;
		aiString path; // Store the texture path for duplicate checking
	};

	class Mesh {
	public:
		std::vector<Vertex> vertices;
		std::vector<GLuint> indices;
		std::vector<Texture> textures;
		glm::vec3 diffuseColor;
		glm::vec3 specularColor;

		void render(Shader& shader) {
			glBindVertexArray(VAO);
			unsigned int diffuseNr = 1;
			unsigned int specularNr = 1;
			for (unsigned int i = 0; i < textures.size(); ++i) {
				glActiveTexture(GL_TEXTURE0 + i);
				std::string number;
				std::string name = textures[i].type;
				if (name == "texture_diffuse")
					number = std::to_string(diffuseNr++);
				else if (name == "texture_specular")
					number = std::to_string(specularNr++);
				// shader.setInt(("material." + name + number).c_str(), i);
				shader.setInt((name + number).c_str(), i);

				// debugging
				// std::cout << "setting texture for " << (name + number).c_str() << "\n";

				glBindTexture(GL_TEXTURE_2D, textures[i].id);
			}
			glActiveTexture(GL_TEXTURE0);

			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

		void setupMesh() {
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

			glBindVertexArray(0);
		}

		void loadMaterialColors(aiMaterial* mat) {
			aiColor3D diffColor, specColor;
			mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffColor);
			mat->Get(AI_MATKEY_COLOR_SPECULAR, specColor);

			diffuseColor = glm::vec3(diffColor.r, diffColor.g, diffColor.b);
			specularColor = glm::vec3(specColor.r, specColor.g, specColor.b);
		}

	private:
		GLuint VAO, VBO, EBO;
	};

	std::vector<Mesh> meshes;
	std::vector<Texture> loadedTextures; // Track loaded textures to avoid duplicates
	std::string directory;

	void loadModel(const std::string& path) {
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
			return;
		}

		directory = path.substr(0, path.find_last_of('/'));

		processNode(scene->mRootNode, scene);
	}

	void processNode(aiNode* node, const aiScene* scene) {
		for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}

		for (unsigned int i = 0; i < node->mNumChildren; ++i) {
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
		Mesh newMesh;
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			Vertex vertex;
			vertex.position.x = mesh->mVertices[i].x;
			vertex.position.y = mesh->mVertices[i].y;
			vertex.position.z = mesh->mVertices[i].z;

			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].y;
			vertex.normal.z = mesh->mNormals[i].z;

			if (mesh->mTextureCoords[0]) {
				vertex.texCoords.x = mesh->mTextureCoords[0][i].x;
				vertex.texCoords.y = mesh->mTextureCoords[0][i].y;
			}
			else {
				vertex.texCoords = glm::vec2(0.0f, 0.0f);
			}

			newMesh.vertices.push_back(vertex);
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; ++j) {
				newMesh.indices.push_back(face.mIndices[j]);
			}
		}

		if (mesh->mMaterialIndex >= 0) {
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			std::vector<Texture> diffuseTextures = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			newMesh.textures.insert(newMesh.textures.end(), diffuseTextures.begin(), diffuseTextures.end());
			std::vector<Texture> specularTextures = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			newMesh.textures.insert(newMesh.textures.end(), specularTextures.begin(), specularTextures.end());

			newMesh.loadMaterialColors(material);
		}

		newMesh.setupMesh();

		return newMesh;
	}

	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName) {
		std::vector<Texture> textures;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
			aiString str;
			mat->GetTexture(type, i, &str);

			bool skip = false;
			for (unsigned int j = 0; j < loadedTextures.size(); ++j) {
				if (std::strcmp(loadedTextures[j].path.C_Str(), str.C_Str()) == 0) {
					textures.push_back(loadedTextures[j]);
					skip = true;
					break;
				}
			}

			if (!skip) {
				Texture texture;
				texture.id = TextureFromFile(str.C_Str(), directory);
				texture.type = typeName;
				texture.path = str;
				textures.push_back(texture);
				loadedTextures.push_back(texture);
			}
		}
		return textures;
	}

	GLuint TextureFromFile(const char* path, const std::string& directory) {
		std::string filename = std::string(path);
		filename = directory + '/' + filename;

		GLuint textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
		if (data) {
			GLenum format;
			if (nrComponents == 1)
				format = GL_RED;
			else if (nrComponents == 3)
				format = GL_RGB;
			else if (nrComponents == 4)
				format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			stbi_image_free(data);
		}
		else {
			std::cerr << "Texture failed to load at path: " << path << std::endl;
			stbi_image_free(data);
		}

		return textureID;
	}
};
