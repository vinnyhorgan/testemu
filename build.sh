gcc -D_GLFW_X11 src/main.c src/lib/glad/src/*.c src/lib/glfw/src/*.c -o testemu -Isrc/lib/glad/include -Isrc/lib/glfw/include -lm
