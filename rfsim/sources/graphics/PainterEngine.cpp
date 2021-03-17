////////////////////////////////////////////////////////////////////////////////////
// MIT License                                                                    //
//                                                                                //
// Copyright (c) 2021 The RobotFootballSim project authors                        //
//                                                                                //
// Permission is hereby granted, free of charge, to any person obtaining a copy   //
// of this software and associated documentation files (the "Software"), to deal  //
// in the Software without restriction, including without limitation the rights   //
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      //
// copies of the Software, and to permit persons to whom the Software is          //
// furnished to do so, subject to the following conditions:                       //
//                                                                                //
// The above copyright notice and this permission notice shall be included in all //
// copies or substantial portions of the Software.                                //
//                                                                                //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    //
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  //
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  //
// SOFTWARE.                                                                      //
////////////////////////////////////////////////////////////////////////////////////

#include <opengl/GLShader.hpp>
#include <opengl/GLDynamicGeometry.hpp>
#include <opengl/GLGeometryLayout.hpp>
#include <opengl/GLTexture.hpp>
#include <shaders/ImageDraw.hpp>
#include <shaders/RectDraw.hpp>
#include <graphics/PainterEngine.hpp>
#include <graphics/PainterItems.hpp>
#include <unordered_map>
#include <utility>

namespace rfsim {

    using Rect = PainterEngine::Rect;
    using Recti = PainterEngine::Recti;
    using Color = PainterEngine::Color;

    class PainterEnginePrivate {
    public:

        PainterEnginePrivate() {
            std::vector<char> vertex;
            std::vector<char> fragment;

            vertex.resize(IMAGE_DRAW_VERTEX_SOURCE.size());
            std::memcpy(vertex.data(), IMAGE_DRAW_VERTEX_SOURCE.data(), IMAGE_DRAW_VERTEX_SOURCE.size());

            fragment.resize(IMAGE_DRAW_FRAGMENT_SOURCE.size());
            std::memcpy(fragment.data(), IMAGE_DRAW_FRAGMENT_SOURCE.data(), IMAGE_DRAW_FRAGMENT_SOURCE.size());

            mImageDrawShader = std::make_shared<GLShader>(vertex, fragment);

            GLGeometryLayout imageDrawLayout(3);
            {
                imageDrawLayout[0].location = 0;
                imageDrawLayout[0].bufferId = 0;
                imageDrawLayout[0].stride = (3 + 2) * sizeof(float);
                imageDrawLayout[0].offset = 0;
                imageDrawLayout[0].baseType = GL_FLOAT;
                imageDrawLayout[0].components = 3;
                imageDrawLayout[0].perInstance = false;

                imageDrawLayout[1].location = 1;
                imageDrawLayout[1].bufferId = 0;
                imageDrawLayout[1].stride = (3 + 2) * sizeof(float);
                imageDrawLayout[1].offset = 3 * sizeof(float);
                imageDrawLayout[1].baseType = GL_FLOAT;
                imageDrawLayout[1].components = 2;
                imageDrawLayout[1].perInstance = false;

                imageDrawLayout[2].location = 2;
                imageDrawLayout[2].bufferId = 1;
                imageDrawLayout[2].stride = 4 * sizeof(float);
                imageDrawLayout[2].offset = 0;
                imageDrawLayout[2].baseType = GL_FLOAT;
                imageDrawLayout[2].components = 4;
                imageDrawLayout[2].perInstance = true;
            }

            mImageDrawGeometry = std::make_shared<GLDynamicGeometry>(2, true, GL_TRIANGLES, std::move(imageDrawLayout));

            vertex.resize(RECT_DRAW_VERTEX_SOURCE.size());
            std::memcpy(vertex.data(), RECT_DRAW_VERTEX_SOURCE.data(), RECT_DRAW_VERTEX_SOURCE.size());

            fragment.resize(RECT_DRAW_FRAGMENT_SOURCE.size());
            std::memcpy(fragment.data(), RECT_DRAW_FRAGMENT_SOURCE.data(), RECT_DRAW_FRAGMENT_SOURCE.size());

            mRectDrawShader = std::make_shared<GLShader>(vertex, fragment);

            GLGeometryLayout rectDrawLayout(4);
            {
                rectDrawLayout[0].location = 0;
                rectDrawLayout[0].bufferId = 0;
                rectDrawLayout[0].stride = 3 * sizeof(float);
                rectDrawLayout[0].offset = 0;
                rectDrawLayout[0].baseType = GL_FLOAT;
                rectDrawLayout[0].components = 3;
                rectDrawLayout[0].perInstance = false;

                rectDrawLayout[1].location = 1;
                rectDrawLayout[1].bufferId = 1;
                rectDrawLayout[1].stride = (4 + 4 + 1) * sizeof(float);
                rectDrawLayout[1].offset = (0) * sizeof(float);
                rectDrawLayout[1].baseType = GL_FLOAT;
                rectDrawLayout[1].components = 4;
                rectDrawLayout[1].perInstance = false;

                rectDrawLayout[2].location = 2;
                rectDrawLayout[2].bufferId = 1;
                rectDrawLayout[2].stride = (4 + 4 + 1) * sizeof(float);
                rectDrawLayout[2].offset = (0 + 4) * sizeof(float);
                rectDrawLayout[2].baseType = GL_FLOAT;
                rectDrawLayout[2].components = 4;
                rectDrawLayout[2].perInstance = false;

                rectDrawLayout[3].location = 3;
                rectDrawLayout[3].bufferId = 1;
                rectDrawLayout[3].stride = (4 + 4 + 1) * sizeof(float);
                rectDrawLayout[3].offset = (0 + 4 + 4) * sizeof(float);
                rectDrawLayout[3].baseType = GL_FLOAT;
                rectDrawLayout[3].components = 1;
                rectDrawLayout[3].perInstance = false;
            }

            mRectDrawGeometry = std::make_shared<GLDynamicGeometry>(2, true, GL_TRIANGLES, std::move(rectDrawLayout));
        }

        void AddRect(const PainterEngine::Rect& r, float angle, const PainterEngine::Palette& palette) {
            PainterRect rect(r, angle, palette, GetNextZ());
            mRects.push_back(rect);
        }

        void AddImage(const PainterEngine::Rect &target, float angle, const std::shared_ptr<Image> &i, const PainterEngine::Palette& palette) {
            auto found = mImageTexCache.find(i.get());
            auto texture = std::shared_ptr<GLTexture>{};

            if (found == mImageTexCache.end()) {
                texture = std::make_shared<GLTexture>(i);
                mImageTexCache.insert({i.get(), texture});
            }
            else {
                texture = found->second;
            }

            PainterImage image(texture, target, angle, palette, GetNextZ());
            mImages.push_back(std::move(image));
        }

        int GetNextZ() {
            mCurrentZ += mStepZ;
            return mCurrentZ;
        }

        void Prepare() {

        }

        void Draw(const Recti& area, const Rect& space, const std::shared_ptr<Window> &target, const Color& clearColor) {
            target->MakeContextCurrent();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
            glDepthMask(GL_TRUE);
            glClearDepthf(1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glFrontFace(GL_CCW);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glViewport(area.x, area.y, area.z, area.w);

            static const std::string AREA_SIZE = "areaSize";
            static const std::string PROJ_VIEW = "projView";
            glm::vec2 areaSize = glm::vec2(space.z, space.w);
            glm::mat4 proj = glm::ortho(space.x, space.x + space.z, space.y, space.y + space.w, 0.0f, (float) -mFarZ);

            // Draw images separately
            // todo: batch draw commands with the same image

            mImageDrawShader->Bind();
            for (const auto& image: mImages) {
                static const std::string IMAGE_TEXTURE = "imageTexture";
                static const std::string TRANSPARENT_COLOR = "transparentColor";
                static const std::string IS_SRGB = "isSRGB";

                size_t indicesToDraw = 0;

                mImageDrawGeometry->ClearGeometry();
                image.PrepareImage(*mImageDrawGeometry, indicesToDraw);
                mImageDrawGeometry->UpdateResource();


                mImageDrawShader->SetMatrix4(PROJ_VIEW, proj);
                mImageDrawShader->SetVec2(AREA_SIZE, areaSize);
                mImageDrawShader->SetTexture(IMAGE_TEXTURE, image.image, 0);
                mImageDrawShader->SetVec4(TRANSPARENT_COLOR, image.transparentColor);
                mImageDrawShader->SetBool(IS_SRGB, image.image->IsSRGB());

                assert(indicesToDraw);

                mImageDrawGeometry->Bind();
                mImageDrawGeometry->Draw(6, 1);
                mImageDrawGeometry->Unbind();
            }
            mImageDrawShader->Unbind();

            // Draw all rects in batch fashion
            // Params are packed into vertex attributes

            mRectDrawShader->Bind();
            mRectDrawGeometry->ClearGeometry();
            size_t totalIndicesToDraw = 0;
            size_t totalVerticesToDraw = 0;
            for (const auto& rect: mRects) {
                size_t indicesToDraw = 0;
                size_t verticesToDraw = 0;

                rect.PrepareRect(*mRectDrawGeometry, totalVerticesToDraw, indicesToDraw, verticesToDraw);

                assert(indicesToDraw);
                assert(verticesToDraw);
                totalIndicesToDraw += indicesToDraw;
                totalVerticesToDraw += verticesToDraw;
            }
            mRectDrawGeometry->UpdateResource();
            mRectDrawShader->SetMatrix4(PROJ_VIEW, proj);
            mRectDrawShader->SetVec2(AREA_SIZE, areaSize);
            mRectDrawGeometry->Bind();
            mRectDrawGeometry->Draw(totalIndicesToDraw, 1);
            mRectDrawGeometry->Unbind();
            mRectDrawShader->Unbind();
        }

        void Clear() {
            mCurrentZ = mFarZ;
            mRects.clear();
            mImages.clear();
        }

    private:
        int mFarZ = -10000;
        int mStepZ = 2;
        int mCurrentZ = mFarZ;

        std::vector<PainterRect> mRects;
        std::shared_ptr<GLDynamicGeometry> mRectDrawGeometry;
        std::shared_ptr<GLShader> mRectDrawShader;

        std::vector<PainterImage> mImages;
        std::shared_ptr<GLDynamicGeometry> mImageDrawGeometry;
        std::shared_ptr<GLShader> mImageDrawShader;
        std::unordered_map<Image*, std::shared_ptr<GLTexture>> mImageTexCache;
    };

    PainterEngine::PainterEngine(const Recti &area, const Rect& space, std::shared_ptr<Window> target)
        : mArea(area),
          mSpace(space),
          mWindow(std::move(target)) {
        // Actual drawing logic
        mPrivate = std::make_shared<PainterEnginePrivate>();
    }

    void PainterEngine::DrawLine(const Point &from, const Point &to) {

    }

    void PainterEngine::DrawRect(const Rect &rect, float angle) {
        mPrivate->AddRect(rect, angle, mPalette);
    }

    void PainterEngine::DrawEllipse(const Point &center, float radiusX, float radiusY) {

    }

    void PainterEngine::DrawCircle(const Point &center, float radius) {

    }

    void PainterEngine::DrawImage(const Rect &target, float angle, const std::shared_ptr<Image> &image) {
        mPrivate->AddImage(target, angle, image, mPalette);
    }

    void PainterEngine::Clear() {
        mPrivate->Clear();
    }

    void PainterEngine::Draw() {
        mPrivate->Draw(mArea, mSpace, mWindow, mClearColor);
    }

    void PainterEngine::SetDrawArea(const Recti &area) {
        mArea = area;
    }

    void PainterEngine::SetDrawSpace(const Rect &space) {
        mSpace = space;
    }

    void PainterEngine::SetPenColor(const Color &color) {
        mPalette.penColor = color;
    }

    void PainterEngine::SetPenWidth(unsigned int width) {
        assert(width >= 1);
        mPalette.penWidth = width;
    }

    void PainterEngine::SetBrushColor(const Color &color) {
        mPalette.brushColor = color;
    }

    void PainterEngine::SetTransparentColor(const Color &color) {
        mPalette.transparentColor = color;
    }

    void PainterEngine::SetFilling(bool fill) {
        mPalette.filled = fill;
    }

    void PainterEngine::SetClearColor(const Color &color) {
        mClearColor = color;
    }

    void PainterEngine::SetDrawPalette(const Palette &palette) {
        mPalette = palette;
    }

    const PainterEngine::Recti & PainterEngine::GetDrawArea() const {
        return mArea;
    }

    const PainterEngine::Rect & PainterEngine::GetDrawSpace() const {
        return mSpace;
    }

    const PainterEngine::Palette & PainterEngine::GetDrawPalette() const {
        return mPalette;
    }

    const std::shared_ptr<Window> &PainterEngine::GetWindow() const {
        return mWindow;
    }

}