# Lexical-and-Syntax-Analyzer

# Python Lexical and Syntax Analyzer (Using C++/Qt Project)

This project is a Python source code analyzer written in C++ with a graphical user interface (GUI) built using Qt. It provides tools for lexical analysis (tokenizing), syntax analysis (parsing), and parse tree visualization of Python code. The project is structured for cross-platform development using CMake.This project was a group project in college

----------------------------------------------------------------------------------------------------------------------------------------------------------------

## Features

- **Python Lexer**: Analyzes and tokenizes Python source code.
- **Syntax Analyzer**: Parses tokens to check for syntactic correctness.
- **Parse Tree Display**: Visualizes the parse tree for better understanding of code structure.
- **Graphical User Interface**: Easy-to-use GUI for code input, analysis, and visualization.
- **Modular Structure**: Code is organized into separate components for maintainability.

----------------------------------------------------------------------------------------------------------------------------------------------------------------
## Project Structure (in case you need to understand the files)

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
---------------------------------------------------------------------------------------------------------------------------------------------------------------
## Build Instructions

1. **Clone the repository**
   ```bash
   git clone https://github.com/MohamedAbdelhamid0/Lexical-and-Syntax-Analyzer.git
   cd Lexical-and-Syntax-Analyzer
   ```

2. **Create and enter the build directory**
   ```bash
   mkdir build
   cd build
   ```

3. **Configure the project with CMake**
   ```bash
   cmake ..
   ```

4. **Build the project**
   ```bash
   cmake --build .
   ```

5. **Run the application**
   - The executable will appear in the `build` directory.
   - Launch it from your terminal or by double-clicking in your file explorer.
---------------------------------------------------------------------------------------------------------------------------------------------------------------

## Usage

1. Launch the application.
2. Paste or type your Python code into the provided text area.
3. Select operations to:
    - Perform lexical analysis (view tokens).
    - Perform syntax analysis (check for syntax errors).
    - Visualize the parse tree.
4. Review outputs in the GUI.

---------------------------------------------------------------------------------------------------------------------------------------------------------------

# Contributing

Contributions are welcome! Please fork the repository and submit a pull request with your changes. For suggestions or bug reports, please open an issue.

---------------------------------------------------------------------------------------------------------------------------------------------------------------
# Screenshots
![WhatsApp Image 2025-05-17 at 00 22 49_53af227d](https://github.com/user-attachments/assets/566ba965-396b-4325-85f1-516702ebaf87)
![WhatsApp Image 2025-05-17 at 00 24 03_a2395918](https://github.com/user-attachments/assets/a3e17141-75b4-4083-97d6-fa4d079ddae3)
![WhatsApp Image 2025-05-17 at 00 24 04_b34ba47c](https://github.com/user-attachments/assets/9cd13ded-44f3-4d91-90b6-b28bdcae4944)
![WhatsApp Image 2025-05-17 at 00 22 49_2eb3e7b4](https://github.com/user-attachments/assets/a82ed074-41f2-45bf-9965-57f72deb2a3e)
![WhatsApp Image 2025-05-17 at 00 22 50_c9c959e9](https://github.com/user-attachments/assets/70503d46-ccfa-4b68-bfb2-dfc2ec0453dc)
![WhatsApp Image 2025-05-17 at 00 26 41_41aed4bf](https://github.com/user-attachments/assets/5446b664-65d9-4c00-aa5d-dd0d42e78241)
![WhatsApp Image 2025-05-17 at 00 26 41_03cf6af9](https://github.com/user-attachments/assets/1881ea0c-d63f-49b7-ae11-eb4299b24a0c)




---------------------------------------------------------------------------------------------------------------------------------------------------------------

