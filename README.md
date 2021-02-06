# Scoreview-Base

Scoreview is a tool to analyse music and learn how to play it on various instruments.
Mostly a proof of concept, using the GPU to visualise sound in real time.
At the begining of it, I really though it would be amazing and it is. But the technological choices
were weird. However it is a nice tool to visually analyse sounds.

## Command to build it:

```
make project
```

The program uses OpenCL to process sound, so you need to install OpenCL support for your GPU.

It needs a lot of dependecies: SDL2, SDL2_ttf, SDL2_image, SDL2_net, Portaudio, Tinyxml, libevent, libsndfile, OpenGL, glm, OpenCL, FLTK, Protocol buffers and DSPFilters as a submodule.

On windows they can be installed with pacman on MSys except Protocol buffers.

## Examples

```
pacman -Ss SDL2
pacman -S mingw32/mingw-w64-i686-SDL2
pacman -S mingw32/mingw-w64-i686-SDL2_ttf
pacman -S mingw32/mingw-w64-i686-SDL2_image

pacman -Ss protobuf
pacman -S mingw32/mingw-w64-i686-protobufe
pacman -S msys/protobuf-devel
```

On linux, just install the dev packages for the libraries mentioned above.

The artwork was made by Clement Caminal, https://clementcaminal.wixsite.com/portfolio 


www.vreemdelabs.com
blog.vreemdelabs.com