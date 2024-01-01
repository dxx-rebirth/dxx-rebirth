# Use an official Debian image as a base image
FROM debian:bullseye

# Install necessary dependencies
RUN apt-get update && apt-get install -y \
    python3 \
    scons \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-mixer-dev \
    libphysfs-dev \
    libglu1-mesa-dev \
    libpng-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy the source code into the container
COPY . .

# Build the application using SCons
RUN scons sdl2=1 sdlmixer=1 opengl=1
