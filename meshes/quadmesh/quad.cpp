/****************************************************************************
* Italian National Research Council                                         *
* Institute for Applied Mathematics and Information Technologies, Genoa     *
* IMATI-GE / CNR                                                            *
*                                                                           *
* Author: Marco Livesu (marco.livesu@gmail.com)                             *
*                                                                           *
* Copyright(C) 2016                                                         *
* All rights reserved.                                                      *
*                                                                           *
* This file is part of CinoLib                                              *
*                                                                           *
* CinoLib is free software; you can redistribute it and/or modify           *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 3 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
****************************************************************************/
#include <cinolib/meshes/quadmesh/quad.h>
#include <cinolib/bfs.h>
#include <cinolib/timer.h>
#include <cinolib/io/read_write.h>
#include <cinolib/geometry/plane.h>

#include <queue>

namespace cinolib
{

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
Quad<M,V,E,F>::Quad(const char * filename)
{
    load(filename);
    init();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
Quad<M,V,E,F>::Quad(const std::vector<vec3d> & verts,
                    const std::vector<uint>  & faces)
: verts(verts)
, faces(faces)
{
    init();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
Quad<M,V,E,F>::Quad(const std::vector<double> & coords,
                    const std::vector<uint>   & faces)
{
    this->verts = vec3d_from_serialized_xyz(coords);
    this->faces = faces;

    init();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::load(const char * filename)
{
    timer_start("Load Quadmesh");

    clear();
    std::vector<double> coords;
    std::vector<uint>   tris; // unused

    std::string str(filename);
    std::string filetype = str.substr(str.size()-4,4);

    if (filetype.compare(".off") == 0 ||
        filetype.compare(".OFF") == 0)
    {
        read_OFF(filename, coords, tris, faces);
    }
    else if (filetype.compare(".obj") == 0 ||
             filetype.compare(".OBJ") == 0)
    {
        read_OBJ(filename, coords, tris, faces);
    }
    else
    {
        std::cerr << "ERROR : " << __FILE__ << ", line " << __LINE__ << " : load() : file format not supported yet " << endl;
        exit(-1);
    }

    verts = vec3d_from_serialized_xyz(coords);

    logger << num_faces() << " quads read" << endl;
    logger << num_verts() << " verts read" << endl;

    this->mesh_data().filename = std::string(filename);

    timer_stop("Load Quadmesh");
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::save(const char * filename) const
{
    timer_start("Save Quadmesh");

    std::vector<double> coords = serialized_xyz_from_vec3d(verts);
    std::vector<uint>   tris; // unused

    std::string str(filename);
    std::string filetype = str.substr(str.size()-3,3);

    if (filetype.compare("off") == 0 ||
        filetype.compare("OFF") == 0)
    {
        write_OFF(filename, coords, tris, faces);
    }
    else if (filetype.compare(".obj") == 0 ||
             filetype.compare(".OBJ") == 0)
    {
        write_OBJ(filename, coords, tris, faces);
    }
    else
    {
        std::cerr << "ERROR : " << __FILE__ << ", line " << __LINE__ << " : write() : file format not supported yet " << endl;
        exit(-1);
    }

    timer_stop("Save Quadmesh");
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::clear()
{
    bb.reset();
    //
    verts.clear();
    edges.clear();
    faces.clear();
    //
    M std_M_data;
    m_data = std_M_data;
    v_data.clear();
    e_data.clear();
    f_data.clear();
    //
    v2v.clear();
    v2e.clear();
    v2f.clear();
    e2f.clear();
    f2e.clear();
    f2f.clear();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::init()
{
    update_face_tessellation();
    update_adjacency();
    update_bbox();

    v_data.resize(num_verts());
    e_data.resize(num_edges());
    f_data.resize(num_faces());

    update_normals();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::update_face_tessellation()
{
    tessellated_faces.resize(num_faces());

    for(uint fid=0; fid<num_faces(); ++fid)
    {
        uint vid0 = face_vert_id(fid,0);
        uint vid1 = face_vert_id(fid,1);
        uint vid2 = face_vert_id(fid,2);
        uint vid3 = face_vert_id(fid,3);

        vec3d n1 = (vert(vid1)-vert(vid0)).cross(vert(vid2)-vert(vid0));
        vec3d n2 = (vert(vid2)-vert(vid0)).cross(vert(vid3)-vert(vid0));

        bool flip = (n1.dot(n2) < 0); // flip diag: t(0,1,2) t(0,2,3) => t(0,1,3) t(1,2,3)

        tessellated_faces.at(fid).push_back(vid0);
        tessellated_faces.at(fid).push_back(vid1);
        tessellated_faces.at(fid).push_back(flip ? vid3 : vid2);
        tessellated_faces.at(fid).push_back(flip ? vid1 : vid0);
        tessellated_faces.at(fid).push_back(vid2);
        tessellated_faces.at(fid).push_back(vid3);
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::update_adjacency()
{
    timer_start("Build adjacency");

    v2v.clear(); v2v.resize(num_verts());
    v2e.clear(); v2e.resize(num_verts());
    v2f.clear(); v2f.resize(num_verts());
    f2f.clear(); f2f.resize(num_faces());
    f2e.clear(); f2e.resize(num_faces());

    std::map<ipair,std::vector<uint>> e2f_map;
    for(uint fid=0; fid<num_faces(); ++fid)
    {
        for(uint off=0; off<verts_per_face(); ++off)
        {
            uint vid0 = face_vert_id(fid,off);
            uint vid1 = face_vert_id(fid,(off+1)%verts_per_face());
            v2f.at(vid0).push_back(fid);
            e2f_map[unique_pair(vid0,vid1)].push_back(fid);
        }
    }

    edges.clear();
    e2f.clear();
    e2f.resize(e2f_map.size());

    uint fresh_id = 0;
    for(auto e2f_it : e2f_map)
    {
        ipair e    = e2f_it.first;
        uint  eid  = fresh_id++;
        uint  vid0 = e.first;
        uint  vid1 = e.second;

        edges.push_back(vid0);
        edges.push_back(vid1);

        v2v.at(vid0).push_back(vid1);
        v2v.at(vid1).push_back(vid0);

        v2e.at(vid0).push_back(eid);
        v2e.at(vid1).push_back(eid);

        std::vector<uint> fids = e2f_it.second;
        for(uint fid : fids)
        {
            f2e.at(fid).push_back(eid);
            e2f.at(eid).push_back(fid);
            for(uint adj_fid : fids) if (fid != adj_fid) f2f.at(fid).push_back(adj_fid);
        }

        // MANIFOLDNESS CHECKS
        //
        bool is_manifold = (fids.size() > 2 || fids.size() < 1);
        if (is_manifold && !support_non_manifold_edges)
        {
            std::cerr << "Non manifold edge found! To support non manifoldness,";
            std::cerr << "enable the 'support_non_manifold_edges' flag in cinolib.h" << endl;
            assert(false);
        }
        if (is_manifold && print_non_manifold_edges)
        {
            std::cerr << "Non manifold edge! (" << vid0 << "," << vid1 << ")" << endl;
        }
    }

    logger << num_verts() << "\tverts" << endl;
    logger << num_faces() << "\tfaces" << endl;
    logger << num_edges() << "\tedges" << endl;

    timer_stop("Build adjacency");
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::update_bbox()
{
    bb.reset();
    for(uint vid=0; vid<num_verts(); ++vid)
    {
        vec3d v = vert(vid);
        bb.min = bb.min.min(v);
        bb.max = bb.max.max(v);
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
std::vector<double> Quad<M,V,E,F>::vector_coords() const
{
    std::vector<double> coords;
    for(uint vid=0; vid<num_verts(); ++vid)
    {
        coords.push_back(vert(vid).x());
        coords.push_back(vert(vid).y());
        coords.push_back(vert(vid).z());
    }
    return coords;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::update_f_normals()
{
    for(uint fid=0; fid<num_faces(); ++fid)
    {
        // compute the best fitting plane
        std::vector<vec3d> points;
        for(uint off=0; off<verts_per_face(); ++off) points.push_back(face_vert(fid,off));
        Plane best_fit(points);

        // adjust orientation (n or -n?)
        assert(tessellated_faces.at(fid).size()>2);
        vec3d v0 = vert(tessellated_faces.at(fid).at(0));
        vec3d v1 = vert(tessellated_faces.at(fid).at(1));
        vec3d v2 = vert(tessellated_faces.at(fid).at(2));
        vec3d n  = (v1-v0).cross(v2-v0);

        face_data(fid).normal = (best_fit.n.dot(n) < 0) ? -best_fit.n : best_fit.n;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::update_v_normals()
{
    for(uint vid=0; vid<num_verts(); ++vid)
    {
        vec3d n(0,0,0);
        for(uint fid : adj_v2f(vid))
        {
            n += face_data(fid).normal;
        }
        n.normalize();
        vert_data(vid).normal = n;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::update_normals()
{
    update_f_normals();
    update_v_normals();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
uint Quad<M,V,E,F>::face_vert_id(const uint fid, const uint offset) const
{
    uint fid_ptr = fid * verts_per_face();
    return faces.at(fid_ptr + offset);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
vec3d Quad<M,V,E,F>::face_vert(const uint fid, const uint offset) const
{
    return vert(face_vert_id(fid,offset));
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
vec3d Quad<M,V,E,F>::face_centroid(const uint fid) const
{
    vec3d c(0,0,0);
    for(uint off=0; off<verts_per_face(); ++off)
    {
        c += face_vert(fid,off);
    }
    c /= static_cast<double>(verts_per_face());
    return c;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
vec3d Quad<M,V,E,F>::elem_centroid(const uint fid) const
{
    return face_centroid(fid);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
uint Quad<M,V,E,F>::edge_vert_id(const uint eid, const uint offset) const
{
    uint   eid_ptr = eid * 2;
    return edges.at(eid_ptr + offset);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
vec3d Quad<M,V,E,F>::edge_vert(const uint eid, const uint offset) const
{
    return vert(edge_vert_id(eid,offset));
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::elem_show_all()
{
    for(uint fid=0; fid<num_faces(); ++fid)
    {
        face_data(fid).visible = true;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
bool Quad<M,V,E,F>::face_contains_vert(const uint fid, const uint vid) const
{
    for(uint off=0; off<verts_per_face(); ++off)
    {
        if (face_vert_id(fid,off) == vid) return true;
    }
    return false;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
bool Quad<M,V,E,F>::vert_is_singular(const uint vid) const
{
    return (adj_v2v(vid).size()!=4);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::vert_set_color(const Color & c)
{
    for(uint vid=0; vid<num_verts(); ++vid)
    {
        vert_data(vid).color = c;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::vert_set_alpha(const float alpha)
{
    for(uint vid=0; vid<num_verts(); ++vid)
    {
        vert_data(vid).color.a = alpha;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::edge_set_color(const Color & c)
{
    for(uint eid=0; eid<num_edges(); ++eid)
    {
        edge_data(eid).color = c;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::edge_set_alpha(const float alpha)
{
    for(uint eid=0; eid<num_edges(); ++eid)
    {
        edge_data(eid).color.a = alpha;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::face_set_color(const Color & c)
{
    for(uint fid=0; fid<num_faces(); ++fid)
    {
        face_data(fid).color = c;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
void Quad<M,V,E,F>::face_set_alpha(const float alpha)
{
    for(uint fid=0; fid<num_faces(); ++fid)
    {
        face_data(fid).color.a = alpha;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F>
CINO_INLINE
std::vector<float> Quad<M,V,E,F>::export_uvw_param(const int mode) const
{
    std::vector<float> uvw;
    for(uint vid=0; vid<num_verts(); ++vid)
    {
        switch (mode)
        {
            case U_param  : uvw.push_back(vert_data(vid).uvw[0]); break;
            case V_param  : uvw.push_back(vert_data(vid).uvw[1]); break;
            case W_param  : uvw.push_back(vert_data(vid).uvw[2]); break;
            case UV_param : uvw.push_back(vert_data(vid).uvw[0]);
                            uvw.push_back(vert_data(vid).uvw[1]); break;
            case UW_param : uvw.push_back(vert_data(vid).uvw[0]);
                            uvw.push_back(vert_data(vid).uvw[2]); break;
            case VW_param : uvw.push_back(vert_data(vid).uvw[1]);
                            uvw.push_back(vert_data(vid).uvw[2]); break;
            case UVW_param: uvw.push_back(vert_data(vid).uvw[0]);
                            uvw.push_back(vert_data(vid).uvw[1]);
                            uvw.push_back(vert_data(vid).uvw[2]); break;
            default: assert(false);
        }
    }
    return uvw;
}


}