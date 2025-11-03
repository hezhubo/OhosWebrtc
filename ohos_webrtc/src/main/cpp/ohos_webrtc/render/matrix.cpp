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

#include "matrix.h"

namespace webrtc {

using namespace ohos;

Matrix::Matrix() : impl_(DrawingMatrix::Create())
{
    impl_.Reset();
}

Matrix::Matrix(const Matrix& other) : impl_(other.impl_.Copy()) {}

Matrix::Matrix(Matrix&& other) noexcept = default;

Matrix& Matrix::operator=(const Matrix& other)
{
    Matrix(other).Swap(*this);
    return *this;
}

Matrix& Matrix::operator=(Matrix&& other) noexcept = default;

void Matrix::SetValues(float values[kMatrixSize])
{
    // clang-format off
    OH_Drawing_MatrixSetMatrix(impl_.Raw(),
        values[kScaleX], values[kSkewX], values[kTransX],
        values[kSkewY], values[kScaleY], values[kTransY],
        values[kPersp0], values[kPersp1], values[kPersp2]);
    // clang-format on
}

void Matrix::PreRotate(float degree, float px, float py)
{
    impl_.PreRotate(degree, px, py);
}

void Matrix::PreScale(float sx, float sy, float px, float py)
{
    impl_.PreScale(sx, sy, px, py);
}

void Matrix::PreTranslate(float dx, float dy)
{
    impl_.PreTranslate(dx, dy);
}

void Matrix::PostRotate(float degree, float px, float py)
{
    impl_.PostRotate(degree, px, py);
}

void Matrix::PostScale(float sx, float sy, float px, float py)
{
    impl_.PostScale(sx, sy, px, py);
}

void Matrix::PostTranslate(float dx, float dy)
{
    impl_.PostTranslate(dx, dy);
}

void Matrix::Reset()
{
    impl_.Reset();
}

void Matrix::Concat(const Matrix& a, const Matrix& b)
{
    impl_.Concat(a.impl_, b.impl_);
}

void Matrix::PreConcat(const Matrix& other)
{
    impl_.PreConcat(other.impl_);
}

void Matrix::PostConcat(const Matrix& other)
{
    impl_.PostConcat(other.impl_);
}

void Matrix::Rotate(float degree, float px, float py)
{
    impl_.Rotate(degree, px, py);
}

void Matrix::Translate(float dx, float dy)
{
    impl_.Translate(dx, dy);
}

void Matrix::Scale(float sx, float sy, float px, float py)
{
    impl_.Scale(sx, sy, px, py);
}

bool Matrix::Invert(Matrix& inverse)
{
    return impl_.Invert(inverse.impl_);
}

void Matrix::GetAll(float value[kMatrixSize]) const
{
    impl_.GetAll(value);
}

float Matrix::GetValue(int index) const
{
    return impl_.GetValue(index);
}

bool Matrix::IsEqual(const Matrix& other) const
{
    return impl_.IsEqual(other.impl_);
}

bool Matrix::IsIdentity() const
{
    return impl_.IsIdentity();
}

} // namespace webrtc
