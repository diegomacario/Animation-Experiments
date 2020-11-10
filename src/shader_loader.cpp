#include <glad/glad.h>

#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#include "shader_loader.h"

std::shared_ptr<Shader> ShaderLoader::loadResource(const std::string& vShaderFilePath,
                                                   const std::string& fShaderFilePath) const
{
   // Read the vertex and fragment shaders
   std::string vShaderCode, fShaderCode;
   if (!readShaderFile(vShaderFilePath, vShaderCode) ||
       !readShaderFile(fShaderFilePath, fShaderCode))
   {
      return nullptr;
   }

   // Compile the vertex shader
   unsigned int vShaderID = createAndCompileShader(vShaderCode, GL_VERTEX_SHADER);
   if (!shaderCompilationSucceeded(vShaderID))
   {
      logShaderCompilationErrors(vShaderID, GL_VERTEX_SHADER, vShaderFilePath);
      glDeleteShader(vShaderID);
      return nullptr;
   }

   // Compile the fragment shader
   unsigned int fShaderID = createAndCompileShader(fShaderCode, GL_FRAGMENT_SHADER);
   if (!shaderCompilationSucceeded(fShaderID))
   {
      logShaderCompilationErrors(fShaderID, GL_FRAGMENT_SHADER, fShaderFilePath);
      glDeleteShader(vShaderID);
      glDeleteShader(fShaderID);
      return nullptr;
   }

   // Link the shader program
   unsigned int shaderProgID = createAndLinkShaderProgram(vShaderID, fShaderID);
   if (!shaderProgramLinkingSucceeded(shaderProgID))
   {
      logShaderProgramLinkingErrors(shaderProgID);
      glDetachShader(shaderProgID, vShaderID);
      glDetachShader(shaderProgID, fShaderID);
      glDeleteShader(vShaderID);
      glDeleteShader(fShaderID);
      glDeleteProgram(shaderProgID);
      return nullptr;
   }

   // Delete the vertex and fragment shaders
   glDetachShader(shaderProgID, vShaderID);
   glDetachShader(shaderProgID, fShaderID);
   glDeleteShader(vShaderID);
   glDeleteShader(fShaderID);

   // Read the attributes from the shader program
   std::map<std::string, unsigned int> attributes;
   readAttributes(shaderProgID, attributes);

   // Read the uniforms from the shader program
   std::map<std::string, unsigned int> uniforms;
   readUniforms(shaderProgID, uniforms);

   return std::make_shared<Shader>(shaderProgID);
}

std::shared_ptr<Shader> ShaderLoader::loadResource(const std::string& vShaderFilePath,
                                                   const std::string& fShaderFilePath,
                                                   const std::string& gShaderFilePath) const
{
   // Read the vertex and fragment shaders
   std::string vShaderCode, fShaderCode, gShaderCode;
   if (!readShaderFile(vShaderFilePath, vShaderCode) ||
       !readShaderFile(fShaderFilePath, fShaderCode) ||
       !readShaderFile(gShaderFilePath, gShaderCode))
   {
      return nullptr;
   }

   // Compile the vertex shader
   unsigned int vShaderID = createAndCompileShader(vShaderCode, GL_VERTEX_SHADER);
   if (!shaderCompilationSucceeded(vShaderID))
   {
      logShaderCompilationErrors(vShaderID, GL_VERTEX_SHADER, vShaderFilePath);
      glDeleteShader(vShaderID);
      return nullptr;
   }

   // Compile the fragment shader
   unsigned int fShaderID = createAndCompileShader(fShaderCode, GL_FRAGMENT_SHADER);
   if (!shaderCompilationSucceeded(fShaderID))
   {
      logShaderCompilationErrors(fShaderID, GL_FRAGMENT_SHADER, fShaderFilePath);
      glDeleteShader(vShaderID);
      glDeleteShader(fShaderID);
      return nullptr;
   }

   // Compile the geometry shader
   unsigned int gShaderID = createAndCompileShader(gShaderCode, GL_GEOMETRY_SHADER);
   if (!shaderCompilationSucceeded(gShaderID))
   {
      logShaderCompilationErrors(gShaderID, GL_GEOMETRY_SHADER, gShaderFilePath);
      glDeleteShader(vShaderID);
      glDeleteShader(fShaderID);
      glDeleteShader(gShaderID);
      return nullptr;
   }

   // Link the shader program
   unsigned int shaderProgID = createAndLinkShaderProgram(vShaderID, fShaderID, gShaderID);
   if (!shaderProgramLinkingSucceeded(shaderProgID))
   {
      logShaderProgramLinkingErrors(shaderProgID);
      glDetachShader(shaderProgID, vShaderID);
      glDetachShader(shaderProgID, fShaderID);
      glDetachShader(shaderProgID, gShaderID);
      glDeleteShader(vShaderID);
      glDeleteShader(fShaderID);
      glDeleteShader(gShaderID);
      glDeleteProgram(shaderProgID);
      return nullptr;
   }

   // Delete the vertex, fragment and geometry shaders
   glDetachShader(shaderProgID, vShaderID);
   glDetachShader(shaderProgID, fShaderID);
   glDetachShader(shaderProgID, gShaderID);
   glDeleteShader(vShaderID);
   glDeleteShader(fShaderID);
   glDeleteShader(gShaderID);

   // Read the attributes from the shader program
   std::map<std::string, unsigned int> attributes;
   readAttributes(shaderProgID, attributes);

   // Read the uniforms from the shader program
   std::map<std::string, unsigned int> uniforms;
   readUniforms(shaderProgID, uniforms);

   return std::make_shared<Shader>(shaderProgID);
}

bool ShaderLoader::readShaderFile(const std::string& shaderFilePath, std::string& outShaderCode) const
{
   std::ifstream shaderFile(shaderFilePath);

   if (shaderFile)
   {
      // Read the entire file into the output string
      std::stringstream shaderStream;
      shaderStream << shaderFile.rdbuf();
      outShaderCode = shaderStream.str();
      shaderFile.close();
      return true;
   }
   else
   {
      std::cout << "Error - ShaderLoader::createAndCompileShader - The following shader file could not be opened: " << shaderFilePath << "\n";
      return false;
   }
}

unsigned int ShaderLoader::createAndCompileShader(const std::string& shaderCode, GLenum shaderType) const
{
   // Create and compile the shader
   unsigned int shaderID = glCreateShader(shaderType);
   const char* shaderCodeCStr = shaderCode.c_str();
   glShaderSource(shaderID, 1, &shaderCodeCStr, nullptr);
   glCompileShader(shaderID);
   return shaderID;
}

unsigned int ShaderLoader::createAndLinkShaderProgram(unsigned int vShaderID, unsigned int fShaderID) const
{
   unsigned int shaderProgID = glCreateProgram();

   glAttachShader(shaderProgID, vShaderID);
   glAttachShader(shaderProgID, fShaderID);

   glLinkProgram(shaderProgID);

   return shaderProgID;
}

unsigned int ShaderLoader::createAndLinkShaderProgram(unsigned int vShaderID, unsigned int fShaderID, unsigned int gShaderID) const
{
   unsigned int shaderProgID = glCreateProgram();

   glAttachShader(shaderProgID, vShaderID);
   glAttachShader(shaderProgID, fShaderID);
   glAttachShader(shaderProgID, gShaderID);

   glLinkProgram(shaderProgID);

   return shaderProgID;
}

bool ShaderLoader::shaderCompilationSucceeded(unsigned int shaderID) const
{
   int compilationStatus;
   glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compilationStatus);
   return compilationStatus;
}

bool ShaderLoader::shaderProgramLinkingSucceeded(unsigned int shaderProgID) const
{
   int linkingStatus;
   glGetProgramiv(shaderProgID, GL_LINK_STATUS, &linkingStatus);
   return linkingStatus;
}

void ShaderLoader::logShaderCompilationErrors(unsigned int shaderID, GLenum shaderType, const std::string& shaderFilePath) const
{
   int infoLogLength = 0;
   glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

   std::vector<char> infoLog(infoLogLength);
   glGetShaderInfoLog(shaderID, infoLogLength, nullptr, &infoLog[0]);

   std::cout << "Error - ShaderLoader::checkForCompilationErrors - The error below occurred while compiling this shader: " << shaderFilePath << "\n" << infoLog.data() << "\n";
}

void ShaderLoader::logShaderProgramLinkingErrors(unsigned int shaderProgID) const
{
   int infoLogLength = 0;
   glGetProgramiv(shaderProgID, GL_INFO_LOG_LENGTH, &infoLogLength);

   std::vector<char> infoLog(infoLogLength);
   glGetProgramInfoLog(shaderProgID, infoLogLength, nullptr, &infoLog[0]);

   std::cout << "Error - ShaderLoader::checkForLinkingErrors - The following error occurred while linking a shader program:\n" << infoLog.data() << "\n";
}

void ShaderLoader::readAttributes(unsigned int shaderProgID, std::map<std::string, unsigned int>& outAttributes) const
{
   glUseProgram(shaderProgID);

   // Get the number of active attributes
   int numActiveAttributes = 0;
   glGetProgramiv(shaderProgID, GL_ACTIVE_ATTRIBUTES, &numActiveAttributes);

   // Get the length of the longest attribute name and allocate a vector of characters with that length
   int lengthOfLongestAttributeName = 0;
   glGetProgramiv(shaderProgID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &lengthOfLongestAttributeName);
   std::vector<char> attributeName(lengthOfLongestAttributeName);

   // Loop over the attributes
   int    attributeSize;
   GLenum attributeType;
   for (int attributeIndex = 0; attributeIndex < numActiveAttributes; ++attributeIndex)
   {
      // Get the name of the current attribute
      memset(&attributeName[0], 0, attributeName.size() * sizeof(attributeName[0]));
      glGetActiveAttrib(shaderProgID,
                        static_cast<unsigned int>(attributeIndex),
                        lengthOfLongestAttributeName,
                        nullptr,
                        &attributeSize,
                        &attributeType,
                        &attributeName[0]);

      // Get the location of the current attribute
      int attributeLocation = glGetAttribLocation(shaderProgID, attributeName.data());

      // If the location is valid, store it along with its corresponding name
      if (attributeLocation >= 0)
      {
         std::cout << "Attrib - Name: " << attributeName.data() << " - Location: " << attributeLocation << '\n';
         outAttributes[attributeName.data()] = attributeLocation;
      }
   }

   glUseProgram(0);
}

// TODO: This code doesn't properly handle uniforms that are arrays of structs
//       It only handles:
//       - Uniforms that are simple types
//       - Uniforms that are structs
//       - Uniforms that are arrays of simple types
void ShaderLoader::readUniforms(unsigned int shaderProgID, std::map<std::string, unsigned int>& outUniforms) const
{
   glUseProgram(shaderProgID);

   // Get the number of active uniforms
   int numActiveUniforms = 0;
   glGetProgramiv(shaderProgID, GL_ACTIVE_UNIFORMS, &numActiveUniforms);

   // Get the length of the longest uniform name and allocate a vector of characters with that length
   int lengthOfLongestUniformName = 0;
   glGetProgramiv(shaderProgID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &lengthOfLongestUniformName);
   std::vector<char> uniformName(lengthOfLongestUniformName);

   // Loop over the uniforms
   int    uniformSize;
   GLenum uniformType;
   for (int uniformIndex = 0; uniformIndex < numActiveUniforms; ++uniformIndex)
   {
      // Get the name of the current uniform
      memset(&uniformName[0], 0, uniformName.size() * sizeof(uniformName[0]));
      glGetActiveUniform(shaderProgID,
                         static_cast<unsigned int>(uniformIndex),
                         lengthOfLongestUniformName,
                         nullptr,
                         &uniformSize,
                         &uniformType,
                         &uniformName[0]);

      // Get the location of the current uniform
      int uniformLocation = glGetUniformLocation(shaderProgID, uniformName.data());

      // If the location is valid
      if (uniformLocation >= 0)
      {
         // Check if the uniform is an array by searching for an open square bracket in its name
         std::string uniformNameStr = uniformName.data();
         std::size_t indexOfOpenSquareBracket = uniformNameStr.find('[');
         if (indexOfOpenSquareBracket != std::string::npos)
         {
            // If the uniform is an array, erase everything after the open square bracket
            // E.g. inverseBindMatrices[0] would become inverseBindMatrices
            uniformNameStr.erase(uniformNameStr.begin() + indexOfOpenSquareBracket, uniformNameStr.end());

            // In the loop below we add square brackets with indices to the uniform name and check if the resulting uniform exists
            // We do this until we reach an invalid index
            // E.g. if inverseBindMatrices was an array of 3 matrices, the for loop below would store
            // name/location pairs for inverseBindMatrices[0], inverseBindMatrices[1] and inverseBindMatrices[2]
            std::string uniformNameWithIndex;
            unsigned int uniformArrayIndex = 0;
            while (true)
            {
               uniformNameWithIndex = uniformNameStr + "[" + std::to_string(uniformArrayIndex++) + "]";

               int uniformLocation = glGetUniformLocation(shaderProgID, uniformNameWithIndex.c_str());

               // If the location is invalid, we have reached the end of the array, so we break out of the while loop
               if (uniformLocation < 0)
               {
                  break;
               }

               // If the location is valid, store it along with its corresponding name-with-an-index
               std::cout << "Uniform - Name: " << uniformNameWithIndex << " - Location: " << uniformLocation << '\n';
               outUniforms[uniformNameWithIndex] = uniformLocation;
            }
         }

         // If the location is valid, store it along with its corresponding name
         std::cout << "Uniform - Name: " << uniformNameStr << " - Location: " << uniformLocation << '\n';
         outUniforms[uniformNameStr] = uniformLocation;
      }
   }

   glUseProgram(0);
}
