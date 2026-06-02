#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <mikktspace.h>
#include <renderer/renderer.hpp>

struct MikktContextData {
	uint32_t vert_count;
	uint32_t index_count;

	uint8_t *index_array;
	uint32_t index_stride;

	glm::vec3 *positions;
	glm::vec3 *normals;
	glm::vec2 *uvs;
	glm::vec4 *tangents;
};

uint32_t get_index(const MikktContextData *data, int face, int vert) {
	const uint32_t offset =
			(face * 3 + vert) * data->index_stride;

	uint32_t index = 0;

	memcpy(
			&index,
			&data->index_array[offset],
			data->index_stride);

	return index;
}

int getNumFaces(const SMikkTSpaceContext *context) {
	const auto *data =
			reinterpret_cast<const MikktContextData *>(context->m_pUserData);

	return static_cast<int>(data->index_count / 3);
}

int getNumVerticesOfFace(const SMikkTSpaceContext *, const int) {
	return 3;
}

void getPosition(
		const SMikkTSpaceContext *context,
		float outpos[],
		const int face,
		const int vert) {
	const auto *data =
			reinterpret_cast<const MikktContextData *>(context->m_pUserData);

	const uint32_t index = get_index(data, face, vert);

	const glm::vec3 &p = data->positions[index];

	outpos[0] = p.x;
	outpos[1] = p.y;
	outpos[2] = p.z;
}

void getTexCoord(
		const SMikkTSpaceContext *context,
		float outuv[],
		const int face,
		const int vert) {
	const auto *data =
			reinterpret_cast<const MikktContextData *>(context->m_pUserData);

	const uint32_t index = get_index(data, face, vert);

	const glm::vec2 &uv = data->uvs[index];

	outuv[0] = uv.x;
	outuv[1] = uv.y;
}

void getNormal(
		const SMikkTSpaceContext *context,
		float outnormal[],
		const int face,
		const int vert) {
	const auto *data =
			reinterpret_cast<const MikktContextData *>(context->m_pUserData);

	const uint32_t index = get_index(data, face, vert);

	const glm::vec3 &n = data->normals[index];

	outnormal[0] = n.x;
	outnormal[1] = n.y;
	outnormal[2] = n.z;
}

void setTSpaceBasic(
		const SMikkTSpaceContext *context,
		const float tangent[],
		const float sign,
		const int face,
		const int vert) {
	auto *data =
			reinterpret_cast<MikktContextData *>(context->m_pUserData);

	const uint32_t index = get_index(data, face, vert);

	data->tangents[index] = glm::vec4(
			tangent[0],
			tangent[1],
			tangent[2],
			sign);
}

void generateTangents(RE::Mesh::Primitive::Arg &prim_data) {
	MikktContextData ctx_data{};

	ctx_data.vert_count = prim_data.vert_count;
	ctx_data.index_count = prim_data.index_count;

	ctx_data.index_array = reinterpret_cast<uint8_t *>(
			prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::INDEX)].get());
	ctx_data.index_stride =
			(prim_data.index_count >= UINT16_MAX) ? 4 : 2;
	ctx_data.positions = reinterpret_cast<glm::vec3 *>(
			prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::POSITION)].get());
	ctx_data.normals = reinterpret_cast<glm::vec3 *>(
			prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::NORMAL)].get());
	ctx_data.uvs = reinterpret_cast<glm::vec2 *>(
			prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::UV)].get());
	ctx_data.tangents = reinterpret_cast<glm::vec4 *>(
			prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::TANGENT)].get());

	SMikkTSpaceInterface iface{};

	iface.m_getNumFaces = getNumFaces;
	iface.m_getNumVerticesOfFace = getNumVerticesOfFace;
	iface.m_getPosition = getPosition;
	iface.m_getNormal = getNormal;
	iface.m_getTexCoord = getTexCoord;
	iface.m_setTSpaceBasic = setTSpaceBasic;

	SMikkTSpaceContext mikkt_context{};
	mikkt_context.m_pInterface = &iface;
	mikkt_context.m_pUserData = &ctx_data;

	const int success = genTangSpaceDefault(&mikkt_context);

	if (!success) {
		LOG_ERROR("MikkTSpace tangent generation failed");
	}
}
