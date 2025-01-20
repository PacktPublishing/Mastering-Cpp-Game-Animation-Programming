#include "BoundingBox3D.h"

float BoundingBox3D::getRight() const {
  return mPosition.x + mSize.x;
}

float BoundingBox3D::getBottom() const {
  return mPosition.y + mSize.y;
}

float BoundingBox3D::getBack() const {
  return mPosition.z + mSize.z;
}

glm::vec3 BoundingBox3D::getFrontTopLeft() const {
  return mPosition;
}

glm::vec3 BoundingBox3D::getSize() const {
  return mSize;
}

glm::vec3 BoundingBox3D::getCenter() const {
  return mPosition + mSize / 2.0f;
}

bool BoundingBox3D::contains(BoundingBox3D otherBox) {
  return
    mPosition.x <= otherBox.getFrontTopLeft().x &&
    otherBox.getRight() <= getRight() &&
    mPosition.y <= otherBox.getFrontTopLeft().y &&
    otherBox.getBottom() <= getBottom() &&
    mPosition.z <= otherBox.getFrontTopLeft().z &&
    otherBox.getBack() <= getBack();
}

bool BoundingBox3D::intersects(BoundingBox3D otherBox) {
  return !(
    mPosition.x >= otherBox.getRight() ||
    otherBox.getFrontTopLeft().x >= getRight() ||
    mPosition.y >= otherBox.getBottom() ||
    otherBox.getFrontTopLeft().y >= getBottom() ||
    mPosition.z >= otherBox.getBack() ||
    otherBox.getFrontTopLeft().z >= getBack()
  );
}
