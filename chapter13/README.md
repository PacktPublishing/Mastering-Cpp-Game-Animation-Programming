# Chapter 13

In this chapter you will learn how to add path finding and simple navigation
* Learn how to find adjacent triangles eligible for walking
* Understand and implement the A* path finding algorithm
* Add a simple logic and UI elements to make an instance walk along the generated path

## Controls

* Right mouse button: enable camera view
* Mouse: control view in camera movement mode
* W/A/S/D: move camera movemet foward/back/left/right
* SHIFT + W/A/S/D: faster camera movement
* Q/E: move camera up and down
* SHIFT + Q/E: faster camera movement
* Left mouse button: select instance
* 1/2/3: switch between move/rotate/scale mode of selected instance
* Middle mouse button:
  * move selected instance in X/Z direction in move mode
  * with SHIFT pressed, move selcted instance in Y direction
  * rotate selected instance around X/Z axes in rotation mode
  * with SHIFT presset, rotate instance around Y axis
  * scale selected instance in scale mode
* F10: toggle between edit and view mode
* F11: toggle between fullscreen and windowed display
* [ and ]: switch between configured cameras
* Mouse wheel: change FOV of current camera
* CTRL + Z: undo last action
* CTRL + Y: redo last undo-ed action
* CTRL + N: create new (empty) configuration
* CTRL + L: load existing configurattion
* CTRL + S: save current configuration to a file
* CTRL + Q: exit application
* W/A/S/D: walk with selected instance in view mode
* SHIFT + W/A/S/D: run with selected instance in view mode
* Mouse: rotate instance around Y axis in view mode
* Actions for selected instance in view mode:
  * Space bar: jump (Woman only when walking or running)
  * Space bar: hop (Woman only when idle)
  * E: punch (Man and Woman when idle)
  * P: pick up (Woman only when idle)
  * Q: kick (Man only when idle)
  * U: interact (Man only when idle)
  * F: wave (Man only when idle)
  * R: roll (Man only when running)
* Instance position window control
  * Left mouse button: move window
  * Middle mouse button: move instance octree/level minimap
  * Right mouse button: rotate instance octree/level minimap
  * Mouse wheel: zoom instance octree/level minimap in/out
