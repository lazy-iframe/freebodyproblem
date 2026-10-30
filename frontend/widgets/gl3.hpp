/**************************************************************************
 * Copyright (C) 2026  Azhar Tanweer
 * Contact: azhar.tanweer404@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************/


// frontend/widgets/gl3.hpp
//
// The OpenGL 3.3 entry points the 3D map draws with. Everything else in the
// app goes through ImGui or sticks to OpenGL 1.1, which every platform's
// <GL/gl.h> exports directly; shaders, buffers and framebuffers are not, and
// have to be looked up at run time. That is all this does, through GLFW,
// for the few dozen functions needed — less than a generated loader.
//
// Names live in namespace gl3 and drop the gl/GL_ prefix, so they cannot
// collide with a system <GL/glext.h> that declares the same functions.

#pragma once

// Windows' <GL/gl.h> needs the WINGDIAPI/APIENTRY macros from <windows.h>.
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

#include <cstddef>

#ifdef _WIN32
#  define GL3_API __stdcall
#else
#  define GL3_API
#endif

namespace gl3 {

using Char       = char;
using Sizeiptr   = std::ptrdiff_t;

constexpr GLenum FRAGMENT_SHADER      = 0x8B30;
constexpr GLenum VERTEX_SHADER        = 0x8B31;
constexpr GLenum COMPILE_STATUS       = 0x8B81;
constexpr GLenum LINK_STATUS          = 0x8B82;
constexpr GLenum ARRAY_BUFFER         = 0x8892;
constexpr GLenum ELEMENT_ARRAY_BUFFER = 0x8893;
constexpr GLenum STATIC_DRAW          = 0x88E4;
constexpr GLenum DYNAMIC_DRAW         = 0x88E8;
constexpr GLenum FRAMEBUFFER          = 0x8D40;
constexpr GLenum RENDERBUFFER         = 0x8D41;
constexpr GLenum COLOR_ATTACHMENT0    = 0x8CE0;
constexpr GLenum DEPTH_ATTACHMENT     = 0x8D00;
constexpr GLenum DEPTH_COMPONENT24    = 0x81A6;
constexpr GLenum FRAMEBUFFER_COMPLETE = 0x8CD5;
constexpr GLenum FRAMEBUFFER_BINDING  = 0x8CA6;
constexpr GLenum VERTEX_ARRAY_BINDING = 0x85B5;
constexpr GLenum CURRENT_PROGRAM      = 0x8B8D;
constexpr GLenum ACTIVE_TEXTURE       = 0x84E0;
constexpr GLenum TEXTURE0             = 0x84C0;
constexpr GLenum R32F                 = 0x822E;
constexpr GLenum RED                  = 0x1903;
constexpr GLenum CLAMP_TO_EDGE        = 0x812F;

#define GL3_FUNCS(X)                                                                          \
    X(GLuint, CreateShader,            (GLenum))                                              \
    X(void,   ShaderSource,            (GLuint, GLsizei, const Char* const*, const GLint*))   \
    X(void,   CompileShader,           (GLuint))                                              \
    X(void,   GetShaderiv,             (GLuint, GLenum, GLint*))                              \
    X(void,   GetShaderInfoLog,        (GLuint, GLsizei, GLsizei*, Char*))                    \
    X(void,   DeleteShader,            (GLuint))                                              \
    X(GLuint, CreateProgram,           ())                                                    \
    X(void,   AttachShader,            (GLuint, GLuint))                                      \
    X(void,   LinkProgram,             (GLuint))                                              \
    X(void,   GetProgramiv,            (GLuint, GLenum, GLint*))                              \
    X(void,   GetProgramInfoLog,       (GLuint, GLsizei, GLsizei*, Char*))                    \
    X(void,   DeleteProgram,           (GLuint))                                              \
    X(void,   UseProgram,              (GLuint))                                              \
    X(GLint,  GetUniformLocation,      (GLuint, const Char*))                                 \
    X(void,   Uniform1i,               (GLint, GLint))                                        \
    X(void,   Uniform1f,               (GLint, GLfloat))                                      \
    X(void,   Uniform2f,               (GLint, GLfloat, GLfloat))                             \
    X(void,   Uniform3f,               (GLint, GLfloat, GLfloat, GLfloat))                    \
    X(void,   Uniform4f,               (GLint, GLfloat, GLfloat, GLfloat, GLfloat))           \
    X(void,   UniformMatrix4fv,        (GLint, GLsizei, GLboolean, const GLfloat*))           \
    X(void,   GenVertexArrays,         (GLsizei, GLuint*))                                    \
    X(void,   BindVertexArray,         (GLuint))                                              \
    X(void,   DeleteVertexArrays,      (GLsizei, const GLuint*))                              \
    X(void,   GenBuffers,              (GLsizei, GLuint*))                                    \
    X(void,   BindBuffer,              (GLenum, GLuint))                                      \
    X(void,   BufferData,              (GLenum, Sizeiptr, const void*, GLenum))               \
    X(void,   DeleteBuffers,           (GLsizei, const GLuint*))                              \
    X(void,   VertexAttribPointer,     (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)) \
    X(void,   EnableVertexAttribArray, (GLuint))                                              \
    X(void,   ActiveTexture,           (GLenum))                                              \
    X(void,   GenFramebuffers,         (GLsizei, GLuint*))                                    \
    X(void,   BindFramebuffer,         (GLenum, GLuint))                                      \
    X(void,   FramebufferTexture2D,    (GLenum, GLenum, GLenum, GLuint, GLint))               \
    X(GLenum, CheckFramebufferStatus,  (GLenum))                                              \
    X(void,   DeleteFramebuffers,      (GLsizei, const GLuint*))                              \
    X(void,   GenRenderbuffers,        (GLsizei, GLuint*))                                    \
    X(void,   BindRenderbuffer,        (GLenum, GLuint))                                      \
    X(void,   RenderbufferStorage,     (GLenum, GLenum, GLsizei, GLsizei))                    \
    X(void,   FramebufferRenderbuffer, (GLenum, GLenum, GLenum, GLuint))                      \
    X(void,   DeleteRenderbuffers,     (GLsizei, const GLuint*))

#define GL3_DECLARE(ret, name, args) extern ret (GL3_API* name) args;
GL3_FUNCS(GL3_DECLARE)
#undef GL3_DECLARE

// Look every function up in the current context. True when all were found;
// the first call does the work and later ones return its answer. Call on the
// GL thread, with the context current.
bool load();

} // namespace gl3
