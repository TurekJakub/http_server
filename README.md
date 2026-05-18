# HTTP_Server

## Building

To build this project create your build directory here inside `project` directory first, if already don't exists.

```shell
    mkdir <your_build_directory_name>
```

Then just run following command also inside `project` directory.

```shell
    cd <your_build_directory_name>
    cmake ..
    cmake --build .
```

After the build proceed, the resulting executables should be located inside `build/bin` subdirectory of `<your_build_directory_name>` (on Windows the executables are in same location, but there is one more extra `Debug` subdirectory in the directory structure)

**Note:** The project build needs Perl for it's configuration, which could posse a problem when building on some Windows machines where Perl is not standard part of the environment, in such cases build script will automatically download Strawberry Perl inside the build directory, this does not require any extra actions on users part, but it's probably something good to be aware of. Also the build process for Windows requires `nmake` and therefor is required to run build commands showed above from Visual Studio Developer Command Prompt (to ensure environment where all part of Microsoft toolchain are accessible, newer Visual Studio Developer PowerShell should work the same).
