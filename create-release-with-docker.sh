#!/usr/bin/env bash
#
# build-in-docker.sh
# Compile Aseprite for Linux inside a minimal Docker container.
#
# This script:
#   1. Downloads pre-compiled Skia (from aseprite/skia releases)
#   2. Compiles Aseprite with clang + libstdc++ (ABI-compatible with Skia)
#   3. Extracts the final binary to ./build-output/
#   4. Optionally starts an FTP server to serve the compiled binary
#
# Usage:
#   chmod +x build-in-docker.sh
#   ./build-in-docker.sh            # build only
#   ./build-in-docker.sh --ftp      # build + start FTP server automatically
#   ./build-in-docker.sh ftp        # start FTP server only (if already built)
#

set -euo pipefail

# ── Parse arguments ──────────────────────────────────────────────────────────

AUTO_FTP=false
FTP_ONLY=false

for arg in "$@"; do
    case "$arg" in
        --ftp)  AUTO_FTP=true ;;
        ftp)    FTP_ONLY=true ;;
    esac
done

# ── Sanity checks ───────────────────────────────────────────────────────────

if ! command -v docker &>/dev/null; then
    echo "ERROR: Docker is not installed or not in PATH."
    echo "Install Docker: https://docs.docker.com/engine/install/"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Verify we're at the root of an Aseprite checkout.
if [[ ! -f "EULA.txt" || ! -f "CMakeLists.txt" ]]; then
    echo "ERROR: Run this script from the root of the Aseprite source tree."
    exit 1
fi

# Ensure submodules are initialized.
if [[ ! -f "laf/CMakeLists.txt" ]]; then
    echo "Initializing git submodules..."
    git submodule update --init --recursive
fi

# ── Skia download info ──────────────────────────────────────────────────────

SKIA_TAG=$(cat laf/misc/skia-tag.txt | tr -d '[:space:]')
SKIA_ZIP="Skia-Linux-Release-x64.zip"
SKIA_URL="https://github.com/aseprite/skia/releases/download/${SKIA_TAG}/${SKIA_ZIP}"

BUILD_DIR="$SCRIPT_DIR/.docker-build"
OUTPUT_DIR="$SCRIPT_DIR/build-output"

if $FTP_ONLY; then
    echo "============================================================"
    echo " Aseprite FTP Server (pre-built binary)"
    echo "============================================================"
    echo ""
else
    echo "============================================================"
    echo " Aseprite Docker Build"
    echo "============================================================"
    echo " Skia tag : ${SKIA_TAG}"
    echo " Skia url : ${SKIA_URL}"
    echo ""

    # Verify the Skia download URL is reachable.
    echo "Verifying Skia download URL..."
    HTTP_CODE=$(curl -sIL --ssl-revoke-best-effort \
                -o /dev/null -w "%{http_code}" "${SKIA_URL}" 2>/dev/null || echo "000")
    if [[ "$HTTP_CODE" != "200" ]]; then
        echo "ERROR: Skia download returned HTTP ${HTTP_CODE}"
        echo "URL: ${SKIA_URL}"
        echo "Check that the tag '${SKIA_TAG}' matches a valid release at:"
        echo "  https://github.com/aseprite/skia/releases"
        exit 1
    fi
    echo "  -> OK (HTTP 200)"

    # ── Fresh build/output directories ──────────────────────────────────

    rm -rf "$BUILD_DIR" "$OUTPUT_DIR"
    mkdir -p "$BUILD_DIR" "$OUTPUT_DIR"

    # ── Generate Dockerfile ─────────────────────────────────────────────

    echo "Generating Dockerfile..."

    cat > "$BUILD_DIR/Dockerfile" <<DOCKERFILE
# ═══════════════════════════════════════════════════════════════════
# Stage 1: Build Aseprite
# ═══════════════════════════════════════════════════════════════════
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies (per INSTALL.md Linux section).
# g++ provides libstdc++ headers/ABI needed by the pre-compiled Skia.
RUN apt-get update && apt-get install -y \\
    g++ \\
    clang \\
    cmake \\
    ninja-build \\
    libx11-dev \\
    libxcursor-dev \\
    libxi-dev \\
    libxrandr-dev \\
    libgl1-mesa-dev \\
    libfontconfig1-dev \\
    curl \\
    unzip \\
    && rm -rf /var/lib/apt/lists/*

# ── Download pre-compiled Skia ──────────────────────────────────
# Skia is ABI-sensitive. The pre-built version from aseprite/skia
# is compiled with libstdc++, so we must match that.
ARG SKIA_URL
ARG SKIA_ZIP
WORKDIR /deps
RUN echo "Downloading Skia..." \\
    && curl -sSL --ssl-revoke-best-effort -o "\${SKIA_ZIP}" "\${SKIA_URL}" \\
    && echo "Extracting Skia..." \\
    && unzip -q "\${SKIA_ZIP}" \\
    && rm "\${SKIA_ZIP}" \\
    && test -f /deps/out/Release-x64/libskia.a \\
    && echo "Skia ready at /deps/out/Release-x64/"

# ── Copy Aseprite source ────────────────────────────────────────
COPY . /aseprite

WORKDIR /aseprite/build

# ── Configure with cmake ────────────────────────────────────────
# Flags per INSTALL.md Linux section:
#   - Use clang + libstdc++ for ABI compatibility with Skia.
#   - Point to Skia via SKIA_DIR / SKIA_LIBRARY_DIR / SKIA_LIBRARY.
ENV CC=clang
ENV CXX=clang++

RUN cmake \\
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \\
    -DCMAKE_CXX_FLAGS:STRING=-stdlib=libstdc++ \\
    -DCMAKE_EXE_LINKER_FLAGS:STRING=-stdlib=libstdc++ \\
    -DLAF_BACKEND=skia \\
    -DSKIA_DIR=/deps \\
    -DSKIA_LIBRARY_DIR=/deps/out/Release-x64 \\
    -DSKIA_LIBRARY=/deps/out/Release-x64/libskia.a \\
    -G Ninja \\
    ..

# ── Compile ─────────────────────────────────────────────────────
RUN ninja aseprite

# ═══════════════════════════════════════════════════════════════════
# Stage 2: Minimal runtime image
# ═══════════════════════════════════════════════════════════════════
FROM ubuntu:22.04 AS runtime

# Minimal runtime libraries required by Aseprite.
#   libstdc++6     - C++ standard library (ABI link to Skia)
#   libfontconfig1  - font discovery/rendering
#   libx11-6        - X11 window system
#   libxfixes3      - X Fixes extension
#   libxcursor1     - cursor handling
#   libxi6          - X Input extension
#   libxrandr2      - X RandR (display/monitor)
#   libgl1          - OpenGL (Skia rendering backend)
#   libxext6        - X extensions (common transitive dep)
#   ca-certificates - SSL certs (for update checks if enabled)
RUN apt-get update && apt-get install -y --no-install-recommends \\
    libstdc++6 \\
    libfontconfig1 \\
    libx11-6 \\
    libxfixes3 \\
    libxcursor1 \\
    libxi6 \\
    libxrandr2 \\
    libgl1 \\
    libxext6 \\
    ca-certificates \\
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /aseprite/build/bin/aseprite /usr/local/bin/aseprite

ENTRYPOINT ["/usr/local/bin/aseprite"]

# ═══════════════════════════════════════════════════════════════════
# Stage 3: FTP server (minimal anonymous read-only)
# ═══════════════════════════════════════════════════════════════════
FROM alpine:3.20 AS ftp

RUN apk add --no-cache vsftpd && \\
    mkdir -p /var/run/vsftpd/empty

# Configure vsftpd for anonymous read-only access
RUN echo "anonymous_enable=YES"     > /etc/vsftpd/vsftpd.conf && \\
    echo "local_enable=NO"         >> /etc/vsftpd/vsftpd.conf && \\
    echo "write_enable=NO"         >> /etc/vsftpd/vsftpd.conf && \\
    echo "anon_upload_enable=NO"   >> /etc/vsftpd/vsftpd.conf && \\
    echo "anon_mkdir_write_enable=NO" >> /etc/vsftpd/vsftpd.conf && \\
    echo "dirmessage_enable=NO"    >> /etc/vsftpd/vsftpd.conf && \\
    echo "xferlog_enable=NO"       >> /etc/vsftpd/vsftpd.conf && \\
    echo "connect_from_port_20=YES" >> /etc/vsftpd/vsftpd.conf && \\
    echo "pasv_min_port=21000"     >> /etc/vsftpd/vsftpd.conf && \\
    echo "pasv_max_port=21010"     >> /etc/vsftpd/vsftpd.conf && \\
    echo "seccomp_sandbox=NO"      >> /etc/vsftpd/vsftpd.conf && \\
    echo "anon_root=/data"         >> /etc/vsftpd/vsftpd.conf

# Copy the compiled binary into the FTP data directory
COPY --from=builder /aseprite/build/bin/aseprite /data/aseprite

VOLUME /data
EXPOSE 21 21000-21010
CMD ["vsftpd", "/etc/vsftpd/vsftpd.conf"]
DOCKERFILE
fi

# ── Build the Docker image (skip if ftp-only mode and image already exists) ─

IMAGE_NAME="aseprite"
FTP_IMAGE_NAME="aseprite-ftp"

if ! $FTP_ONLY; then
    echo ""
    echo "Building Docker image '${IMAGE_NAME}'..."
    echo "This will take several minutes (downloading Skia + compiling Aseprite)."
    echo ""

    docker build \
        --build-arg SKIA_ZIP="${SKIA_ZIP}" \
        --build-arg SKIA_URL="${SKIA_URL}" \
        -t "${IMAGE_NAME}" \
        -f "$BUILD_DIR/Dockerfile" \
        "$SCRIPT_DIR"

    # ── Extract binary from the runtime stage ────────────────────────────────

    echo ""
    echo "Extracting binary from Docker image..."
    TMP_CONTAINER=$(docker create "${IMAGE_NAME}")
    docker cp "${TMP_CONTAINER}:/usr/local/bin/aseprite" "${OUTPUT_DIR}/aseprite"
    docker rm "${TMP_CONTAINER}" >/dev/null

    chmod +x "${OUTPUT_DIR}/aseprite"

    # ── Build FTP server image ───────────────────────────────────────────────

    echo ""
    echo "Building FTP server image '${FTP_IMAGE_NAME}'..."
    docker build \
        --target ftp \
        -t "${FTP_IMAGE_NAME}" \
        -f "$BUILD_DIR/Dockerfile" \
        "$SCRIPT_DIR"
fi

# ── Cleanup temp files ──────────────────────────────────────────────────────

rm -rf "$BUILD_DIR"

# ── Done / Print build summary ──────────────────────────────────────────────

if ! $FTP_ONLY; then
    echo ""
    echo "============================================================"
    echo " BUILD SUCCESSFUL"
    echo "============================================================"
    echo ""
    echo " Binary:   ${OUTPUT_DIR}/aseprite"
    echo " Size:     $(du -h "${OUTPUT_DIR}/aseprite" | cut -f1)"
    echo ""
    echo " Run directly:"
    echo "   ${OUTPUT_DIR}/aseprite"
    echo ""
    echo " Or run in Docker (no local deps needed):"
    echo "   docker run --rm -it \\"
    echo "     --network host \\"
    echo "     -e DISPLAY=\$DISPLAY \\"
    echo "     -v /tmp/.X11-unix:/tmp/.X11-unix \\"
    echo "     ${IMAGE_NAME}"
    echo ""
fi

# ── FTP Server ──────────────────────────────────────────────────────────────

FTP_CONTAINER="aseprite-ftp-server"
FTP_START=false

if $FTP_ONLY; then
    FTP_START=true
elif $AUTO_FTP; then
    FTP_START=true
else
    # Ask user if they want to start the FTP server
    echo ""
    read -p "Start FTP server to serve the binary? [Y/n] " REPLY
    REPLY=$(echo "$REPLY" | tr '[:upper:]' '[:lower:]')
    if [[ -z "$REPLY" || "$REPLY" == "y" || "$REPLY" == "yes" ]]; then
        FTP_START=true
    fi
fi

if $FTP_START; then
    # Stop any previously running FTP container
    if docker ps -a --format '{{.Names}}' | grep -qx "${FTP_CONTAINER}"; then
        echo "Stopping previous FTP server..."
        docker rm -f "${FTP_CONTAINER}" >/dev/null 2>&1
    fi

    # Check if FTP image exists
    if ! docker image inspect "${FTP_IMAGE_NAME}" >/dev/null 2>&1; then
        echo "ERROR: FTP image '${FTP_IMAGE_NAME}' not found."
        echo "Run './build-in-docker.sh' first to build everything."
        exit 1
    fi

    echo ""
    echo "Starting FTP server container '${FTP_CONTAINER}'..."
    docker run -d \
        --name "${FTP_CONTAINER}" \
        --restart unless-stopped \
        -p 21:21 \
        -p 21000-21010:21000-21010 \
        "${FTP_IMAGE_NAME}"

    echo ""
    echo "============================================================"
    echo " FTP SERVER RUNNING"
    echo "============================================================"
    echo ""
    echo " Download the binary with an FTP client:"
    echo ""
    echo "   ftp $(hostname -I 2>/dev/null | awk '{print $1}' || echo 'localhost')"
    echo "   Name: anonymous"
    echo "   Password: (empty)"
    echo "   ftp> binary"
    echo "   ftp> get aseprite"
    echo "   ftp> quit"
    echo ""
    echo " Or one-liner with curl:"
    echo "   curl -O ftp://localhost/aseprite"
    echo ""
    echo " Stop the FTP server:"
    echo "   docker rm -f ${FTP_CONTAINER}"
    echo ""
fi