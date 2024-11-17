#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define WIDTH 240
#define HEIGHT 136
#define SCALE 4
#define PALETTE_SIZE 16

const char *vertex_source =
	"#version 330 core\n"
	"layout (location = 0) in vec3 aPos;\n"
	"layout (location = 1) in vec2 aTexCoord;\n"
	"out vec2 TexCoord;\n"
	"void main() {\n"
	"    gl_Position = vec4(aPos, 1.0);\n"
	"    TexCoord = aTexCoord;\n"
	"}\n";

const char *fragment_source =
	"#version 330 core\n"
	"out vec4 FragColor;\n"
	"in vec2 TexCoord;\n"
	"uniform sampler1D pal_tex;\n"
	"uniform sampler2D fb_tex;\n"
	"uniform int fb_width;\n"
	"uniform float warp;\n"
	"uniform float scan;\n"
	"void main() {\n"
	"    vec2 texel = TexCoord * vec2(fb_width, textureSize(fb_tex, 0).y);\n"
	"    ivec2 pixel = ivec2(floor(texel));\n"
	"    float index_byte = int(texelFetch(fb_tex, pixel, 0).r * 255.0);\n"
	"    int color_index;\n"
	"    if (int(texel.x * 2.0) % 2 == 0) {\n"
	"        color_index = int(index_byte) & 0x0F;\n"
	"    } else {\n"
	"        color_index = (int(index_byte) >> 4) & 0x0F;\n"
	"    }\n"
	"    vec4 color = texelFetch(pal_tex, color_index, 0);\n"
	"    vec2 uv = TexCoord;\n"
	"    vec2 dc = abs(0.5 - uv) * abs(0.5 - uv);"
	"    uv.x -= 0.5;\n"
	"    uv.x *= 1.0 + (dc.y * (0.3 * warp));\n"
	"    uv.x += 0.5;\n"
	"    uv.y -= 0.5;\n"
	"    uv.y *= 1.0 + (dc.x * (0.4 * warp));\n"
	"    uv.y += 0.5;\n"
	"    if (uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0) {\n"
	"        FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
	"    } else {\n"
	"        float apply = abs(sin(gl_FragCoord.y) * 0.5 * scan);\n"
	"        FragColor = mix(color, vec4(0.0, 0.0, 0.0, 1.0), apply);\n"
	"    }\n"
	"}\n";

GLFWwindow *window;
unsigned int shader_program;
unsigned int vao;
unsigned int pal_texture, fb_texture;
uint8_t fb[WIDTH * HEIGHT / 2];

bool is_fullscreen = false;
int prev_x, prev_y, prev_w, prev_h;
static int get_current_monitor(void);
static void toggle_fullscreen(void);

// EMULATION STUFF
extern uint16_t pc;
extern uint8_t sp, a, x, y, status;
void reset6502(void);
void exec6502(uint32_t tick_count);
void step6502(void);
void irq6502(void);
void nmi6502(void);

uint8_t ram[1 << 16];

uint8_t read6502(uint16_t address) {
	return ram[address];
}

void write6502(uint16_t address, uint8_t value) {
	ram[address] = value;
}

static void reset(void) {
	memset(ram, 0, sizeof(ram));
	reset6502();
	pc = 0x41C0;
}

// CALLBACKS
static void draw() {
	glClear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, pal_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fb_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH / 2, HEIGHT, GL_RED, GL_UNSIGNED_BYTE, fb);

	glUseProgram(shader_program);
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glfwSwapBuffers(window);
}

static void size_callback(GLFWwindow *window, int width, int height) {
	int scale_x = width / WIDTH;
	int scale_y = height / HEIGHT;
	int scale = scale_x < scale_y ? scale_x : scale_y;
	if (scale == 0) scale = 1;

	int view_width = WIDTH * scale;
	int view_height = HEIGHT * scale;
	int view_x = (width - view_width) / 2;
	int view_y = (height - view_height) / 2;

	glViewport(view_x, view_y, view_width, view_height);

	draw();
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ENTER && (mods & GLFW_MOD_ALT) && action == GLFW_PRESS) {
		toggle_fullscreen();
	}

	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		printf("PRE-STATE\n");

		printf("PC: %04X\n", pc);
		printf("SP: %02X\n", sp);
		printf("A: %02X\n", a);
		printf("X: %02X\n", x);
		printf("Y: %02X\n", y);
		printf("Status: %02X\n", status);
		printf("\n");

		step6502();

		printf("PC: %04X\n", pc);
		printf("SP: %02X\n", sp);
		printf("A: %02X\n", a);
		printf("X: %02X\n", x);
		printf("Y: %02X\n", y);
		printf("Status: %02X\n", status);
		printf("\n");

		printf("STACK\n");
		for (int i = 0x100; i < 0x1FF; i++) {
			printf("%02X ", ram[i]);
		}
		printf("\n");
	}
}

int main() {
	// SETUP
	if (!glfwInit()) {
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH * SCALE, HEIGHT * SCALE, "Emu - v0.1.0", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

#ifdef _WIN32
	HWND hwnd = glfwGetWin32Window(window);
	BOOL value = TRUE;
	DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
#endif

	glfwSetWindowSizeLimits(window, WIDTH, HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
	glfwShowWindow(window);

	glfwSetFramebufferSizeCallback(window, size_callback);
	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		glfwTerminate();
		return -1;
	}

	#define HEX(hex) ((hex >> 16) & 0xFF), ((hex >> 8) & 0xFF), (hex & 0xFF), 255

	// Lost Century palette lospec
	uint8_t pal[PALETTE_SIZE * 4] = {
		HEX(0x1a1c2c), HEX(0x5d275d), HEX(0xb13e53), HEX(0xef7d57),
		HEX(0xffcd75), HEX(0xa7f070), HEX(0x38b764), HEX(0x257179),
		HEX(0x29366f), HEX(0x3b5dc9), HEX(0x41a6f6), HEX(0x73eff7),
		HEX(0xf4f4f4), HEX(0x94b0c2), HEX(0x566c86), HEX(0x333c57)
	};

	// SHADERS
	unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_source, NULL);
	glCompileShader(vertex_shader);

	int success;
	char info_log[512];
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
		printf("Error compiling vertex shader\n%s\n", info_log);
	}

	unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_source, NULL);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
		printf("Error compiling fragment shader\n%s\n", info_log);
	}

	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader_program, 512, NULL, info_log);
		printf("Error linking shader program\n%s\n", info_log);
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	// MORE GL STUFF
	float vertices[] = {
		 1.0f,  1.0f, 0.0f,    1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f,    1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,    0.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,    0.0f, 1.0f
	};

	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3
	};

	unsigned int vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) 0);
	glEnableVertexAttribArray(0);

	// texture attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) (3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// TEXTURES

	// palette texture (1D)
	glGenTextures(1, &pal_texture);
	glBindTexture(GL_TEXTURE_1D, pal_texture);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, PALETTE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, pal);

	// framebuffer texture (2D)
	glGenTextures(1, &fb_texture);
	glBindTexture(GL_TEXTURE_2D, fb_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, WIDTH / 2, HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, fb);

	// assign uniforms
	glUseProgram(shader_program);
	glUniform1i(glGetUniformLocation(shader_program, "pal_tex"), 0);
	glUniform1i(glGetUniformLocation(shader_program, "fb_tex"), 1);
	glUniform1i(glGetUniformLocation(shader_program, "fb_width"), WIDTH / 2);
	glUniform1f(glGetUniformLocation(shader_program, "warp"), 0.0f);
	glUniform1f(glGetUniformLocation(shader_program, "scan"), 0.75f);

	// emulation stuff
	reset();

	FILE *rom = fopen("rom.bin", "rb");
	if (!rom) {
		printf("Error opening rom.bin\n");
		return -1;
	}

	fseek(rom, 0, SEEK_END);
	long rom_size = ftell(rom);
	fseek(rom, 0, SEEK_SET);

	uint8_t *program = malloc(rom_size);
	fread(program, 1, rom_size, rom);
	fclose(rom);

	for (int i = 0; i < rom_size; i++) {
		ram[0x41C0 + i] = program[i];
	}

	free(program);

	// MAIN LOOP
	double last_time = glfwGetTime();
	int frame_count = 0;

	int time_counter = 0;

	while (!glfwWindowShouldClose(window)) {
		double current_time = glfwGetTime();
		double delta_time = current_time - last_time;
		last_time = current_time;

		frame_count++;

		static double fps_time_accum = 0.0;
		fps_time_accum += delta_time;
		if (fps_time_accum >= 1.0) {
			// printf("FPS: %d\n", frame_count);
			frame_count = 0;
			fps_time_accum = 0.0;
		}

		for (int i = 0; i < WIDTH * HEIGHT / 2; i++) {
			fb[i] = read6502(0x200 + i);
		}

		// run 6502 at 10 MHz
		int current_monitor = get_current_monitor();
		int monitor_count;
		GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);
		GLFWmonitor *monitor = monitors[current_monitor];
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);
		int refresh_rate = mode->refreshRate;

		exec6502(10000000 / refresh_rate);

		draw();

		glfwPollEvents();
	}

	// CLEANUP
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteProgram(shader_program);

	glfwTerminate();
	return 0;
}

// functions adapted from raylib
static int get_current_monitor(void) {
	int index = 0;
	int monitor_count = 0;
	GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);
	GLFWmonitor *monitor = NULL;

	if (monitor_count >= 1) {
		int closestDist = 0x7FFFFFFF;

		int wcx = 0;
		int wcy = 0;

		glfwGetWindowPos(window, &wcx, &wcy);

		int w, h;
		glfwGetWindowSize(window, &w, &h);

		wcx += w / 2;
		wcy += h / 2;

		for (int i = 0; i < monitor_count; i++) {
			int mx = 0;
			int my = 0;

			monitor = monitors[i];
			glfwGetMonitorPos(monitor, &mx, &my);
			const GLFWvidmode *mode = glfwGetVideoMode(monitor);

			if (mode) {
				const int right = mx + mode->width - 1;
				const int bottom = my + mode->height - 1;

				if ((wcx >= mx) && (wcx <= right) && (wcy >= my) && (wcy <= bottom)) {
					index = i;
					break;
				}

				int xclosest = wcx;
				if (wcx < mx) xclosest = mx;
				else if (wcx > right) xclosest = right;

				int yclosest = wcy;
				if (wcy < my) yclosest = my;
				else if (wcy > bottom) yclosest = bottom;

				int dx = wcx - xclosest;
				int dy = wcy - yclosest;
				int dist = (dx * dx) + (dy * dy);
				if (dist < closestDist) {
					index = i;
					closestDist = dist;
				}
			}
		}
	}

	return index;
}

static void toggle_fullscreen(void) {
	const int monitor = get_current_monitor();
	int monitor_count;
	GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);

	if ((monitor >= 0) && (monitor < monitor_count)) {
		const GLFWvidmode *mode = glfwGetVideoMode(monitors[monitor]);

		if (mode) {
			if (!is_fullscreen) {
				glfwGetWindowPos(window, &prev_x, &prev_y);
				glfwGetWindowSize(window, &prev_w, &prev_h);

				glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
				glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);

				int monitor_x, monitor_y;
				glfwGetMonitorPos(monitors[monitor], &monitor_x, &monitor_y);

				glfwSetWindowPos(window, monitor_x, monitor_y);
				glfwSetWindowSize(window, mode->width, mode->height);

				glfwFocusWindow(window);

				is_fullscreen = true;
			} else {
				glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
				glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_FALSE);

				glfwSetWindowPos(window, prev_x, prev_y);
				glfwSetWindowSize(window, prev_w, prev_h);

				glfwFocusWindow(window);

				is_fullscreen = false;
			}
		}
	}
}
