# Emscripten

## Build

### Descent 1

```
emscons scons CXX=em++ CXXFLAGS="-sUSE_SDL=2 -Icommon/include/physfs/src -Icommon/include/gl4es/include" d1x=1 d2x=0 sharepath=""
```

### Descent 2

```
emscons scons CXX=em++ CXXFLAGS="-sUSE_SDL=2 -Icommon/include/physfs/src -Icommon/include/gl4es/include" d1x=0 d2x=1 sharepath=""
```

## Link

### Descent 1

```
em++ -flto -O3 common/*/*.o common/*/*/*.o d1x-rebirth/main/*.o similar/*/*.o similar/*/*/*.o libphysfs.a libGL.a libGLU.a -o index.html -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["pcx"]' -sUSE_SDL_MIXER=2 -sSDL2_MIXER_FORMATS='["mid"]' -sFULL_ES2 -lGL -sASYNCIFY -sENVIRONMENT=web --preload-file data/ --preload-file d1x.ini --preload-file dgguspat@/etc/timidity -sINITIAL_MEMORY=128mb -sSTACK_SIZE=524288 -Wl,-u,fileno -Wl,-u,htonl --closure 1
```

### Descent 2

```
em++ -flto -O3 common/*/*.o common/*/*/*.o d2x-rebirth/main/*.o d2x-rebirth/libmve/*.o similar/*/*.o similar/*/*/*.o libphysfs.a libGL.a libGLU.a -o index.html -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["pcx"]' -sUSE_SDL_MIXER=2 -sSDL2_MIXER_FORMATS='["mid"]' -sFULL_ES2 -lGL -sASYNCIFY -sENVIRONMENT=web --preload-file data/ --preload-file d2x.ini --preload-file dgguspat@/etc/timidity -sINITIAL_MEMORY=128mb -sSTACK_SIZE=524288 -Wl,-u,fileno -Wl,-u,htonl --closure 1
```
