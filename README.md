# Lexical-and-Syntax-Analyzer

# Python Syntax Analyzer (C++/Qt Project)

This project is a Python source code analyzer written in C++ with a graphical user interface (GUI) built using Qt. It provides tools for lexical analysis (tokenizing), syntax analysis (parsing), and parse tree visualization of Python code. The project is structured for cross-platform development using CMake.This project was a group project in college

----------------------------------------------------------------------------------------------------------------------------------------------------------------

## Features

- **Python Lexer**: Analyzes and tokenizes Python source code.
- **Syntax Analyzer**: Parses tokens to check for syntactic correctness.
- **Parse Tree Display**: Visualizes the parse tree for better understanding of code structure.
- **Graphical User Interface**: Easy-to-use GUI for code input, analysis, and visualization.
- **Modular Structure**: Code is organized into separate components for maintainability.

----------------------------------------------------------------------------------------------------------------------------------------------------------------
## Project Structure (in case you need ot understand the files)

| File/Folder            | Description                                                   
|------------------------|---------------------------------------------------------------|                   
| `CMakeLists.txt`       | CMake build configuration.                                    |
| `main.cpp`             | Application entry point.                                      |
| `mainwindow.cpp/h`     | Main window logic and definitions for the GUI.                |
| `mainwindow.ui`        | Qt Designer XML file for GUI layout.                          |
| `parsetreedisplay.cpp/h` | Parse tree visualization logic and headers.                 |
| `pythonlexer.cpp/h`    | Lexical analysis logic and definitions for Python code.       |
| `syntaxanalyzer.cpp/h` | Syntax analysis logic and definitions.                        |


---------------------------------------------------------------------------------------------------------------------------------------------------------------
# Getting Started

## Prerequisites

- **C++ Compiler** (e.g., GCC, Clang, MSVC)
- **CMake** (version 3.10 or newer)
- **Qt** (version 5 or 6, for GUI components)

## Build Instructions

1. **Clone the repository**
   ```bash
   git clone <your-repository-url>
   cd <your-repository-name>
   ```

2. **Create and enter the build directory**
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake**
   ```bash
   cmake ..
   ```

4. **Build the project**
   ```bash
   cmake --build .
   ```

5. **Run the application**
   - The executable will be available in the `build` directory.
   - You can launch it from your terminal or by double-clicking the executable.



## Usage

1. Launch the application.
2. Paste or type your Python code into the provided text area.
3. Select operations to:
    - Perform lexical analysis (view tokens).
    - Perform syntax analysis (check for syntax errors).
    - Visualize the parse tree.
4. Review outputs in the GUI.



## Contributing

Contributions are welcome! Please fork the repository and submit a pull request with your changes. For suggestions or bug reports, please open an issue.


## Author(s)
Mohamed Abdelhamid
My teammates from college





------------------------------------------------------------------------------------------------------------------------------------------------------------------
