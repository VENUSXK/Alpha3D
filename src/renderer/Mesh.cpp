#include "core/opengl.h"
#include "core/Log.h"

#include "renderer/Mesh.h"
#include "renderer/Texture.h"
#include "renderer/Shader.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	// normal
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	// texcoords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	// tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

	glBindVertexArray(0);
}

Mesh::~Mesh() {
	if (vao) glDeleteVertexArrays(1, &vao);
	if (vbo) glDeleteBuffers(1, &vbo);
	if (ebo) glDeleteBuffers(1, &ebo);
}

void Mesh::Bind() {
	glBindVertexArray(vao);
}

void Mesh::FillMissingTextures() {
	auto has = [&](const std::string& type) {
		for (auto& t : textures) if (t.GetType() == type) return true;
		return false;
	};

	if (!has("albedo"))   textures.push_back(Texture(DefaultTextures::White(), "albedo"));
	if (!has("normal"))   textures.push_back(Texture(DefaultTextures::Normal(), "normal"));
	if (!has("arm"))      textures.push_back(Texture(DefaultTextures::White(), "arm"));
	if (!has("emissive")) textures.push_back(Texture(DefaultTextures::Black(), "emissive"));
}


void Mesh::Draw() {
	glBindVertexArray(vao);

	if (indices.empty()) { // Cube
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertex_count));
	}
	else { // Sphere
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>((GLsizei)indices.size()), GL_UNSIGNED_INT, nullptr);
	}

	glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader)
{
	unsigned int albedoNr = 1;
	unsigned int normalNr = 1;
	unsigned int armNr = 1;
	unsigned int emissiveNr = 1;
	for (unsigned int i = 0; i < this->textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i + 3); 
		//LOG_TRACE("loading texture {}", GL_TEXTURE0 + i + 3);
		std::string number;
		std::string type = this->textures[i].GetType();
		if (type == "albedo")
			number = std::to_string(albedoNr++);
		else if (type == "normal")
			number = std::to_string(normalNr++);
		else if (type == "arm")
			number = std::to_string(armNr++);
		else if (type == "emissive")
			number = std::to_string(emissiveNr++);

		shader.setInt(("material." + type + "_" + number).c_str(), i + 3);
		//LOG_TRACE("material." + type + "_" + number);
		glBindTexture(GL_TEXTURE_2D, this->textures[i].GetId());
	}
	glActiveTexture(GL_TEXTURE0);


	Draw();
}
