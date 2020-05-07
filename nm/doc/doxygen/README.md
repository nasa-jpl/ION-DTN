# Doxygen Documentation for Developers

To generate the full documentation simply run "doxygen" from this directory.


You will need the following to generate the complete documentation.  Only doxygen is required for text-only output:
- doxygen - Available from most package managers.  See http://doxygen.nl for more information
- graphviz - Available from most package managers.  Used to generate caller graphs.
- plantuml.jar - Used to generate additional UML diagrams.
  - Download from http://sourceforge.net/projects/plantuml/files/plantuml.jar/download into this directory.
  - Requires java.


## Files

- src - Documentation sources and resources included in Doxygen output
- output - After running doxygen, this directory will contain the output.
- Doxyfile - The Doxygen configuration file
