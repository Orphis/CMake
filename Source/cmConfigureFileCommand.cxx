/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2001 Insight Consortium
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * The name of the Insight Consortium, nor the names of any consortium members,
   nor of any contributors, may be used to endorse or promote products derived
   from this software without specific prior written permission.

  * Modified source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "cmConfigureFileCommand.h"

// cmConfigureFileCommand
bool cmConfigureFileCommand::InitialPass(std::vector<std::string> const& args)
{
  if(args.size() < 2 )
    {
    this->SetError("called with incorrect number of arguments, expected 2");
    return false;
    }
  m_InputFile = args[0];
  m_OuputFile = args[1];
  m_CopyOnly = false;
  m_EscapeQuotes = false;
  m_Immediate = false;
  m_AtOnly = false;
  for(unsigned int i=2;i < args.size();++i)
    {
    if(args[i] == "COPYONLY")
      {
      m_CopyOnly = true;
      }
    else if(args[i] == "ESCAPE_QUOTES")
      {
      m_EscapeQuotes = true;
      }
    else if(args[i] == "@ONLY")
      {
      m_AtOnly = true;
      }
    else if(args[i] == "IMMEDIATE")
      {
      m_Immediate = true;
      }
    }
  
  // If we were told to copy the file immediately, then do it on the
  // first pass (now).
  if(m_Immediate)
    {
    this->ConfigureFile();
    }
  
  return true;
}

void cmConfigureFileCommand::FinalPass()
{
  if(!m_Immediate)
    {
    this->ConfigureFile();
    }
}

void cmConfigureFileCommand::ConfigureFile()
{
  m_Makefile->ExpandVariablesInString(m_InputFile);
  m_Makefile->AddCMakeDependFile(m_InputFile.c_str());
  m_Makefile->ExpandVariablesInString(m_OuputFile);
  cmSystemTools::ConvertToUnixSlashes(m_OuputFile);
  std::string::size_type pos = m_OuputFile.rfind('/');
  if(pos != std::string::npos)
    {
    std::string path = m_OuputFile.substr(0, pos);
    cmSystemTools::MakeDirectory(path.c_str());
    }
  
  if(m_CopyOnly)
    {
    cmSystemTools::CopyFileIfDifferent(m_InputFile.c_str(),
                                       m_OuputFile.c_str());
    }
  else
    {
    std::string tempOutputFile = m_OuputFile;
    tempOutputFile += ".tmp";
    std::ofstream fout(tempOutputFile.c_str());
    if(!fout)
      {
      cmSystemTools::Error("Could not open file for write in copy operatation", 
                           tempOutputFile.c_str());
      return;
      }
    std::ifstream fin(m_InputFile.c_str());
    if(!fin)
      {
      cmSystemTools::Error("Could not open file for read in copy operatation",
                           m_InputFile.c_str());
      return;
      }

    // now copy input to output and expand varibles in the
    // input file at the same time
    const int bufSize = 4096;
    char buffer[bufSize];
    std::string inLine;
    cmRegularExpression cmdefine("#cmakedefine[ \t]*([A-Za-z_0-9]*)");
    while(fin)
      {
      fin.getline(buffer, bufSize);
      if(fin)
        {
        inLine = buffer;
        m_Makefile->ExpandVariablesInString(inLine, m_EscapeQuotes, m_AtOnly);
        m_Makefile->RemoveVariablesInString(inLine, m_AtOnly);
        // look for special cmakedefine symbol and handle it
        // is the symbol defined
        if (cmdefine.find(inLine))
          {
          const char *def = m_Makefile->GetDefinition(cmdefine.match(1).c_str());
          if(!cmSystemTools::IsOff(def))
            {
            cmSystemTools::ReplaceString(inLine,
                                         "#cmakedefine", "#define");
            }
          else
            {
            cmSystemTools::ReplaceString(inLine,
                                         "#cmakedefine", "#undef");
            }
          }
        fout << inLine << "\n";
        }
      }
    // close the files before attempting to copy
    fin.close();
    fout.close();
    cmSystemTools::CopyFileIfDifferent(tempOutputFile.c_str(),
                                       m_OuputFile.c_str());
    cmSystemTools::RemoveFile(tempOutputFile.c_str());
    }
}

  
