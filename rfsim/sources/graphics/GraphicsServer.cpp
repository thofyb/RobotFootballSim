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

#include <graphics/GraphicsServer.hpp>

namespace rfsim {

    static constexpr auto WHITE_COLOR = glm::vec4(1.0f);
    static constexpr auto NO_TRANSPARENT_COLOR = glm::vec4(2.0f);
    static constexpr auto SHADOW_COLOR = glm::vec4(0.1, 0.1, 0.1, 1.5f);

    GraphicsServer::GraphicsServer(const std::shared_ptr<Window> &window,
                                   const std::shared_ptr<PainterEngine> &painter,
                                   const std::string& resPath) {
        mWindow = window;
        mPainter = painter;
        mResPath = resPath;

        static const auto SEP = "/";
        static const auto ROBOT_IMAGE_PATH = "sprites/robot.png";
        static const auto FIELD_IMAGE_PATH = "sprites/play-field.png";
        static const auto BALL_IMAGE_PATH = "sprites/ball.png";
        static const auto BALL_IMAGE_TRSP_PATH = "sprites/soccer-ball.png";
        static const auto SHADOW_IMAGE_PATH = "sprites/shadow.png";
        static const auto ON_COLLISION_IMAGE_PATH = "sprites/hit.png";
        static const auto ON_OUT_IMAGE_PATH = "sprites/out-of-bounds.png";

        auto prefix = mResPath + SEP;

        mBallImage = Image::LoadFromFilePath(prefix + BALL_IMAGE_TRSP_PATH);
        mFieldImage = Image::LoadFromFilePath(prefix + FIELD_IMAGE_PATH);
        mOnCollisionImage = Image::LoadFromFilePath(prefix + ON_COLLISION_IMAGE_PATH);
        mOnOutImage = Image::LoadFromFilePath(prefix + ON_OUT_IMAGE_PATH);
        mShadowImage = Image::LoadFromFilePath(prefix + SHADOW_IMAGE_PATH);

        // todo: each robot will have unique image (realism)
        mRobotImages.push_back(Image::LoadFromFilePath(prefix + ROBOT_IMAGE_PATH));
    }

    void GraphicsServer::SetSettings(const GraphicsSettings &settings) {
        mSettings = settings;
    }

    void GraphicsServer::GetSettings(GraphicsSettings &settings) const {
        settings = mSettings;
    }

    void GraphicsServer::BeginGame(const GraphicsSceneSettings &sceneSettings) {
        assert(mState == InternalState::Default);
        mState = InternalState::InGame;

        mSceneSettings = sceneSettings;
    }

    void GraphicsServer::BeginDraw(const GraphicsGameState& gameState) {
        assert(mState == InternalState::InGame);
        mState = InternalState::InGameBeginDraw;
        mCurrentState = gameState;

        auto room00 = mSceneSettings.roomTopLeftBounds;
        auto roomWH = mSceneSettings.roomBottomRightBounds;

        // Prepare area
        mPainter->SetClearColor({ mSettings.backgroundColor, 1.0f });
        mPainter->Clear();
        mPainter->SetDrawSpace({ room00.x, room00.y, roomWH.x, roomWH.y });
    }

    void GraphicsServer::DrawStaticObjects() {
        assert(mState == InternalState::InGameBeginDraw);

        auto room00 = mSceneSettings.roomTopLeftBounds;
        auto roomWH = mSceneSettings.roomBottomRightBounds;
        auto size = roomWH - room00;

        mPainter->SetTransparentColor(NO_TRANSPARENT_COLOR);
        mPainter->SetBrushColor(WHITE_COLOR);
        mPainter->DrawImage({room00, size}, 0, mFieldImage);
    }

    void GraphicsServer::DrawDynamicObjects() {
        assert(mState == InternalState::InGameBeginDraw);

        // Draw robots
        for (const auto &r: mCurrentState.robots) {
            const auto radius = mSceneSettings.robotRadius;

            const PainterEngine::Rect rect = {
                r.position.x - radius,
                r.position.y - radius,
                radius * 2.0f,
                radius * 2.0f
            };

            if (mSettings.drawShadows) {
                mPainter->SetBrushColor(mSettings.shadowIntensity * SHADOW_COLOR);
                mPainter->SetTransparentColor(NO_TRANSPARENT_COLOR);
                mPainter->DrawImage(rect + glm::vec4{0,radius * 0.7,0,0}, 0, mShadowImage);
            }

            mPainter->SetBrushColor(WHITE_COLOR);
            mPainter->SetTransparentColor(WHITE_COLOR);
            mPainter->DrawImage(rect, r.angle, mRobotImages.back());
        }

        // Draw ball
        {
            const auto &b = mCurrentState.ball;
            const auto radius = mSceneSettings.ballRadius;

            const PainterEngine::Rect rect = {
                    b.position.x - radius,
                    b.position.y - radius,
                    radius * 2.0f,
                    radius * 2.0f
            };

            if (mSettings.drawShadows) {
                mPainter->SetBrushColor(mSettings.shadowIntensity * SHADOW_COLOR);
                mPainter->SetTransparentColor(NO_TRANSPARENT_COLOR);
                mPainter->DrawImage(rect + glm::vec4{0,radius * 0.8,0,0}, 0, mShadowImage);
            }

            mPainter->SetBrushColor(WHITE_COLOR);
            mPainter->SetTransparentColor(NO_TRANSPARENT_COLOR);
            mPainter->DrawImage(rect, b.angle, mBallImage);
        }
    }

    void GraphicsServer::DrawAuxInfo() {
        assert(mState == InternalState::InGameBeginDraw);

        // Draw info about collisions on top of the robots
        if (mSettings.drawCollisionInfo) {
            for (const auto &c : mCurrentState.robotRobotCollisions) {
                const float factor = 2.0f;
                const float size = mSceneSettings.robotRadius * 2 * factor;

                const auto &ra = mCurrentState.robots[c.robotIdA];
                const auto &rb = mCurrentState.robots[c.robotIdB];

                const PainterEngine::Rect rectA = {
                    ra.position.x - size / 2,
                    ra.position.y - size / 2,
                    size, size
                };

                const PainterEngine::Rect rectB = {
                    rb.position.x - size / 2,
                    rb.position.y - size / 2,
                    size, size
                };

                mPainter->DrawImage(rectA, 0, mOnCollisionImage);
                mPainter->DrawImage(rectB, 0, mOnCollisionImage);
            }
        }

        // Draw info about robots, which leave the area of the field
        if (mSettings.drawOutInfo) {
            for (int id : mCurrentState.robotFieldBoundsCollisions) {
                const float factor = 2.0f;
                const float size = mSceneSettings.robotRadius * 2 * factor;

                const auto &r = mCurrentState.robots[id];

                const PainterEngine::Rect rect = {
                    r.position.x - size / 2.0f,
                    r.position.y - size / 2.0f,
                    size, size
                };

                mPainter->DrawImage(rect, 0, mOnOutImage);
            }
        }
    }

    void GraphicsServer::DrawPostUI() {
        assert(mState == InternalState::InGameBeginDraw);

        // Draw something, what must be placed on top of the UI
    }

    void GraphicsServer::EndDraw() {
        assert(mState == InternalState::InGameBeginDraw);
        mState = InternalState::InGame;

        // Commit draw
        mPainter->Draw();
    }

    void GraphicsServer::EndGame() {
        assert(mState == InternalState::InGame);
        mState = InternalState::Default;
    }

}