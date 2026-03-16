# iOS Toolchain for Aseprite
# Sets up cross-compilation for iOS arm64 devices

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum iOS version")
set(CMAKE_OSX_SYSROOT iphoneos)

# Use Xcode's default compiler
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# iOS-specific settings
set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH YES)
set(CMAKE_IOS_INSTALL_COMBINED NO)

# Tell CMake this is a cross-compile
set(CMAKE_CROSSCOMPILING TRUE)

# Force static libraries for iOS
set(BUILD_SHARED_LIBS OFF)
