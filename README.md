# Mastering C++ Game Animation Programming, First Edition</h1>
This is the code repository for [Mastering C++ Game Animation Programming, First Edition](https://www.packtpub.com/en-us/product/mastering-c-game-animation-programming-9781835881934), published by Packt.

**Enhance your skills with advanced game animation techniques in C++, OpenGL, and Vulkan**

<p align="center">
   <a href="https://packt.link/cppgameanimation" alt="Discord" title="Learn more on the Discord server"><img width="32px" src="https://cliply.co/wp-content/uploads/2021/08/372108630_DISCORD_LOGO_400.gif"/></a>
  &#8287;&#8287;&#8287;&#8287;&#8287;
  <a href="https://packt.link/free-ebook/9781835881927"><img width="32px" alt="Free PDF" title="Free PDF" src="https://cdn-icons-png.flaticon.com/512/4726/4726010.png"/></a>
 &#8287;&#8287;&#8287;&#8287;&#8287;
  <a href="https://packt.link/gbp/9781835881927"><img width="32px" alt="Graphic Bundle" title="Graphic Bundle" src="https://cdn-icons-png.flaticon.com/512/2659/2659360.png"/></a>
  &#8287;&#8287;&#8287;&#8287;&#8287;
   <a href="https://www.amazon.com/Mastering-Game-Animation-Programming-techniques/dp/1835881920"><img width="32px" alt="Amazon" title="Get your copy" src="https://cdn-icons-png.flaticon.com/512/15466/15466027.png"/></a>
  &#8287;&#8287;&#8287;&#8287;&#8287;
</p>

## About the book
<a href="https://www.packtpub.com/en-us/product/mastering-c-game-animation-programming-9781835881934">
<img src="https://content.packt.com/_/image/original/B22428/cover_image.jpg?version=1743150629" alt="Mastering C++ Game Animation Programming, First Edition" height="256px" align="right">
</a>

With two decades of programming experience across multiple languages and platforms, expert game developer and console porting programmer Michael Dunsky guides you through the intricacies of character animation programming. This book tackles the common challenges developers face in creating sophisticated, efficient, and visually appealing character animations.
You’ll learn how to leverage the Open Asset Import Library for easy 3D model loading and optimize your 3D engine by offloading computations from the CPU to the GPU. The book covers visual selection, extended camera handling, and separating your application into edit and simulation modes. You’ll also master configuration storage to progressively build your virtual world piece by piece.
As you develop your engine-like application, you’ll implement collision detection, inverse kinematics, and expert techniques to bring your characters to life with realistic visuals and fluid movement. For more advanced animation and character behavior controls, you’ll design truly immersive and responsive NPCs, load real game maps, and use navigation algorithms, enabling the instances to roam freely in complex environments.
By the end of this book, you’ll be skilled at designing interactive virtual worlds inhabited by lifelike NPCs that exhibit natural, context-aware behaviors.</details>

## Key Learnings
* Master the basics of the Open Asset Import Library
* Animate thousands of game characters
* Extend ImGui with more advanced control types
* Implement simple configuration file handling
* Explore collision detection between 3D models and world objects
* Combine inverse kinematics and collision detection
* Work with state machines, behavior trees, and interactive NPC behaviors
* Implement navigation for NPC movement in unknown terrains


## Chapters
<img src="https://cliply.co/wp-content/uploads/2020/02/372002150_DOCUMENTS_400px.gif" alt="Unity Cookbook, Fifth Edition" height="556px" align="right">

1. Working with Open Asset Import Library
1. Moving Animation Calculations from CPU to GPU
1. Adding a Visual Selection
1. Enhancing Application Handling
1. Saving and Loading the Configuration
1. Extending Camera Handling
1. Enhancing Animation Controls
1. An Introduction to Collision Detection
1. Adding Behavior and Interaction
1. Advanced Animation Blending
1. Loading a Game Map
1. Advanced Collision Detection
1. Adding Simple Navigation
1. Creating Immersive Interactive Worlds


## Instructions and Navigations
All of the code is organized into folders.

### Software and Hardware List

| Chapter | Software required | OS required |
| -------- | ------------------------------------ | ----------------------------------- |
| 1-15 | Visual Studio 2022 or Eclipse 2023-06 (or newer) | Windows 10 (or newer), or Linux (i.e. Ubuntu 22.04, Debian 12, or newer) |

## Get to know the Author

**Michael Dunsky** is an electronics engineer, console porting programmer, and game developer with more than 20 years of programming experience. He started with BASIC at the young age of 14 and expanded his portfolio over the years to include assembly language, C, C++, Java, Python, VHDL, OpenGL, GLSL, and Vulkan. During his career, he also gained extensive knowledge of Linux, virtual machines, server operation, and infra-structure automation. Michael holds a Master of Science degree in Computer Sci-ence from the FernUniversität in Hagen with a focus on computer graphics, parallel programming, and software systems.


## Other Related Books
* [Vulkan 3D Graphics Rendering Cookbook, Second Edition](https://www.packtpub.com/en-us/product/vulkan-3d-graphics-rendering-cookbook-second-edition/9781803248110)
* [C++ Game Animation Programming, Second Edition](https://www.packtpub.com/en-us/product/c-game-animation-programming-second-edition/9781803246529)

## Author Notes & Recent Fixes

The following section contains updates within the text of the book.

### Chapter 1

Building the code by using the top-level `CMakeLists.txt` file will fail for the code in Chapter 14 if the two libraries `SDL` and `SDL_mixer` are not installed.
Please follow the instructions in section **Installing SDL and SDL_mixer on Windows** on page **459** and/or **Installing SDL and SDL_mixer on Linux** on page **460** before building the code for all chapters.
