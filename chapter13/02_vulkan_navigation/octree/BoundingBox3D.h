/* based on https://github.com/pvigier/Quadtree
   see LICENSE file for original MIT license */

#pragma once

#include <glm/glm.hpp>

class BoundingBox3D {
  public:
    BoundingBox3D() : mPosition(glm::vec3(0.0f)), mSize(glm::vec3(0.0f)) {};
    BoundingBox3D(glm::vec3 pos, glm::vec3 size) : mPosition(pos), mSize(size) {};

    float getRight() const;
    float getBottom() const;
    float getBack() const;
    glm::vec3 getFrontTopLeft() const;
    glm::vec3 getSize() const;
    glm::vec3 getCenter() const;

    bool contains(BoundingBox3D otherBox);
    bool intersects(BoundingBox3D otherBox);

  private:
    glm::vec3 mPosition = glm::vec3(0.0f);
    glm::vec3 mSize = glm::vec3(0.0f);
};
