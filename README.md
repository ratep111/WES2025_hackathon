# Getting started

⚠️ **Make sure you have ESP-IDF v5.0.2 installed before continuing.**

After cloning the repo, run the project setup script:

     ./project_init.sh 

This will:

- Set up Git pre-commit hooks
- Initialize all submodules
- Apply required patches
- Check/install clang-format
- Prepare sdkconfig for the BL devkit

Then build the project with:

     idf.py build 
