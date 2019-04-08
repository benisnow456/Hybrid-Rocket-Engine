# Hybrid-Rocket-Engine
This repository contains the code used for my senior thesis, which was designing, simulating, building, and testing a hybrid rocket engine.

The Arduino folder contains the .ino file that actually controls the engine. Communication with the engine is accomplished using the Arduino serial monitor and a dongle containing an Xbee radio module.

The MATLAB folder contains the .m class file used to simulate a burn, as well as an example of how to use the class. It also contains code from https://github.com/Progdrasil/CEAMatlabAPI, which is a MATLAB implementation of NASA's CEA software. The thermo.lib file contained in the folder is NOT the thermo.lib file in Progdrasil's github, however. I have modified it to contain ABS plastic as a reactant. 

Please make sure to read the full user guide for the engine, found in Appendix C of my thesis before even considering attempting to fire the engine. Also, my permanant contact info is listed in my thesis so please reach out to me if you have any questions.
