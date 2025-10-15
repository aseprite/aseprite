#!/bin/bash

# Aseprite Build and Install Script for macOS
# This script builds Aseprite and creates an app bundle in Applications folder

set -e  # Exit on any error

echo "🎨 Aseprite Build and Install Script for macOS"
echo "=============================================="

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Configuration
APP_NAME="Aseprite"
BUILD_DIR="$SCRIPT_DIR/build"
APP_BUNDLE="/Applications/$APP_NAME.app"
TEMP_APP_BUNDLE="$SCRIPT_DIR/aseprite/$APP_NAME.app"

echo "📁 Script directory: $SCRIPT_DIR"
echo "🔨 Build directory: $BUILD_DIR"
echo "📱 App bundle will be created at: $APP_BUNDLE"
echo ""

# Check if we're in the right directory
if [[ ! -f "EULA.txt" || ! -f ".gitmodules" ]]; then
    echo "❌ Error: Please run this script from the Aseprite source directory"
    exit 1
fi

# Check dependencies
echo "🔍 Checking dependencies..."

if ! command -v cmake &> /dev/null; then
    echo "❌ Error: cmake not found. Please install cmake:"
    echo "   brew install cmake"
    exit 1
fi

if ! command -v ninja &> /dev/null; then
    echo "❌ Error: ninja not found. Please install ninja:"
    echo "   brew install ninja"
    exit 1
fi

echo "✅ Dependencies check passed"
echo ""

# Build Aseprite using the automatic build script
echo "🔨 Building Aseprite..."
echo "This may take several minutes..."
echo ""

# Use printf to automatically select options: "N" for new build, "release" for build type, "aseprite" for name
if ! printf "N\nrelease\naseprite\n" | ./build.sh --auto --norun; then
    echo "❌ Error: Build failed"
    exit 1
fi

echo ""
echo "✅ Build completed successfully!"
echo ""

# Create app bundle structure
echo "📦 Creating app bundle..."

# Create aseprite directory if it doesn't exist
if [ ! -d "$SCRIPT_DIR/aseprite" ]; then
    mkdir -p "$SCRIPT_DIR/aseprite"
fi

# Remove existing temp app bundle if it exists
if [ -d "$TEMP_APP_BUNDLE" ]; then
    rm -rf "$TEMP_APP_BUNDLE"
fi

# Create app bundle directory structure
mkdir -p "$TEMP_APP_BUNDLE/Contents/MacOS"
mkdir -p "$TEMP_APP_BUNDLE/Contents/Resources"

# Copy the executable
cp "$SCRIPT_DIR/aseprite/bin/aseprite" "$TEMP_APP_BUNDLE/Contents/MacOS/aseprite"

# Copy data directory
cp -r "$SCRIPT_DIR/aseprite/bin/data" "$TEMP_APP_BUNDLE/Contents/Resources/"

# Create Info.plist
cat > "$TEMP_APP_BUNDLE/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>aseprite</string>
    <key>CFBundleIdentifier</key>
    <string>org.aseprite.aseprite</string>
    <key>CFBundleName</key>
    <string>Aseprite</string>
    <key>CFBundleDisplayName</key>
    <string>Aseprite</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>ASPR</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleIconFile</key>
    <string>aseprite</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>CFBundleDocumentTypes</key>
    <array>
        <dict>
            <key>CFBundleTypeExtensions</key>
            <array>
                <string>ase</string>
                <string>aseprite</string>
            </array>
            <key>CFBundleTypeName</key>
            <string>Aseprite Sprite</string>
            <key>CFBundleTypeRole</key>
            <string>Editor</string>
            <key>LSHandlerRank</key>
            <string>Owner</string>
        </dict>
        <dict>
            <key>CFBundleTypeExtensions</key>
            <array>
                <string>png</string>
                <string>gif</string>
                <string>jpg</string>
                <string>jpeg</string>
                <string>bmp</string>
                <string>tga</string>
                <string>webp</string>
            </array>
            <key>CFBundleTypeName</key>
            <string>Image Files</string>
            <key>CFBundleTypeRole</key>
            <string>Editor</string>
            <key>LSHandlerRank</key>
            <string>Alternate</string>
        </dict>
    </array>
</dict>
</plist>
EOF

# Create app icon from PNG files
ICON_CREATED=false

# Try to create ICNS from PNG using sips (built-in macOS tool)
if [ -f "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" ] && command -v sips &> /dev/null; then
    echo "🎨 Creating app icon from ase256.png..."
    
    # Create iconset directory
    ICONSET_DIR="$TEMP_APP_BUNDLE/Contents/Resources/aseprite.iconset"
    mkdir -p "$ICONSET_DIR"
    
    # Generate different icon sizes from the 256px PNG
    sips -z 16 16 "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" --out "$ICONSET_DIR/icon_16x16.png" >/dev/null 2>&1
    sips -z 32 32 "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" --out "$ICONSET_DIR/icon_16x16@2x.png" >/dev/null 2>&1
    sips -z 32 32 "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" --out "$ICONSET_DIR/icon_32x32.png" >/dev/null 2>&1
    sips -z 64 64 "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" --out "$ICONSET_DIR/icon_32x32@2x.png" >/dev/null 2>&1
    sips -z 128 128 "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" --out "$ICONSET_DIR/icon_128x128.png" >/dev/null 2>&1
    sips -z 256 256 "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" --out "$ICONSET_DIR/icon_128x128@2x.png" >/dev/null 2>&1
    sips -z 256 256 "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" --out "$ICONSET_DIR/icon_256x256.png" >/dev/null 2>&1
    sips -z 512 512 "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" --out "$ICONSET_DIR/icon_256x256@2x.png" >/dev/null 2>&1
    sips -z 512 512 "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" --out "$ICONSET_DIR/icon_512x512.png" >/dev/null 2>&1
    
    # Convert iconset to icns
    if iconutil -c icns "$ICONSET_DIR" -o "$TEMP_APP_BUNDLE/Contents/Resources/aseprite.icns" >/dev/null 2>&1; then
        echo "✅ App icon created successfully"
        ICON_CREATED=true
        # Clean up iconset directory
        rm -rf "$ICONSET_DIR"
    else
        echo "⚠️  Failed to create ICNS, will use PNG fallback"
        rm -rf "$ICONSET_DIR"
    fi
fi

# Fallback: copy PNG icons if ICNS creation failed
if [ "$ICON_CREATED" = false ]; then
    if [ -f "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" ]; then
        cp "$SCRIPT_DIR/aseprite/bin/data/icons/ase256.png" "$TEMP_APP_BUNDLE/Contents/Resources/icon.png"
        echo "📱 Using PNG icon as fallback"
    elif [ -f "$SCRIPT_DIR/aseprite/bin/data/icons/ase128.png" ]; then
        cp "$SCRIPT_DIR/aseprite/bin/data/icons/ase128.png" "$TEMP_APP_BUNDLE/Contents/Resources/icon.png"
        echo "📱 Using PNG icon as fallback"
    fi
fi

echo "✅ App bundle created"
echo ""

# Install to Applications folder
echo "📲 Installing to Applications folder..."

# Check if Applications folder is writable
if [ -w "/Applications" ]; then
    # Remove existing app if it exists
    if [ -d "$APP_BUNDLE" ]; then
        echo "🗑️  Removing existing Aseprite.app..."
        rm -rf "$APP_BUNDLE"
    fi
    
    # Move the app bundle to Applications
    mv "$TEMP_APP_BUNDLE" "$APP_BUNDLE"
    
    echo "✅ Aseprite has been installed to Applications folder!"
    echo ""
    echo "🎉 Installation completed successfully!"
    echo ""
    echo "You can now:"
    echo "• Find Aseprite in your Applications folder"
    echo "• Launch it from Spotlight (⌘+Space, then type 'Aseprite')"
    echo "• Add it to your Dock for easy access"
    echo ""
    echo "📂 App bundle location: $APP_BUNDLE"
    
    # Open Applications folder to show the new app
    echo "🔍 Opening Applications folder..."
    open /Applications
    
else
    echo "⚠️  Cannot write to /Applications folder. You can manually copy the app:"
    echo "   sudo mv '$TEMP_APP_BUNDLE' '$APP_BUNDLE'"
    echo "   Or drag '$TEMP_APP_BUNDLE' to your Applications folder"
fi

echo ""
echo "🎨 Enjoy using Aseprite!"