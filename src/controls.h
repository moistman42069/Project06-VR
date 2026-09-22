#pragma once
#include <cstdint>
struct Controls { float lx=0,ly=0,rx=0,ry=0,lt=0,rt=0,lg=0,rg=0; uint32_t buttons=0; bool focused=false,blocked=false; };
enum Button : uint32_t { A=1,B=2,X=4,Y=8,Menu=16,LClick=32,RClick=64,LB=128,RB=256,LT=512,RT=1024 };
