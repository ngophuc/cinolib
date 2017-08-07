/*********************************************************************************
*  Copyright(C) 2016: Marco Livesu                                               *
*  All rights reserved.                                                          *
*                                                                                *
*  This file is part of CinoLib                                                  *
*                                                                                *
*  CinoLib is dual-licensed:                                                     *
*                                                                                *
*   - For non-commercial use you can redistribute it and/or modify it under the  *
*     terms of the GNU General Public License as published by the Free Software  *
*     Foundation; either version 3 of the License, or (at your option) any later *
*     version.                                                                   *
*                                                                                *
*   - If you wish to use it as part of a commercial software, a proper agreement *
*     with the Author(s) must be reached, based on a proper licensing contract.  *
*                                                                                *
*  This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE       *
*  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.     *
*                                                                                *
*  Author(s):                                                                    *
*                                                                                *
*     Marco Livesu (marco.livesu@gmail.com)                                      *
*     http://pers.ge.imati.cnr.it/livesu/                                        *
*                                                                                *
*     Italian National Research Council (CNR)                                    *
*     Institute for Applied Mathematics and Information Technologies (IMATI)     *
*     Via de Marini, 6                                                           *
*     16149 Genoa,                                                               *
*     Italy                                                                      *
**********************************************************************************/
#ifndef CINO_HEXMESH_H
#define CINO_HEXMESH_H

#include <sys/types.h>
#include <vector>

#include <cinolib/cinolib.h>
#include <cinolib/bbox.h>
#include <cinolib/geometry/vec3.h>
#include <cinolib/meshes/quadmesh.h>
#include <cinolib/meshes/mesh_attributes.h>
#include <cinolib/meshes/abstract_volume_mesh.h>
#include <cinolib/standard_elements_tables.h>
#include <cinolib/hexmesh_split_schemas.h>

namespace cinolib
{

template<class M = Mesh_min_attributes, // default template arguments
         class V = Vert_min_attributes,
         class E = Edge_min_attributes,
         class F = Polygon_min_attributes,
         class P = Polyhedron_min_attributes>
class Hexmesh : public AbstractPolyhedralMesh<M,V,E,F,P>
{
    public:

        Hexmesh(){}

        Hexmesh(const char * filename);

        Hexmesh(const std::vector<double> & coords,
                const std::vector<uint>   & polys);

        Hexmesh(const std::vector<vec3d> & verts,
                const std::vector<uint>  & polys);

        Hexmesh(const std::vector<vec3d>             & verts,
                const std::vector<std::vector<uint>> & polys);

        Hexmesh(const std::vector<vec3d>             & verts,
                const std::vector<std::vector<uint>> & faces,
                const std::vector<std::vector<uint>> & polys,
                const std::vector<std::vector<bool>> & polys_face_winding);

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        MeshType mesh_type() const { return HEXMESH; }

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        void init();
        void load(const char * filename);
        void save(const char * filename) const;

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        void update_normals();
        void update_hex_quality(const uint cid);
        void update_hex_quality();
        void print_quality(const bool list_folded_elements = false);

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        uint verts_per_poly(const uint) const { return  8; }
        uint edges_per_poly(const uint) const { return 12; }
        uint faces_per_poly(const uint) const { return  6; }
        uint verts_per_face(const uint) const { return  4; }

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        Quadmesh<M,V,E,F> export_surface() const;
        Quadmesh<M,V,E,F> export_surface(std::map<uint,uint> & c2f_map,
                                         std::map<uint,uint> & f2c_map) const;

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        vec3d verts_average(const std::vector<uint> & vids) const;

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        std::vector<uint> face_tessellation(const uint fid) const;

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

        void   poly_subdivide(const std::vector<std::vector<std::vector<uint>>> & split_scheme);
        double poly_volume   (const uint) const { assert(false && "TODO!"); return 1.0; }
};

}

#ifndef  CINO_STATIC_LIB
#include "hexmesh.cpp"
#endif

#endif // CINO_HEXMESH_H