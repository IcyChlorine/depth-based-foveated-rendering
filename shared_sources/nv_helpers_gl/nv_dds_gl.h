#ifndef __NV_DDS_H__
#define __NV_DDS_H__

#if defined(WIN32)
#  include <windows.h>
#endif

#include <string>
#include <deque>
#include <assert.h>
#include<stdint.h>

#if defined(MACOS)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include "GL/glew.h"
#include "nv_helpers/nv_dds.h"
//#include <GL/gl.h>
//#include <GL/glext.h>
#endif

namespace nv_dds
{

    class CDDSImageGL : public nv_dds::CDDSImage
    {
        public:
            CDDSImageGL();
            virtual ~CDDSImageGL();

            virtual bool load(std::string filename, bool flipImage = true, bool RGB2RGBA=false);

            bool upload_texture1D();
            bool upload_texture2D(unsigned int imageIndex = 0, GLenum target = GL_TEXTURE_2D);
            bool upload_texture3D();
            bool upload_textureRectangle();
            bool upload_textureCubemap();

        private:
#ifndef MACOS
            static PFNGLTEXIMAGE3DEXTPROC glTexImage3D;
            static PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glCompressedTexImage1DARB;
            static PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB;
            static PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glCompressedTexImage3DARB;
#endif
    };
}
#endif
