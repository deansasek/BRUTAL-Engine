#include "model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

engine::model engine::model::createModel(std::string modelPath) {
	engine::model model;

	model.data.modelPath = modelPath;

	engine::model::loadModel(model);

	engine::model::createVertexBuffer();
	engine::model::createIndexBuffer();

	return model;
}

void engine::model::loadModel(engine::model model) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, model.data.modelPath.c_str())) {
		throw std::runtime_error("Failed to load model!");
	}
	else {
		logger::log("Successfully loaded model!", 1);
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			engine::model::vertexStruct vertex{};

			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.textureCoordinates = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			model.data.vertices.push_back(vertex);
			model.data.indices.push_back(model.data.indices.size());
		}
	}
}

void engine::model::renderModel() {

}

void engine::model::destroyModel() {

}

void engine::model::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(engine::model::data.vertices[0]) * engine::model::data.vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	renderer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(renderer::device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, engine::model::data.vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(renderer::device, stagingBufferMemory);

	renderer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderer::vertexBuffer, renderer::vertexBufferMemory);

	renderer::copyBuffer(stagingBuffer, renderer::vertexBuffer, bufferSize);

	vkDestroyBuffer(renderer::device, stagingBuffer, nullptr);
	vkFreeMemory(renderer::device, stagingBufferMemory, nullptr);

	logger::log("Successfully created vertex buffer!", 1);
}

void engine::model::createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(engine::model::data.indices[0]) * engine::model::data.indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	renderer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(renderer::device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, engine::model::data.indices.data(), (size_t)bufferSize);
	vkUnmapMemory(renderer::device, stagingBufferMemory);

	renderer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderer::indexBuffer, renderer::indexBufferMemory);

	renderer::copyBuffer(stagingBuffer, renderer::indexBuffer, bufferSize);

	vkDestroyBuffer(renderer::device, stagingBuffer, nullptr);
	vkFreeMemory(renderer::device, stagingBufferMemory, nullptr);

	logger::log("Successfully created index buffer!", 1);
}