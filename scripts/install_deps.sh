#!/bin/bash

# This is the script to install all dependecy needed for the build of ft_vox.
# It will install GLM, a math library for easy manipolation of matrix, vectors and etc
# It will install also stb image, a header library for handling faces and polygons in opengl
# To run this script: chmod +x ./install_deps.sh
# DO NOT RUN SCRIPT WITHOUT CHECKING THEM

set -e # this is needed to force bash to exit the script on any non zero exit code

LIBS_DIR="libs"
mkdir -p "$LIBS_DIR"

# GLM - header-only math library
if [ ! -d "$LIBS_DIR/glm" ]; then
	echo "Downloading GLM..."
	git clone --depth=1 --branch 1.0.1 https://github.com/g-truc/glm.git "$LIBS_DIR/glm"
	echo "GLM ready."
else
	echo "GLM already present."
fi

# stb_image - header-only image loader
if [ ! -f "$LIBS_DIR/stb_image.h" ]; then
	echo "Downloading stb_image..."
	curl -fsSL -o "$LIBS_DIR/stb_image.h" \
		https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
	echo "stb_image ready."
else
	echo "stb_image already present."
fi

# System package check (Debian/Ubuntu)
PKG_MISSING=()
for pkg in libglfw3-dev libglew-dev; do
	if ! dpkg -s "$pkg" &>/dev/null; then
		PKG_MISSING+=("$pkg")
	fi
done

if [ ${#PKG_MISSING[@]} -gt 0 ]; then
	echo ""
	echo "Missing system packages. Please install them with 'sudo apt install ${PKG_MISSING[*]}'"
	exit 1
fi

echo "All packages installed."