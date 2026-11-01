#include "extras/ControlsHost.h"
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace {
// FlyControls switches on physical KeyboardEvent.code, not layout-dependent
// virtual-key letters. These are standard PC set-1 scan positions. Extended
// navigation keys are distinct from the numeric keypad at the same scan code.
std::string sourceKey(LPARAM detail){const auto bits=static_cast<std::uintptr_t>(detail);const auto scan=(bits>>16)&0xff;const bool extended=(bits&(std::uintptr_t{1}<<24))!=0;if(extended){switch(scan){case 0x48:return "ArrowUp";case 0x50:return "ArrowDown";case 0x4b:return "ArrowLeft";case 0x4d:return "ArrowRight";default:return "Unidentified";}}switch(scan){case 0x2a:return "ShiftLeft";case 0x36:return "ShiftRight";case 0x11:return "KeyW";case 0x1f:return "KeyS";case 0x1e:return "KeyA";case 0x20:return "KeyD";case 0x13:return "KeyR";case 0x21:return "KeyF";case 0x10:return "KeyQ";case 0x12:return "KeyE";default:return "Unidentified";}}
}
bool forwardNativeControlsWin32Message(NativeControlsInput& input,void* handle,std::uint32_t message,std::uintptr_t key,std::intptr_t detail){
 const auto window=static_cast<HWND>(handle);RECT bounds{};if(window&&GetClientRect(window,&bounds)){const auto width=static_cast<double>(bounds.right-bounds.left),height=static_cast<double>(bounds.bottom-bounds.top);input.setWindowSize(width,height);input.setSurfaceBounds(0,0,width,height);}
 (void)key;
 switch(message){case WM_KEYDOWN:case WM_SYSKEYDOWN:input.keyDown(sourceKey(detail),(GetKeyState(VK_MENU)&0x8000)!=0);break;case WM_KEYUP:case WM_SYSKEYUP:input.keyUp(sourceKey(detail));break;case WM_MOUSEMOVE:input.pointerMove(static_cast<short>(detail&0xffff),static_cast<short>((detail>>16)&0xffff));break;case WM_LBUTTONDOWN:input.pointerDown(0);break;case WM_MBUTTONDOWN:input.pointerDown(1);break;case WM_RBUTTONDOWN:input.pointerDown(2);break;case WM_LBUTTONUP:input.pointerUp(0);break;case WM_MBUTTONUP:input.pointerUp(1);break;case WM_RBUTTONUP:input.pointerUp(2);break;case WM_CANCELMODE:input.pointerCancel();break;case WM_CONTEXTMENU:return input.contextMenu();default:break;}return false;
}
#else
bool forwardNativeControlsWin32Message(NativeControlsInput&,void*,std::uint32_t,std::uintptr_t,std::intptr_t){throw std::logic_error("Win32 controls input is unavailable on this platform");}
#endif
