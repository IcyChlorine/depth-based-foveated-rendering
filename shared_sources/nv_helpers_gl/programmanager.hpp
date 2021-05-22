/*-----------------------------------------------------------------------
  Copyright (c) 2014, NVIDIA. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Neither the name of its contributors may be used to endorse 
     or promote products derived from this software without specific
     prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/

#ifndef NV_PROGRAMMANAGER_INCLUDED
#define NV_PROGRAMMANAGER_INCLUDED

#if USEOPENGL
#include <GL/glew.h>
#endif
#include <stdio.h>
#include <vector>
#include <string>
#include "nv_helpers/AssetLoader.h"

#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER          0x8B30
#define GL_VERTEX_SHADER            0x8B31
#define GL_GEOMETRY_SHADER          0x8DD9
#define GL_TESS_EVALUATION_SHADER   0x8E87
#define GL_TESS_CONTROL_SHADER      0x8E88
#define GL_COMPUTE_SHADER           0x91B9
#endif

namespace nv_helpers_gl
{

  class ProgramManager {
  public:
    struct IncludeEntry {
      std::string   name;
      std::string   filename;
      std::string   content;
    };

    typedef std::vector<IncludeEntry> IncludeRegistry;

    static std::string format(const char* msg, ...);

  public:
    static const unsigned int PREPROCESS_ONLY_PROGRAM = ~0;

    class ProgramID {
    public:
      size_t  m_value;

      ProgramID() : m_value(~0) {}

      ProgramID( size_t b) : m_value(b) {}
      operator size_t() const { return m_value; }
      ProgramID& operator=( size_t b) { m_value = b; return *this; }

    };

    class IncludeID {
    public:
      size_t  m_value;

      IncludeID() : m_value(~0) {}

      IncludeID( size_t b) : m_value(b) {}
      operator size_t() const { return m_value; }
      IncludeID& operator=( size_t b) { m_value = b; return *this; }

    };

    struct Definition {

      Definition() : type(0) , prepend(""), filename("") {}
      Definition(unsigned int type, std::string const & prepend, std::string const & filename) : type(type) , prepend(prepend) , filename(filename) {}
      Definition(unsigned int type, std::string const & filename) : type(type) , prepend("") , filename(filename) {}

      unsigned int  type;
      std::string prepend;
      std::string filename;
      std::string preprocessed;
    };

    struct Program {
      Program() : program(0) {}

      unsigned int              program;
      std::vector<Definition>   definitions;
    };

    IncludeID registerInclude(std::string const & name, std::string const & filename, std::string const & content= std::string() );

    ProgramID createProgram(size_t num, const Definition* definitions);
    ProgramID createProgram(const std::vector<Definition>& definitions);
    ProgramID createProgram(const Definition& def0, const Definition& def1 = Definition(), const Definition& def2 = Definition(), const Definition& def3 = Definition(), const Definition& def4 = Definition());

    void destroyProgram( ProgramID idx );

    void reloadPrograms();
    void deletePrograms();
    bool areProgramsValid();


    bool isValid( ProgramID idx ) const;
    unsigned int get(ProgramID idx) const;
    Program& getProgram(ProgramID idx);
    const Program& getProgram(ProgramID idx) const;
    IncludeEntry& getInclude(IncludeID idx);
    const IncludeEntry& getInclude(IncludeID idx) const;

    std::string m_prepend;
    std::string m_useCacheFile;
    bool        m_preferCache;
    bool        m_preprocessOnly;
    bool        m_rawOnly;
    bool        m_forceLineFilenames;
    bool        m_lineMarkers;

    // Note: we might want to avoid and directly rely on AssetLoaderAddSearchPath in the samples... ?
    void addDirectory(const std::string& dir){
        AssetLoaderAddSearchPath(dir.c_str());
        //m_directories.push_back(dir);
    }

    ProgramManager() 
      : m_preprocessOnly(false)
      , m_preferCache(false)
      , m_rawOnly(false)
      , m_forceLineFilenames(false)
      , m_lineMarkers(true)
    {
      AssetLoaderAddSearchPath(".");
      //m_directories.push_back(".");
    }

  private:
    bool createProgram(Program& prog, size_t num, Definition* definitions);
#if USEOPENGL
    bool loadBinary( GLuint program, const std::string& combinedPrepend, const std::string& combinedFilenames );
    void saveBinary( GLuint program, const std::string& combinedPrepend, const std::string& combinedFilenames );
    std::string binaryName(const std::string& combinedPrepend, const std::string& combinedFilenames);
#endif

    //std::vector<std::string>  m_directories;
    IncludeRegistry           m_includes;
    std::vector<Program>      m_programs;
  };

}//namespace nvglf


#endif//NV_PROGRAM_INCLUDED
