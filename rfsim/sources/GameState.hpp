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

#ifndef RFSIM_GAMESTATE_HPP
#define RFSIM_GAMESTATE_HPP

#include <glm/vec2.hpp>
#include <vector>

namespace rfsim {

    struct RobotInitInfo {
        int         id;
        glm::vec2   position;
        // Angle in radians
        float       angle;
    };

    struct BodyState {
        glm::vec2   position = {};
        glm::vec2   velocity = {};
        float       angle = 0;
    };

    struct RobotCollisionInfo {
        int robotIdA = -1;
        int robotIdB = -1;
    };

    class GameState {
    public:
        struct Robot {
            glm::vec2 position{};
            float angle = 0.0f;
        };

        struct Ball {
            glm::vec2 position{};
        };

        unsigned int teamSize = 0;
        unsigned int scoreTeam1 = 0;
        unsigned int scoreTeam2 = 0;
        std::vector<unsigned int> robotsTeam1;
        std::vector<unsigned int> robotsTeam2;
        std::vector<Robot> robots;
        Ball ball;
    };

}

#endif //RFSIM_GAMESTATE_HPP