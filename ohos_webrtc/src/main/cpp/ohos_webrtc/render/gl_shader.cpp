/**
 * Copyright (c) 2024 Archermind Technology (Nanjing) Co. Ltd.
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
 */

#include "gl_shader.h"

#include <GLES3/gl3.h>

#include "rtc_base/logging.h"

namespace webrtc {

#define kMaxLogSize 1024

GlShader::GlShader() = default;

GlShader::~GlShader()
{
    if (id_ > 0) {
        glDeleteProgram(id_);
        id_ = 0;
    }
}

bool GlShader::Compile(const char* vertexShaderString, const char* fragmentShaderString)
{
    unsigned int vertex, fragment;

    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderString, NULL);
    glCompileShader(vertex);
    if (!CheckCompileErrors(vertex, "VERTEX")) {
        return false;
    }

    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShaderString, NULL);
    glCompileShader(fragment);
    if (!CheckCompileErrors(fragment, "FRAGMENT")) {
        return false;
    }

    // shader Program
    id_ = glCreateProgram();
    glAttachShader(id_, vertex);
    glAttachShader(id_, fragment);
    glLinkProgram(id_);
    if (!CheckCompileErrors(id_, "PROGRAM")) {
        return false;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return true;
}

void GlShader::Use()
{
    glUseProgram(id_);
}

int GlShader::GetAttribLocation(const char* name)
{
    return glGetAttribLocation(id_, name);
}

int GlShader::GetUniformLocation(const char* name)
{
    return glGetUniformLocation(id_, name);
}

void GlShader::SetBool(const char* name, bool value) const
{
    glUniform1i(glGetUniformLocation(id_, name), static_cast<int>(value));
}

void GlShader::SetInt(const char* name, int value) const
{
    glUniform1i(glGetUniformLocation(id_, name), value);
}

void GlShader::SetFloat(const char* name, float value) const
{
    glUniform1f(glGetUniformLocation(id_, name), value);
}

void GlShader::SetVector2f(const char* name, float x, float y)
{
    glUniform2f(glGetUniformLocation(id_, name), x, y);
}

void GlShader::SetVector3f(const char* name, float x, float y, float z)
{
    glUniform3f(glGetUniformLocation(id_, name), x, y, z);
}

void GlShader::SetVector4f(const char* name, float x, float y, float z, float w)
{
    glUniform4f(glGetUniformLocation(id_, name), x, y, z, w);
}

void GlShader::SetMatrix4(const char* name, const std::array<float, 16>& matrix)
{
    glUniformMatrix4fv(glGetUniformLocation(id_, name), 1, false, matrix.data());
}

bool GlShader::CheckCompileErrors(unsigned int shader, const std::string& type)
{
    int success;
    char infoLog[kMaxLogSize];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, kMaxLogSize, NULL, infoLog);
            RTC_LOG(LS_ERROR) << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << ", " << infoLog;
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, kMaxLogSize, NULL, infoLog);
            RTC_LOG(LS_ERROR) << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << ", " << infoLog;
            return false;
        }
    }

    return true;
}

} // namespace webrtc
