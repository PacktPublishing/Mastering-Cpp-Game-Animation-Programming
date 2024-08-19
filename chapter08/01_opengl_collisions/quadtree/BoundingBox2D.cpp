#include "BoundingBox2D.h"

float BoundingBox2D::getRight() const {
  return mPosition.x + mSize.x;
}

float BoundingBox2D::getBottom() const {
  return mPosition.y + mSize.y;
}

glm::vec2 BoundingBox2D::getTopLeft() const {
  return mPosition;
}

glm::vec2 BoundingBox2D::getSize() const {
  return mSize;
}

glm::vec2 BoundingBox2D::getCenter() const {
  return mPosition + mSize / 2.0f;
}

bool BoundingBox2D::contains(BoundingBox2D otherBox) {
  return
    otherBox.getTopLeft().x <= mPosition.x &&
    getRight() <= otherBox.getRight() &&
    otherBox.getTopLeft().y <= mPosition.y &&
    getBottom() <= otherBox.getBottom();
}

bool BoundingBox2D::intersects(BoundingBox2D otherBox) {
  return !(
    mPosition.x >= otherBox.getRight() ||
    otherBox.getTopLeft().x >= getRight() ||
    mPosition.y >= otherBox.getBottom() ||
    otherBox.getTopLeft().y >= getBottom()
  );
}
