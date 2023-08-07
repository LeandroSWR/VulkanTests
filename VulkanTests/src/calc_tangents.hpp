#pragma once

#include "mikktspace.h"
#include <vt_model.hpp>

#define CALC_TANGENTS_DEBUG 0

namespace vt {

    /*class Mesh;

    class CalcTangents {

    public:
        CalcTangents();
        void calc(VtModel::Builder* mesh);

    private:

        SMikkTSpaceInterface iface{};
        SMikkTSpaceContext context{};

        static int get_vertex_index(const SMikkTSpaceContext* context, int iFace, int iVert);

        static int get_num_faces(const SMikkTSpaceContext* context);
        static int get_num_vertices_of_face(const SMikkTSpaceContext* context, int iFace);
        static void get_position(const SMikkTSpaceContext* context, float outpos[],
            int iFace, int iVert);

        static void get_normal(const SMikkTSpaceContext* context, float outnormal[],
            int iFace, int iVert);

        static void get_tex_coords(const SMikkTSpaceContext* context, float outuv[],
            int iFace, int iVert);

        static void set_tspace_basic(const SMikkTSpaceContext* context,
            const float tangentu[],
            float fSign, int iFace, int iVert);

    };*/
}
