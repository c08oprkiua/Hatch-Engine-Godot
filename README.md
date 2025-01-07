# Hatch-Engine-Godot
An engine module that aims to add full support for HatchGameEngine to Godot 4.

This is a C++ engine module for Godot 4 (4.4 tested, other versions may or may not work) that aims to add full compatibility with [HatchGameEngine](https://github.com/HatchGameEngine/HatchGameEngine/tree/master) to Godot 4, including:
* Running Hatch projects
* Accessing loading, and interacting with Hatch files

# Features
Currently, it allows the user to open a Hatch archive (`.hatch`) and examine the files therein.

# Usage
Copy the `hatch/` folder into the `modules/` folder of a copy of the Godot source code, and then compile Godot.