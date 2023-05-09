# Cross-compiling for macOS on a Linux host
## Prerequisites

In order to cross-compile for macOS, you'll need to first build and install a copy of the [osxcross](https://github.com/tpoechtrager/osxcross) project as well as an applicable [macOS SDK](https://developer.apple.com/download/all/?q=xcode) to build it (and `dxx-rebirth`) against.

The Command Line Tools bundles from Apple are the most efficient to work with, as they don't include SDKs for other platforms or Xcode.  The "Command Line Tools for Xcode 14" package is known to work for this process.

There will also be several packages you'll need to install from your Linux distribution in order to successfully build osxcross.  The package names will vary by distribution, but tested package installation commands are:

Alma Linux 9: `yum install llvm clang pkgconf-pkg-config cmake cpio git patch python3 python3-pip openssl-devel xz-devel bzip2-devel libxml2-devel bash wget openssl`

Ubuntu 22.04: `apt-get install llvm clang pkg-config cmake cpio git patch python3 python3-pip libssl-dev liblzma-dev libbz2-dev libxml2-dev bash wget openssl`

### Obtaining the SDK

Before building osxcross, you'll need a copy of the macOS SDK.  Download a copy of the Command Line Tools from the link above (requires registration with Apple) and you'll have a .dmg file containing the SDK, among other tools for macOS.

### Preparing the SDK

In order for osxcross to be able to use the SDK, it needs to be preprocessed and packaged.  osxcross has tools to do this.  In order to run them, clone the repository into a directory and change into it.  From there, run:

`./tools/gen_sdk_package_tools_dmg.sh <command_line_tools_for_xcode>.dmg`

Once this completes, you'll have a .tar.xz file for each of the SDKs in the DMG.  Select one (such as the most recent one; 12.3 is known to work) and move it to the `tarballs` directory.

### Building and installing osxcross

At this point, you can begin building osxcross.  After deciding where the installed tools should be installed on your system, run the following:

`TARGET_DIR=/path/to/osxcross/installation/location ./build.sh`

There will be a confirmation of the settings you're using to build osxcross, so if everything looks correct, press enter to begin the build process.

After the install process is complete, you may want to edit your shell login script to add the `bin` directory in your osxcross install location to your path, but this is optional.

### Setting up osxcross-macports

In order to build dxx-rebirth, the osxcross compiler needs to be able to link against macOS libraries for the dependencies.  osxcross includes tooling to obtain packages from the [MacPorts](https://www.macports.org) project.  Setting up the package manager requires setting the `MACOSX_DEPLOYMENT_TARGET` environment variable and, depending on your version of OpenSSL, editing the osxcross-macports script.

`MACOSX_DEPLOYMENT_TARGET` should be set to `12.0`, `11.0`, or some other macOS version which will be the minimum supported macOS version for libraries used by it.  This can be set in a shell login script and exported to ensure the same version is consistently used.  ***Changing this value will require reinstalling all of the MacPorts packages*** (but will not require reinstalling osxcross).

Note that if you have OpenSSL 3.0.0 through 3.0.6, you'll need to make a change to your copy of `osxcross-macports` per [this bug](https://github.com/tpoechtrager/osxcross/issues/349).  OpenSSL versions below 3.0.0 and above 3.0.6 are unaffected.

### Installing MacPorts packages

The following command assumes the osxcross `bin` directory is in your path.  If that's not the case, you'll need to explicitly specify the path to it.

`osxcross-macports install libsdl2 libsdl2_image libsdl2_mixer libpng jpeg physfs`

### Installing scons

`scons` is also required, which can be installed with:

`pip install scons`

## Building dxx-rebirth

At this point, the prerequisites are in place, and dxx-rebirth can be built.  Note that the `macos_bundle_libs` depends on a tool (`dylibbundler`) which does not run on non-macOS hosts.  As a result, builds from this process will depend on libraries existing in locations defined by the MacPorts project.  However, it is possible to copy the libraries to a macOS host and either package them with `dylibbundler` on that host or run the resulting binary by placing them in the same locations as the MacPorts project does (`/opt/local/lib`).  The specific location to place libraries on a Mac for `dylibbundler` to pick them up will be irrelevant once [this `dylibbundler` bug](https://github.com/auriamg/macdylibbundler/issues/82) is resolved.

To build dxx-rebirth, you'll need to know which C++ compiler you built.  This will look something like `x86_64-apple-darwin21.4-clang++` or `aarch64-apple-darwin21.4-clang++`.  The `darwin21.4` part may change depending on the SDK you built osxcross with, as each macOS SDK version targets a specific maximum Darwin version and osxcross allows for having multiple concurrent versions installed simultaneously.

Similarly, you'll also need to know the location of the osxcross-installed `pkg-config` command.  This will look something like `x86_64-apple-darwin21.4-pkg-config` or `aarch64-apple-darwin21.4-pkg-config`.  Again, the Darwin version may change depending on the macOS SDK used to build osxcross.

Once all of these are put together, you can switch to the location where you have the dxx-rebirth source and build it with a command that sets the `CXX` and `PKG_CONFIG` variables and runs `scons`:

`CXX=x86_64-apple-darwin21.4-clang++ PKG_CONFIG=x86_64-apple-darwin21.4-pkg-config scons macos_add_frameworks=False macos_bundle_libs=False host_platform=darwin`

This will result in a `build` directory containing a `D1X-Rebirth.app` directory and a `D2X-Rebirth.app` directory.  The `.app` directories themselves are seen by macOS as applications, so they can be transferred to a Mac by compressing them into a ZIP file or similar and copying them.