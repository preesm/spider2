How to run SPIDER 2.0 unitary tests
===

The unitary tests of SPIDER 2.0 rely on the GTest framework.

# Configuring project with tests building

Make sure that during the cmake configuration the **BUILD_TESTING** flag was set to ON.
If not, re-run your cmake command with the following addition:
```shell
    cmake .. [YOUR_CMAKE_OPTIONS] -DBUILD_TESTING=ON
```

# Run a specific test

Run the **runTest.sh** script in the folder scripts/ with the test name from the top-level folder.
ex: 
```shell
    ./scripts/linux/runTest.sh expression
```

# Run all tests

Run the following command from the top-level folder:
```shell
    ./scripts/linux/runTest.sh all
```

# Run the coverage script

**ATTENTION:** To run the coverage report you need to have gcovr installed.
To install gcovr:
- on Linux:
```shell
    sudo apt install gcovr
```
- on Windows:
__(coming soon...)__

Run the following command from the top-level folder:
```shell
    ./scripts/linux/runTestWithCoverage.sh
```
The coverage report will be located in **bin/coverage** folder.
Open the **coverage.html** with the web browser of your choice.

