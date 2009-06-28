/*  TA3D, a remake of Total Annihilation
    Copyright (C) 2005  Roland BROCHARD

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA*/

/*-----------------------------------------------------------------------------------\
|                                         3dm.cpp                                    |
|  ce fichier contient les structures, classes et fonctions nécessaires à la lecture |
| des fichiers 3dm de TA3D qui sont les fichiers contenant les modèles 3D des objets |
| du jeu.                                                                            |
|                                                                                    |
\-----------------------------------------------------------------------------------*/

#include "../stdafx.h"
#include "../misc/matrix.h"
#include "../TA3D_NameSpace.h"
#include "../ta3dbase.h"
#include "3dm.h"
#include "../gfx/particles/particles.h"
#include "../ingame/sidedata.h"
#include "../languages/i18n.h"
#include "../misc/math.h"
#include "../misc/paths.h"
#include "../misc/files.h"
#include "../logs/logs.h"
#include "../gfx/gl.extensions.h"
#include <zlib.h>


namespace TA3D
{
    void MESH_3DM::init3DM()
    {
        init();
        Color = 0;
        RColor = 0;
        Flag = 0;
        frag_shader_src.clear();
        vert_shader_src.clear();
        s_shader.destroy();
    }



    void MESH_3DM::destroy3DM()
    {
        destroy();
        Color = 0;
        RColor = 0;
        Flag = 0;
        frag_shader_src.clear();
        vert_shader_src.clear();
        s_shader.destroy();
    }

    bool MESH_3DM::draw(float t, ANIMATION_DATA *data_s, bool sel_primitive, bool alset, bool notex, int side, bool chg_col, bool exploding_parts)
    {
        bool explodes = script_index >= 0 && data_s && (data_s->flag[script_index] & FLAG_EXPLODE);
        bool hide = false;
        bool set = false;
        float color_factor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        if ( !tex_cache_name.empty() )
        {
            for(int i = 0 ; i < tex_cache_name.size() ; ++i)
                load_texture_id(i);
            tex_cache_name.clear();
        }

        if (!(explodes && !exploding_parts))
        {
            glPushMatrix();

            glTranslatef(pos_from_parent.x, pos_from_parent.y, pos_from_parent.z);
            if (script_index >= 0 && data_s)
            {
                if (!explodes ^ exploding_parts)
                {
                    glTranslatef(data_s->axe[0][script_index].pos, data_s->axe[1][script_index].pos, data_s->axe[2][script_index].pos);
                    glRotatef(data_s->axe[0][script_index].angle, 1.0f, 0.0f, 0.0f);
                    glRotatef(data_s->axe[1][script_index].angle, 0.0f, 1.0f, 0.0f);
                    glRotatef(data_s->axe[2][script_index].angle, 0.0f, 0.0f, 1.0f);
                }
                hide = data_s->flag[script_index] & FLAG_HIDE;
            }
            else if (animation_data && data_s == NULL)
            {
                Vector3D R;
                Vector3D T;
                animation_data->animate(t, R, T);
                glTranslatef(T.x, T.y, T.z);
                glRotatef(R.x, 1.0f, 0.0f, 0.0f);
                glRotatef(R.y, 0.0f, 1.0f, 0.0f);
                glRotatef(R.z, 0.0f, 0.0f, 1.0f);
            }

            hide |= explodes ^ exploding_parts;
            if (chg_col)
                glGetFloatv(GL_CURRENT_COLOR, color_factor);
            int texID = player_color_map[side];
            if (script_index >= 0 && data_s && (data_s->flag[script_index] & FLAG_ANIMATED_TEXTURE)
                && !fixed_textures && !gltex.empty())
                texID = ((int)(t * 10.0f)) % gltex.size();
            if (gl_dlist.size() > texID && gl_dlist[texID] && !hide && !chg_col && !notex && false)
            {
                glCallList( gl_dlist[ texID ] );
                alset = false;
                set = false;
            }
            else if (!hide)
            {
                bool creating_list = false;
                if (gl_dlist.size() <= texID)
                    gl_dlist.resize(texID + 1);
                if (!chg_col && !notex && gl_dlist[texID] == 0 && false)
                {
                    gl_dlist[texID] = glGenLists(1);
                    glNewList(gl_dlist[texID], GL_COMPILE_AND_EXECUTE);
                    alset = false;
                    set = false;
                    creating_list = true;
                }
                if (nb_t_index > 0 && nb_vtx > 0 && t_index != NULL)
                {
                    bool activated_tex = false;
                    glEnableClientState(GL_VERTEX_ARRAY);		// Les sommets
                    glEnableClientState(GL_NORMAL_ARRAY);
                    alset = false;
                    set = false;
                    if (!chg_col || !notex)
                    {
                        if (Flag&SURFACE_PLAYER_COLOR)
                            glColor4f(player_color[side * 3], player_color[side * 3 + 1], player_color[side * 3 + 2], (Color & 0xFF) / 255.0f);		// Couleur de matière
                        else
                            glColor4ubv((GLubyte*)&Color);		// Couleur de matière
                    }
                    else if (chg_col && notex)
                    {
                        if (Flag & SURFACE_PLAYER_COLOR)
                            glColor4f(player_color[player_color_map[side] * 3] * color_factor[0],
                                      player_color[player_color_map[side] * 3 + 1] * color_factor[1],
                                      player_color[player_color_map[side] * 3 + 2] * color_factor[2],
                                      geta32(Color) / 255.0f * color_factor[3]);		// Couleur de matière
                        else
                            glColor4f(getr32(Color) / 255.0f * color_factor[0],
                                      getg32(Color) / 255.0f * color_factor[1],
                                      getb32(Color) / 255.0f * color_factor[2],
                                      geta32(Color) / 255.0f * color_factor[3]);		// Couleur de matière
                    }

                    if (Flag & SURFACE_GLSL)			// Using vertex and fragment programs
                    {
                        s_shader.on();
                        for (int j = 0; j < gltex.size() ; ++j)
                            s_shader.setvar1i( String::Format("tex%d",j).c_str(), j );
                    }

                    if (Flag&SURFACE_GOURAUD)			// Type d'éclairage
                        glShadeModel (GL_SMOOTH);
                    else
                        glShadeModel (GL_FLAT);

                    if (Flag&SURFACE_LIGHTED)			// Eclairage
                        glEnable(GL_LIGHTING);
                    else
                        glDisable(GL_LIGHTING);

                    if (chg_col || !notex)
                    {
                        if (Flag&SURFACE_BLENDED || (chg_col && color_factor[3] != 1.0f)) // La transparence
                        {
                            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
                            glEnable(GL_BLEND);
                            glAlphaFunc( GL_GREATER, 0.1 );
                            glEnable( GL_ALPHA_TEST );
                        }
                        else
                        {
                            glDisable(GL_ALPHA_TEST);
                            glDisable(GL_BLEND);
                        }
                    }

                    if ((Flag&SURFACE_TEXTURED) && !notex) // Les textures et effets de texture
                    {
                        activated_tex = true;
                        for (int j = 0; j < gltex.size() ; ++j)
                        {
                            glActiveTextureARB(GL_TEXTURE0_ARB + j);
                            glEnable(GL_TEXTURE_2D);
                            glBindTexture(GL_TEXTURE_2D, gltex[j]);
                            if (j == gltex.size() - 1 && Flag&SURFACE_REFLEC)
                            {
                                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                                glEnable(GL_TEXTURE_GEN_S);
                                glEnable(GL_TEXTURE_GEN_T);
                                glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, TA3D_GL_COMBINE_EXT);
                                glTexEnvi(GL_TEXTURE_ENV,TA3D_GL_COMBINE_RGB_EXT,GL_INTERPOLATE);

                                glTexEnvi(GL_TEXTURE_ENV,TA3D_GL_SOURCE0_RGB_EXT,GL_TEXTURE);
                                glTexEnvi(GL_TEXTURE_ENV,TA3D_GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
                                glTexEnvi(GL_TEXTURE_ENV,TA3D_GL_SOURCE1_RGB_EXT,TA3D_GL_PREVIOUS_EXT);
                                glTexEnvi(GL_TEXTURE_ENV,TA3D_GL_OPERAND1_RGB_EXT,GL_SRC_COLOR);
                                glTexEnvi(GL_TEXTURE_ENV,TA3D_GL_SOURCE2_RGB_EXT,TA3D_GL_CONSTANT_EXT);
                                glTexEnvi(GL_TEXTURE_ENV,TA3D_GL_OPERAND2_RGB_EXT, GL_SRC_COLOR);
                                float RColorf[4] = { getr32(RColor) / 255.0f,
                                                     getg32(RColor) / 255.0f,
                                                     getb32(RColor) / 255.0f,
                                                     geta32(RColor) / 255.0f};
                                glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR, RColorf);
                            }
                            else
                            {
                                glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
                                glDisable(GL_TEXTURE_GEN_S);
                                glDisable(GL_TEXTURE_GEN_T);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                            }
                        }
                        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                        for (int j = 0; j < gltex.size() ; ++j)
                        {
                            glClientActiveTextureARB(GL_TEXTURE0_ARB + j);
                            glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
                        }
                    }
                    else
                    {
                        for (int j = 6; j >= 0; --j)
                        {
                            glActiveTextureARB(GL_TEXTURE0_ARB + j);
                            glDisable(GL_TEXTURE_2D);
                            glClientActiveTextureARB(GL_TEXTURE0_ARB + j);
                            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                        }
                        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                        glEnable(GL_TEXTURE_2D);
                        glBindTexture(GL_TEXTURE_2D, gfx->default_texture);
                    }
                    glVertexPointer(3, GL_FLOAT, 0, points);
                    glNormalPointer(GL_FLOAT, 0, N);
                    switch(type)
                    {
                    case MESH_TYPE_TRIANGLES:
                        glDrawRangeElements(GL_TRIANGLES, 0, nb_vtx - 1, nb_t_index, GL_UNSIGNED_SHORT, t_index);				// draw everything
                        break;
                    case MESH_TYPE_TRIANGLE_STRIP:
                        glDisable( GL_CULL_FACE );
                        glDrawRangeElements(GL_TRIANGLE_STRIP, 0, nb_vtx - 1, nb_t_index, GL_UNSIGNED_SHORT, t_index);		// draw everything
                        glEnable( GL_CULL_FACE );
                        break;
                    };

                    if ((Flag&(SURFACE_ADVANCED | SURFACE_GOURAUD)) == SURFACE_ADVANCED)
                        glShadeModel (GL_SMOOTH);
                    if ((Flag&SURFACE_GLSL) && (Flag&SURFACE_ADVANCED))			// Using vertex and fragment programs
                        s_shader.off();

                    if (activated_tex)
                    {
                        for (int j = 0; j < gltex.size() ; ++j)
                        {
                            glClientActiveTextureARB(GL_TEXTURE0_ARB + j);
                            glDisableClientState(GL_TEXTURE_COORD_ARRAY);

                            glActiveTextureARB(GL_TEXTURE0_ARB + j);
                            glDisable(GL_TEXTURE_2D);
                            glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
                            glDisable(GL_TEXTURE_GEN_S);
                            glDisable(GL_TEXTURE_GEN_T);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        }
                        glClientActiveTextureARB(GL_TEXTURE0_ARB);
                        glActiveTextureARB(GL_TEXTURE0_ARB);
                        glEnable(GL_TEXTURE_2D);
                    }
                }
                if (creating_list)
                    glEndList();
            }
            if (chg_col)
                glColor4fv(color_factor);
            if (child && !(explodes && !exploding_parts))
                alset = child->draw(t, data_s, sel_primitive, alset, notex, side, chg_col, exploding_parts && !explodes );
            glPopMatrix();
        }
        if (next)
            alset = next->draw(t, data_s, sel_primitive, alset, notex, side, chg_col, exploding_parts);

        return alset;
    }

    inline byte* read_from_mem(void* buf, const int len, byte* data)
    {
        memcpy(buf, data, len);
        return data + len;
    }

    byte *MESH_3DM::load(byte *data, const String &filename)
    {
        destroy();

        if (data == NULL)
            return NULL;

        uint8 len = data[0];
        ++data;
        char tmp[256];
        data = read_from_mem(tmp, len, data);
        name = String(tmp, len);

        data = read_from_mem(&pos_from_parent.x, sizeof(pos_from_parent.x), data);
        if (isNaN(pos_from_parent.x))           // Some error checks
        {
            name.clear();
            return NULL;
        }

        data = read_from_mem(&pos_from_parent.y, sizeof(pos_from_parent.y), data);
        if (isNaN(pos_from_parent.y))           // Some error checks
        {
            name.clear();
            return NULL;
        }

        data=read_from_mem(&pos_from_parent.z,sizeof(pos_from_parent.z),data);
        if (isNaN(pos_from_parent.z))
        {
            name.clear();
            return NULL;
        }

        data = read_from_mem(&nb_vtx, sizeof(nb_vtx), data);
        if (nb_vtx < 0)
        {
            name.clear();
            return NULL;
        }
        if (nb_vtx > 0)
        {
            points = new Vector3D[nb_vtx<<1];
            data = read_from_mem(points,sizeof(Vector3D)*nb_vtx,data);
        }
        else
            points = NULL;

        data = read_from_mem(sel, sizeof(GLushort) * 4, data);

        data = read_from_mem(&nb_p_index, sizeof(nb_p_index), data); // Read point data
        if (nb_p_index < 0)
        {
            if (points)
                delete[] points;
            name.clear();
            init();
            return NULL;
        }
        if (nb_p_index > 0)
        {
            p_index = new GLushort[nb_p_index];
            data=read_from_mem(p_index,sizeof(GLushort)*nb_p_index,data);
        }
        else
            p_index=NULL;

        data=read_from_mem(&nb_l_index,sizeof(nb_l_index),data);	// Read line data
        if (nb_l_index < 0)
        {
            if (points) delete[] points;
            if (p_index) delete[] p_index;
            name.clear();
            init();
            return NULL;
        }
        if (nb_l_index > 0)
        {
            l_index = new GLushort[nb_l_index];
            data=read_from_mem(l_index,sizeof(GLushort)*nb_l_index,data);
        }
        else
            l_index=NULL;

        data = read_from_mem(&nb_t_index, sizeof(nb_t_index), data); // Read triangle data
        if (nb_t_index < 0)
        {
            if (points) delete[] points;
            if (p_index) delete[] p_index;
            if (l_index) delete[] l_index;
            name.clear();
            init();
            return NULL;
        }
        if (nb_t_index > 0)
        {
            t_index = new GLushort[nb_t_index];
            data=read_from_mem(t_index,sizeof(GLushort)*nb_t_index,data);
        }
        else
            t_index=NULL;

        tcoord = new float[nb_vtx<<1];
        data = read_from_mem(tcoord,sizeof(float)*nb_vtx<<1,data);

        float Colorf[4];
        float RColorf[4];
        data = read_from_mem(Colorf, sizeof(float) * 4, data);	// Read surface data
        data = read_from_mem(RColorf, sizeof(float) * 4, data);
        Color = makeacol32((int)(Colorf[0] * 255), (int)(Colorf[1] * 255), (int)(Colorf[2] * 255), (int)(Colorf[3] * 255));
        RColor = makeacol32((int)(RColorf[0] * 255), (int)(RColorf[1] * 255), (int)(RColorf[2] * 255), (int)(RColorf[3] * 255));
        data = read_from_mem(&Flag, sizeof(Flag), data);
        Flag |= SURFACE_ADVANCED;           // a 3DM cannot use 3DO specific stuffs
        sint8 NbTex = 0;
        data = read_from_mem(&NbTex,sizeof(NbTex),data);
        bool compressed = NbTex < 0;
        NbTex = abs( NbTex );
        gltex.resize(NbTex);
        for (uint8 i = 0; i < NbTex; ++i)
        {
            SDL_Surface *tex;
            if (!compressed)
            {
                int tex_w;
                int tex_h;
                data = read_from_mem(&tex_w, sizeof(tex_w), data);
                data = read_from_mem(&tex_h, sizeof(tex_h), data);

                tex = gfx->create_surface_ex(32, tex_w, tex_h);
                if (tex == NULL)
                {
                    destroy();
                    return NULL;
                }
                try
                {
                    for (int y = 0; y < tex->h; ++y)
                        for (int x = 0; x < tex->w; ++x)
                            data = read_from_mem(&SurfaceInt(tex, x, y), 4, data);
                }
                catch(...)
                {
                    destroy();
                    return NULL;
                }
            }
            else
            {
                int img_size = 0;
                uint8 bpp;
                int w, h;
                data = read_from_mem(&w,sizeof(w),data);
                data = read_from_mem(&h,sizeof(h),data);
                data = read_from_mem(&bpp,sizeof(bpp),data);
                data = read_from_mem(&img_size,sizeof(img_size),data);	// Read RGBA data
                byte *buffer = new byte[ img_size ];

                try
                {
                    data = read_from_mem( buffer, img_size, data );

                    tex = gfx->create_surface_ex( bpp, w, h );
                    uLongf len = tex->w * tex->h * tex->format->BytesPerPixel;
                    uncompress ( (Bytef*) tex->pixels, &len, (Bytef*) buffer, img_size);
                }
                catch( ... )
                {
                    delete[] buffer;
                    destroy();
                    return NULL;
                }

                delete[] buffer;
            }

            String cache_filename = !filename.empty() ? filename + String::Format("-%s-%d.bin", !name.empty() ? name.c_str() : "none", i ) : String( "" );
            cache_filename.replace('/', 'S');
            cache_filename.replace('\\', 'S');

            gltex[i] = 0;
            if (!gfx->is_texture_in_cache(cache_filename))
            {
                cache_filename = TA3D::Paths::Files::ReplaceExtension( cache_filename, ".tex" );
                if (!TA3D::Paths::Exists( TA3D::Paths::Caches + cache_filename ))
                    SaveTex( tex, TA3D::Paths::Caches + cache_filename );
            }
            tex_cache_name.push_back( cache_filename );

            SDL_FreeSurface(tex);
        }

        if (Flag & SURFACE_GLSL) // Fragment & Vertex shaders
        {
            uint32 shader_size;
            data = read_from_mem(&shader_size,4,data);
            char *buf = new char[shader_size+1];
            buf[shader_size]=0;
            data=read_from_mem(buf,shader_size,data);
            vert_shader_src = buf;
            delete[] buf;

            data = read_from_mem(&shader_size,4,data);
            buf = new char[shader_size+1];
            buf[shader_size]=0;
            data = read_from_mem(buf,shader_size,data);
            frag_shader_src = buf;
            delete[] buf;
            s_shader.load_memory(frag_shader_src.c_str(),frag_shader_src.size(),vert_shader_src.c_str(),vert_shader_src.size());
        }

        N = new Vector3D[nb_vtx << 1]; // Calculate normals
        if (nb_t_index>0 && t_index != NULL)
        {
            F_N = new Vector3D[nb_t_index / 3];
            for (int i = 0; i < nb_vtx; ++i)
                N[i].x=N[i].z=N[i].y=0.0f;
            int e = 0;
            for (int i=0;i<nb_t_index;i+=3)
            {
                Vector3D AB,AC,Normal;
                AB = points[t_index[i+1]] - points[t_index[i]];
                AC = points[t_index[i+2]] - points[t_index[i]];
                Normal = AB * AC;
                Normal.unit();
                F_N[e++] = Normal;
                for (byte e = 0; e < 3; ++e)
                    N[t_index[i + e]] = N[t_index[i + e]] + Normal;
            }
            for (int i = 0; i < nb_vtx; ++i)
                N[i].unit();
        }

        byte link;
        data=read_from_mem(&link,1,data);

        if (link == 2) // Load animation data if present
        {
            animation_data = new ANIMATION;
            data = read_from_mem( &(animation_data->type), 1, data );
            data = read_from_mem( &(animation_data->angle_0), sizeof(Vector3D), data);
            data = read_from_mem( &(animation_data->angle_1), sizeof(Vector3D), data);
            data = read_from_mem( &(animation_data->angle_w), sizeof(float), data );
            data = read_from_mem( &(animation_data->translate_0), sizeof(Vector3D), data);
            data = read_from_mem( &(animation_data->translate_1), sizeof(Vector3D), data);
            data = read_from_mem( &(animation_data->translate_w), sizeof(float), data);

            data = read_from_mem(&link,1,data);
        }

        if (link)
        {
            MESH_3DM *pChild = new MESH_3DM;
            child = pChild;
            data = pChild->load(data,filename);
            if (data == NULL)
            {
                destroy();
                return NULL;
            }
        }
        else
            child = NULL;

        data = read_from_mem(&link,1,data);
        if (link)
        {
            MESH_3DM *pNext = new MESH_3DM;
            next = pNext;
            data = pNext->load(data, filename);
            if (data == NULL)
            {
                destroy();
                return NULL;
            }
        }
        else
            next = NULL;
        return data;
    }

    MODEL *MESH_3DM::load(const String &filename)
    {
        uint32 file_length(0);
        byte *data = VFS::instance()->readFile(filename, &file_length);
        if (!data)
        {
            LOG_ERROR(LOG_PREFIX_3DM << "could not read file '" << filename << "'");
            return NULL;
        }

        if (data[0] == 0)       // This is a pointer file
        {
            String realFilename = (char*)data + 1;
            realFilename.trim();
            LOG_INFO(LOG_PREFIX_3DM << "file '" << filename << "' points to '" << realFilename << "'");
            delete[] data;
            data = VFS::instance()->readFile(realFilename, &file_length);
            if (!data)
            {
                LOG_ERROR(LOG_PREFIX_3DM << "could not read file '" << realFilename << "'");
                return NULL;
            }
        }

        MESH_3DM *mesh = new MESH_3DM;
        mesh->load(data, filename);
        delete[] data;

        MODEL *model = new MODEL;
        model->mesh = mesh;
        model->postLoadComputations();
        return model;
    }
} // namespace TA3D
