How to run SPIDER 2.0 unitary tests
===

The unitary tests of SPIDER 2.0 rely on the GTest framework.

# Install GTest framework 

To install GTest:
    - on Linux:
        ```
            sudo apt install libgtest-dev
            cd /usr/src/gtest
            sudo su
            cmake CMakeLists.txt
            make -j$(nproc)
            cp *.a /usr/lib
        ```
    - on Windows:
        __(coming soon...)__

With root permissions run the **install.sh** script.

# Run a specific test

Run the **runTest.sh** script in the folder scripts/ with the test name from the top-level folder.
ex: 
```
    ./scripts/linux/runTest.sh expression
```

# Run all tests

Run the following command from the top-level folder:
```
    ./scripts/linux/runTest.sh all
```

# Run the coverage script

**ATTENTION:** To run the coverage report you need to have gcovr installed.
To install gcovr:
    - on Linux:
        ```
            sudo apt install gcovr
        ```
    - on Windows:
        __(coming soon...)__

Run the following command from the top-level folder:
```
    ./scripts/linux/runTestWithCoverage.sh
```
The coverage report will be located in **bin/coverage** folder.
Open the **coverage.html** with the web browser of your choice.

