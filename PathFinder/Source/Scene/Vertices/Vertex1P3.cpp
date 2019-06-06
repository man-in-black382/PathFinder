//
//  Vertex1P3.cpp
//  Geometry
//
//  Created by Pavlo Muratov on 21.06.17.
//  Copyright © 2017 MPO. All rights reserved.
//

#include "Vertex1P3.hpp"

namespace Geometry {

    Vertex1P3::Vertex1P3()
            :
            position(glm::vec3()) {
    }

    Vertex1P3::Vertex1P3(glm::vec3 p)
            :
            position(p) {
    }

}
