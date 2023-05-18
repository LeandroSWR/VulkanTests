#include "calc_tangents.hpp"
#include <cassert>
#include <stdexcept>
#include <gl/GL.h>
#include <iostream>

#include <format>

namespace vt {
    /*CalcTangents::CalcTangents() {
        iface.m_getNumFaces = get_num_faces;
        iface.m_getNumVerticesOfFace = get_num_vertices_of_face;

        iface.m_getNormal = get_normal;
        iface.m_getPosition = get_position;
        iface.m_getTexCoord = get_tex_coords;
        iface.m_setTSpaceBasic = set_tspace_basic;

        context.m_pInterface = &iface;
    }

    void CalcTangents::calc(VtModel::Builder* mesh) {

        context.m_pUserData = mesh;

        genTangSpaceDefault(&this->context);
    }

    int CalcTangents::get_num_faces(const SMikkTSpaceContext* context) {
        VtModel::Builder* working_mesh = static_cast<VtModel::Builder*> (context->m_pUserData);

        return (int)working_mesh->indices.size() / 3;
    }

    int CalcTangents::get_num_vertices_of_face(const SMikkTSpaceContext* context,
        const int iFace) {
        return 3;
    }

    void CalcTangents::get_position(const SMikkTSpaceContext* context,
        float* outpos,
        const int iFace, const int iVert) {

        VtModel::Builder* working_mesh = static_cast<VtModel::Builder*> (context->m_pUserData);

        auto index = get_vertex_index(context, iFace, iVert);
        auto vertex = working_mesh->vertices[index];

        outpos[0] = vertex.position.x;
        outpos[1] = vertex.position.y;
        outpos[2] = vertex.position.z;
    }

    void CalcTangents::get_normal(const SMikkTSpaceContext* context,
        float* outnormal,
        const int iFace, const int iVert) {
        VtModel::Builder* working_mesh = static_cast<VtModel::Builder*> (context->m_pUserData);

        auto index = get_vertex_index(context, iFace, iVert);
        auto vertex = working_mesh->vertices[index];

        outnormal[0] = vertex.normal.x;
        outnormal[1] = vertex.normal.y;
        outnormal[2] = vertex.normal.z;
    }

    void CalcTangents::get_tex_coords(const SMikkTSpaceContext* context,
        float* outuv,
        const int iFace, const int iVert) {
        VtModel::Builder* working_mesh = static_cast<VtModel::Builder*> (context->m_pUserData);

        auto index = get_vertex_index(context, iFace, iVert);
        auto vertex = working_mesh->vertices[index];

        outuv[0] = vertex.uv.x;
        outuv[1] = vertex.uv.y;
    }

    void CalcTangents::set_tspace_basic(const SMikkTSpaceContext* context,
        const float* tangentu,
        const float fSign, const int iFace, const int iVert) {
        VtModel::Builder* working_mesh = static_cast<VtModel::Builder*> (context->m_pUserData);


        auto index = get_vertex_index(context, iFace, iVert);
        auto* vertex = &working_mesh->vertices[index];

        vertex->tangent.x = tangentu[0];
        vertex->tangent.y = tangentu[1];
        vertex->tangent.z = tangentu[2];
        vertex->tangent.w = fSign;
    }

    int CalcTangents::get_vertex_index(const SMikkTSpaceContext* context, int iFace, int iVert) {
        VtModel::Builder* working_mesh = static_cast<VtModel::Builder*> (context->m_pUserData);

        auto face_size = get_num_vertices_of_face(context, iFace);

        auto indices_index = (iFace * face_size) + iVert;

        int index = working_mesh->indices[indices_index];
        return index;
    }*/
}