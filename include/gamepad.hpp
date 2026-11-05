#pragma once
// Game controller input for desktop builds. Buttons are translated to the same
// logical set the Switch build reads from libnx, so both share one control map.
#ifndef __SWITCH__
#include <SDL.h>
namespace xs {
enum class Pad {None,Up,Down,Left,Right,A,B,X,Y,L,R,ZL,ZR,Plus,Minus,R3};
// Call for every SDL event. Opens/closes controllers on hot-plug and returns the
// logical button that was just pressed (Pad::None for anything else).
inline Pad gamepad_translate(const SDL_Event& event){
    static int stick_x=0,stick_y=0;static bool zl=false,zr=false;
    constexpr int threshold=16000;
    switch(event.type){
        case SDL_CONTROLLERDEVICEADDED:SDL_GameControllerOpen(event.cdevice.which);return Pad::None;
        case SDL_CONTROLLERDEVICEREMOVED:
            if(auto* c=SDL_GameControllerFromInstanceID(event.cdevice.which))SDL_GameControllerClose(c);
            return Pad::None;
        case SDL_CONTROLLERBUTTONDOWN:
            switch(event.cbutton.button){
                case SDL_CONTROLLER_BUTTON_DPAD_UP:return Pad::Up;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:return Pad::Down;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:return Pad::Left;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:return Pad::Right;
                case SDL_CONTROLLER_BUTTON_A:return Pad::A;
                case SDL_CONTROLLER_BUTTON_B:return Pad::B;
                case SDL_CONTROLLER_BUTTON_X:return Pad::X;
                case SDL_CONTROLLER_BUTTON_Y:return Pad::Y;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:return Pad::L;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:return Pad::R;
                case SDL_CONTROLLER_BUTTON_START:return Pad::Plus;
                case SDL_CONTROLLER_BUTTON_BACK:return Pad::Minus;
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:return Pad::R3;
                default:return Pad::None;
            }
        case SDL_CONTROLLERAXISMOTION:{
            const int value=event.caxis.value;
            switch(event.caxis.axis){
                case SDL_CONTROLLER_AXIS_LEFTX:{const int now=value<-threshold?-1:value>threshold?1:0;
                    const bool edge=now!=0&&now!=stick_x;stick_x=now;if(edge)return now<0?Pad::Left:Pad::Right;break;}
                case SDL_CONTROLLER_AXIS_LEFTY:{const int now=value<-threshold?-1:value>threshold?1:0;
                    const bool edge=now!=0&&now!=stick_y;stick_y=now;if(edge)return now<0?Pad::Up:Pad::Down;break;}
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:{const bool now=value>threshold;const bool edge=now&&!zl;zl=now;if(edge)return Pad::ZL;break;}
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:{const bool now=value>threshold;const bool edge=now&&!zr;zr=now;if(edge)return Pad::ZR;break;}
                default:break;
            }
            return Pad::None;
        }
        default:return Pad::None;
    }
}
}
#endif
