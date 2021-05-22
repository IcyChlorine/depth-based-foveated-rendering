///////////////////////////////////////////////////////////////////////////////
//
// Description:
// 
// Loads DDS images (DXTC1, DXTC3, DXTC5, RGB (888, 888X), and RGBA (8888) are
// supported) for use in OpenGL. Image is flipped when its loaded as DX images
// are stored with different coordinate system. If file has mipmaps and/or 
// cubemaps then these are loaded as well. Volume textures can be loaded as 
// well but they must be uncompressed.
//
// When multiple textures are loaded (i.e a volume or cubemap texture), 
// additional faces can be accessed using the array operator. 
//
// The mipmaps for each face are also stored in a list and can be accessed like 
// so: image.get_mipmap() (which accesses the first mipmap of the first 
// image). To get the number of mipmaps call the get_num_mipmaps function for
// a given texture.
//
// Call the is_volume() or is_cubemap() function to check that a loaded image
// is a volume or cubemap texture respectively. If a volume texture is loaded
// then the get_depth() function should return a number greater than 1. 
// Mipmapped volume textures and DXTC compressed volume textures are supported.
//
///////////////////////////////////////////////////////////////////////////////
//
// Update: 9/15/2003
//
// Added functions to create new image from a buffer of pixels. Added function
// to save current image to disk.
//
// Update: 6/11/2002
//
// Added some convenience functions to handle uploading textures to OpenGL. The
// following functions have been added:
//
//     bool upload_texture1D();
//     bool upload_texture2D(unsigned int imageIndex = 0, GLenum target = GL_TEXTURE_2D);
//     bool upload_textureRectangle();
//     bool upload_texture3D();
//     bool upload_textureCubemap();
//
// See function implementation below for instructions/comments on using each
// function.
//
// The open function has also been updated to take an optional second parameter
// specifying whether the image should be flipped on load. This defaults to 
// true.
//
///////////////////////////////////////////////////////////////////////////////
// Sample usage
///////////////////////////////////////////////////////////////////////////////
//
// Loading a compressed texture:
//
// CDDSImageGL image;
// GLuint texobj;
//
// image.load("compressed.dds");
// 
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_2D);
// glBindTexture(GL_TEXTURE_2D, texobj);
//
// glCompressedTexImage2D(GL_TEXTURE_2D, 0, image.get_format(), 
//     image.get_width(), image.get_height(), 0, image.get_size(), 
//     image);
//
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     CSurface mipmap = image.get_mipmap(i);
//
//     glCompressedTexImage2D(GL_TEXTURE_2D, i+1, image.get_format(), 
//         mipmap.get_width(), mipmap.get_height(), 0, mipmap.get_size(), 
//         mipmap);
// } 
///////////////////////////////////////////////////////////////////////////////
// 
// Loading an uncompressed texture:
//
// CDDSImageGL image;
// GLuint texobj;
//
// image.load("uncompressed.dds");
//
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_2D);
// glBindTexture(GL_TEXTURE_2D, texobj);
//
// glTexImage2D(GL_TEXTURE_2D, 0, image.get_components(), image.get_width(), 
//     image.get_height(), 0, image.get_format(), GL_UNSIGNED_BYTE, image);
//
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     glTexImage2D(GL_TEXTURE_2D, i+1, image.get_components(), 
//         image.get_mipmap(i).get_width(), image.get_mipmap(i).get_height(), 
//         0, image.get_format(), GL_UNSIGNED_BYTE, image.get_mipmap(i));
// }
//
///////////////////////////////////////////////////////////////////////////////
// 
// Loading an uncompressed cubemap texture:
//
// CDDSImageGL image;
// GLuint texobj;
// GLenum target;
// 
// image.load("cubemap.dds");
// 
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_CUBE_MAP_ARB);
// glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, texobj);
// 
// for (int n = 0; n < 6; n++)
// {
//     target = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB+n;
// 
//     glTexImage2D(target, 0, image.get_components(), image[n].get_width(), 
//         image[n].get_height(), 0, image.get_format(), GL_UNSIGNED_BYTE, 
//         image[n]);
// 
//     for (int i = 0; i < image[n].get_num_mipmaps(); i++)
//     {
//         glTexImage2D(target, i+1, image.get_components(), 
//             image[n].get_mipmap(i).get_width(), 
//             image[n].get_mipmap(i).get_height(), 0,
//             image.get_format(), GL_UNSIGNED_BYTE, image[n].get_mipmap(i));
//     }
// }
//
///////////////////////////////////////////////////////////////////////////////
// 
// Loading a volume texture:
//
// CDDSImageGL image;
// GLuint texobj;
// 
// image.load("volume.dds");
// 
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_3D);
// glBindTexture(GL_TEXTURE_3D, texobj);
// 
// PFNGLTEXIMAGE3DPROC glTexImage3D;
// glTexImage3D(GL_TEXTURE_3D, 0, image.get_components(), image.get_width(), 
//     image.get_height(), image.get_depth(), 0, image.get_format(), 
//     GL_UNSIGNED_BYTE, image);
// 
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     glTexImage3D(GL_TEXTURE_3D, i+1, image.get_components(), 
//         image[0].get_mipmap(i).get_width(), 
//         image[0].get_mipmap(i).get_height(), 
//         image[0].get_mipmap(i).get_depth(), 0, image.get_format(), 
//         GL_UNSIGNED_BYTE, image[0].get_mipmap(i));
// }
#ifdef MEMORY_LEAKS_CHECK
#   pragma message("build will Check for Memory Leaks!")
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif

#include<iostream>
#include <string.h>
#if defined(WIN32)
#  include <windows.h>
#  define GET_EXT_POINTER(name, type) \
      name = (type)wglGetProcAddress(#name)
#elif defined(UNIX)
#define GLX_GLXEXT_PROTOTYPES
#  include <GL/glx.h>
#  define GET_EXT_POINTER(name, type) \
      name = (type)glXGetProcAddressARB((const GLubyte*)#name)
#else
#  define GET_EXT_POINTER(name, type)
#endif

#ifdef MACOS
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#define GL_TEXTURE_RECTANGLE_NV GL_TEXTURE_RECTANGLE_EXT
#else
#include "GL/glew.h"
//#include <GL/gl.h>
//#include <GL/glext.h>
#endif

#include <stdio.h>
#include <assert.h>
#include "nv_dds_gl.h"

using namespace std;
using namespace nv_dds;

///////////////////////////////////////////////////////////////////////////////
// CDDSImageGL public functions

///////////////////////////////////////////////////////////////////////////////
// default constructor
CDDSImageGL::CDDSImageGL() : CDDSImage()
{
}

CDDSImageGL::~CDDSImageGL()
{
}

bool CDDSImageGL::load(std::string filename, bool flipImage, bool RGB2RGBA)
{
    return CDDSImage::load(filename, flipImage, RGB2RGBA);
}

///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed 1D texture
bool CDDSImageGL::upload_texture1D()
{
    assert(m_valid);
    assert(!m_images.empty());

    const CTexture &baseImage = m_images[0];

    assert(baseImage.get_height() == 1);
    assert(baseImage.get_width() > 0);

    if (is_compressed())
    {
        if (glCompressedTexImage1D == NULL)
            return false;
        
        glCompressedTexImage1D(GL_TEXTURE_1D, 0, m_format, 
            baseImage.get_width(), 0, baseImage.get_size(), baseImage);
        
        // load all mipmaps
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = baseImage.get_mipmap(i);
            glCompressedTexImage1D(GL_TEXTURE_1D, i+1, m_format, 
                mipmap.get_width(), 0, mipmap.get_size(), mipmap);
        }
    }
    else
    {
        GLint alignment = -1;
        if (!is_dword_aligned())
        {
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }

        glTexImage1D(GL_TEXTURE_1D, 0, m_internal_format, baseImage.get_width(), 0,
            m_format, GL_UNSIGNED_BYTE, baseImage);

        // load all mipmaps
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = baseImage.get_mipmap(i);

            glTexImage1D(GL_TEXTURE_1D, i+1, m_internal_format, 
                mipmap.get_width(), 0, m_format, GL_UNSIGNED_BYTE, mipmap);
        }

        if (alignment != -1)
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed 2D texture
//
// imageIndex - allows you to optionally specify other loaded surfaces for 2D
//              textures such as a face in a cubemap or a slice in a volume
//
//              default: 0
//
// target     - allows you to optionally specify a different texture target for
//              the 2D texture such as a specific face of a cubemap
//
//              default: GL_TEXTURE_2D
bool CDDSImageGL::upload_texture2D(unsigned int imageIndex, GLenum target)
{
    assert(m_valid);
    assert(!m_images.empty());
    assert(imageIndex >= 0);
    assert(imageIndex < m_images.size());
    assert(m_images[imageIndex]);

    const CTexture &image = m_images[imageIndex];

    assert(image.get_height() > 0);
    assert(image.get_width() > 0);
    assert(target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE_NV ||
        (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB && 
         target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB));
    
    if (is_compressed())
    {
        if (glCompressedTexImage2D == NULL)
            return false;
        
        glCompressedTexImage2D(target, 0, m_format, image.get_width(), 
            image.get_height(), 0, image.get_size(), image);
        
        // load all mipmaps
        for (unsigned int i = 0; i < image.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = image.get_mipmap(i);

            glCompressedTexImage2D(target, i+1, m_format, 
                mipmap.get_width(), mipmap.get_height(), 0, 
                mipmap.get_size(), mipmap);
        }
    }
    else
    {
        GLint alignment = -1;
        if (!is_dword_aligned())
        {
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }
        glTexImage2D(target, 0, m_internal_format, image.get_width(), 
            image.get_height(), 0, m_format, GL_UNSIGNED_BYTE, 
            image);

        // load all mipmaps
        for (unsigned int i = 0; i < image.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = image.get_mipmap(i);
            
            glTexImage2D(target, i+1, m_internal_format, mipmap.get_width(), 
                mipmap.get_height(), 0, m_format, GL_UNSIGNED_BYTE, mipmap); 
        }

        if (alignment != -1)
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    }
    
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed 3D texture
bool CDDSImageGL::upload_texture3D()
{
    assert(m_valid);
    assert(!m_images.empty());
    assert(m_type == Texture3D);

    const CTexture &baseImage = m_images[0];
    
    assert(baseImage.get_depth() >= 1);

    if (is_compressed())
    {
        if (glCompressedTexImage3D == NULL)
            return false;

        glCompressedTexImage3D(GL_TEXTURE_3D, 0, m_format,  
            baseImage.get_width(), baseImage.get_height(), baseImage.get_depth(),
            0, baseImage.get_size(), baseImage);
        
        // load all mipmap volumes
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = baseImage.get_mipmap(i);

            glCompressedTexImage3D(GL_TEXTURE_3D, i+1, m_format, 
                mipmap.get_width(), mipmap.get_height(), mipmap.get_depth(), 
                0, mipmap.get_size(), mipmap);
        }
    }
    else
    {
    /*    STRANGE... this part doesn make linkage work... missing func...
            if (glTexImage3D == NULL)
            return false;
    
        GLint alignment = -1;
        if (!is_dword_aligned())
        {
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }

        glTexImage3D(GL_TEXTURE_3D, 0, m_internal_format, baseImage.get_width(), 
            baseImage.get_height(), baseImage.get_depth(), 0, m_format, 
            GL_UNSIGNED_BYTE, baseImage);
        
        // load all mipmap volumes
        for (unsigned int i = 0; i < baseImage.get_num_mipmaps(); i++)
        {
            const CSurface &mipmap = baseImage.get_mipmap(i);

            glTexImage3D(GL_TEXTURE_3D, i+1, m_internal_format, 
                mipmap.get_width(), mipmap.get_height(), mipmap.get_depth(), 0, 
                m_format, GL_UNSIGNED_BYTE,  mipmap);
        }

        if (alignment != -1)
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);*/
    }
    
    return true;
}

bool CDDSImageGL::upload_textureRectangle()
{
    return upload_texture2D(0, GL_TEXTURE_RECTANGLE_NV);
}

///////////////////////////////////////////////////////////////////////////////
// uploads a compressed/uncompressed cubemap texture
bool CDDSImageGL::upload_textureCubemap()
{
    assert(m_valid);
    assert(!m_images.empty());
    assert(m_type == TextureCubemap);
    assert(m_images.size() == 6);

    GLenum target;

    // loop through cubemap faces and load them as 2D textures 
    for (unsigned int n = 0; n < 6; n++)
    {
        // specify cubemap face
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + n;
        if (!upload_texture2D(n, target))
            return false;
    }

    return true;
}

