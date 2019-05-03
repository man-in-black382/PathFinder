//
//  Vertex1P.cpp
//  Geometry
//
//  Created by Pavlo Muratov on 10.04.17.
//  Copyright © 2017 MPO. All rights reserved.
//

#include "Vertex1P4.hpp"

namespace Geometry {

    Vertex1P4::Vertex1P4()
            :
            position(glm::vec4()) {
    }

    Vertex1P4::Vertex1P4(glm::vec4 p)
            :
            position(p) {
    }

}
