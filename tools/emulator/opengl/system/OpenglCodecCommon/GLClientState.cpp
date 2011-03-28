/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "GLClientState.h"
#include "ErrorLog.h"
#include <stdio.h>
#include <stdlib.h>
#include "glUtils.h"
#include <cutils/log.h>

GLClientState::GLClientState(int nLocations)
{
    if (nLocations < LAST_LOCATION) {
        nLocations = LAST_LOCATION;
    }
    m_nLocations = nLocations;
    m_states = new VertexAttribState[m_nLocations];
    for (int i = 0; i < m_nLocations; i++) {
        m_states[i].enabled = 0;
        m_states[i].enableDirty = false;
    }
    m_currentArrayVbo = 0;
    m_currentIndexVbo = 0;
    // init gl constans;
    m_states[VERTEX_LOCATION].glConst = GL_VERTEX_ARRAY;
    m_states[NORMAL_LOCATION].glConst = GL_NORMAL_ARRAY;
    m_states[COLOR_LOCATION].glConst = GL_COLOR_ARRAY;
    m_states[POINTSIZE_LOCATION].glConst = GL_POINT_SIZE_ARRAY_OES;
    m_states[TEXCOORD0_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD1_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD2_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD3_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD4_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD5_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD6_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD7_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_activeTexture = 0;

    m_pixelStore.unpack_alignment = 4;
    m_pixelStore.pack_alignment = 4;
}

GLClientState::~GLClientState()
{
    delete m_states;
}

void GLClientState::enable(int location, int state)
{
    if (!validLocation(location)) {
        return;
    }

    m_states[location].enableDirty |= (state != m_states[location].enabled);
    m_states[location].enabled = state;
}

void GLClientState::setState(int location, int size, GLenum type, GLsizei stride, void *data)
{
    if (!validLocation(location)) {
        return;
    }
    m_states[location].size = size;
    m_states[location].type = type;
    m_states[location].stride = stride;
    m_states[location].data = data;
    m_states[location].bufferObject = m_currentArrayVbo;
    m_states[location].elementSize = glSizeof(type) * size;
}

void GLClientState::setBufferObject(int location, GLuint id)
{
    if (!validLocation(location)) {
        return;
    }

    m_states[location].bufferObject = id;
}

const GLClientState::VertexAttribState * GLClientState::getState(int location)
{
    if (!validLocation(location)) {
        return NULL;
    }
    return & m_states[location];
}

const GLClientState::VertexAttribState * GLClientState::getStateAndEnableDirty(int location, bool *enableChanged)
{
    if (!validLocation(location)) {
        return NULL;
    }

    if (enableChanged) {
        *enableChanged = m_states[location].enableDirty;
    }

    m_states[location].enableDirty = false;
    return & m_states[location];
}

int GLClientState::getLocation(GLenum loc)
{
    int retval;

    switch(loc) {
    case GL_VERTEX_ARRAY:
        retval = int(VERTEX_LOCATION);
        break;
    case GL_NORMAL_ARRAY:
        retval = int(NORMAL_LOCATION);
        break;
    case GL_COLOR_ARRAY:
        retval = int(COLOR_LOCATION);
        break;
    case GL_POINT_SIZE_ARRAY_OES:
        retval = int(POINTSIZE_LOCATION);
        break;
    case GL_TEXTURE_COORD_ARRAY:
        retval = int (TEXCOORD0_LOCATION + m_activeTexture);
        break;
    default:
        retval = loc;
    }
    return retval;
}

int GLClientState::setPixelStore(GLenum param, GLint value)
{
    int retval = 0;
    switch(param) {
    case GL_UNPACK_ALIGNMENT:
        if (value == 1 || value == 2 || value == 4 || value == 8) {
            m_pixelStore.unpack_alignment = value;
        } else {
            retval =  GL_INVALID_VALUE;
        }
        break;
    case GL_PACK_ALIGNMENT:
        if (value == 1 || value == 2 || value == 4 || value == 8) {
            m_pixelStore.pack_alignment = value;
        } else {
            retval =  GL_INVALID_VALUE;
        }
        break;
        default:
            retval = GL_INVALID_ENUM;
    }
    return retval;
}


size_t GLClientState::pixelDataSize(GLsizei width, GLsizei height, GLenum format, GLenum type, int pack)
{

    int components = 0;
    int componentsize = 0;
    int pixelsize = 0;
    switch(type) {
    case GL_UNSIGNED_BYTE:
        componentsize = 1;
        break;
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
        pixelsize = 2;
        break;
    default:
        ERR("pixelDataSize: unknown pixel type - assuming pixel data 0\n");
        componentsize = 0;
    }

    if (pixelsize == 0) {
        switch(format) {
#if 0
        case GL_RED:
        case GL_GREEN:
        case GL_BLUE:
#endif
        case GL_ALPHA:
        case GL_LUMINANCE:
            components = 1;
            break;
        case GL_LUMINANCE_ALPHA:
            components = 2;
            break;
        case GL_RGB:
#if 0
        case GL_BGR:
#endif
            components = 3;
            break;
        case GL_RGBA:
#if 0
        case GL_BGRA:
#endif
            components = 4;
            break;
        default:
            ERR("pixelDataSize: unknown pixel format...\n");
            components = 0;
        }
        pixelsize = components * componentsize;
    }

    int alignment = pack ? m_pixelStore.pack_alignment : m_pixelStore.unpack_alignment;

    if (pixelsize == 0 ) {
        ERR("unknown pixel size: width: %d height: %d format: %d type: %d pack: %d align: %d\n",
             width, height, format, type, pack, alignment);
    }
    size_t linesize = pixelsize * width;
    size_t aligned_linesize = int(linesize / alignment) * alignment;
    if (aligned_linesize < linesize) {
        aligned_linesize += alignment;
    }
    return aligned_linesize * height;
}

