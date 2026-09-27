#pragma once
#include <cmath>
// Local yaw * pitch * roll; zero offsets preserve the accepted aim orientation.
inline void P06HandAdjustment(const float degrees[3],float q[4]){
    constexpr float halfRadians=.008726646259971648f;
    float sx=std::sin(degrees[0]*halfRadians),cx=std::cos(degrees[0]*halfRadians);
    float sy=std::sin(degrees[1]*halfRadians),cy=std::cos(degrees[1]*halfRadians);
    float sz=std::sin(degrees[2]*halfRadians),cz=std::cos(degrees[2]*halfRadians);
    q[0]=cy*sx*cz+sy*cx*sz;q[1]=sy*cx*cz-cy*sx*sz;
    q[2]=cy*cx*sz-sy*sx*cz;q[3]=cy*cx*cz+sy*sx*sz;
}
