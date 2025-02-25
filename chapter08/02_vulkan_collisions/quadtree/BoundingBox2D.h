/* based on https://github.com/pvigier/Quadtree
   see LICENSE file for original MIT license */

#pragma once

#include <glm/glm.hpp>

class BoundingBox2D {
  public:
    BoundingBox2D() : mPosition(glm::vec2(0.0f)), mSize(glm::vec2(0.0f)) {};
    BoundingBox2D(glm::vec2 pos, glm::vec2 size) : mPosition(pos), mSize(size) {};

    float getRight() const;
    float getBottom() const;
    glm::vec2 getTopLeft() const;
    glm::vec2 getSize() const;
    glm::vec2 getCenter() const;

    bool contains(BoundingBox2D otherBox);
    bool intersects(BoundingBox2D otherBox);

  private:
    glm::vec2 mPosition = glm::vec2(0.0f);
    glm::vec2 mSize = glm::vec2(0.0f);
};
