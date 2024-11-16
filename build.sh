gcc -D_GLFW_X11 src/main.c src/lib/glad/src/*.c src/lib/glfw/src/*.c src/lib/miniz/*.c -o testemu -Isrc/lib/glad/include -Isrc/lib/glfw/include -Isrc/lib/miniaudio -Isrc/lib/miniz -lm
