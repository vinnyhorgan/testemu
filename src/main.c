#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdint.h>

#define WIDTH 320
#define HEIGHT 240
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
	"void main() {\n"
	"    vec2 texel = TexCoord * vec2(fb_width, textureSize(fb_tex, 0).y);\n"
	"    ivec2 pixel = ivec2(texel);\n"
	"    float index_byte = int(texelFetch(fb_tex, pixel, 0).r * 255.0);\n"
	"    int color_index;\n"
	"    if (int(texel.x) % 2 == 0) {\n"
	"        color_index = int(index_byte) & 0x0F;\n"
	"    } else {\n"
	"        color_index = (int(index_byte) >> 4) & 0x0F;\n"
	"    }\n"
	"    FragColor = texelFetch(pal_tex, color_index, 0);\n"
	"}\n";

static void size_callback(GLFWwindow *window, int width, int height) {
	float aspect_ratio = (float) WIDTH / HEIGHT;

	if (width / aspect_ratio > height) {
		glViewport((width - height * aspect_ratio) / 2, 0, height * aspect_ratio, height);
	} else {
		glViewport(0, (height - width / aspect_ratio) / 2, width, width / aspect_ratio);
	}
}

static void set_pixel(uint8_t *fb, int x, int y, uint8_t color) {
	int index = (y * WIDTH + x) / 2;
	if (x % 2 == 0) {
		fb[index] = (fb[index] & 0x0F) | (color << 4);
	} else {
		fb[index] = (fb[index] & 0xF0) | (color & 0x0F);
	}
}

static uint8_t get_pixel(uint8_t *fb, int x, int y) {
	int index = (y * WIDTH + x) / 2;
	if (x % 2 == 0) {
		return (fb[index] >> 4) & 0x0F;
	} else {
		return fb[index] & 0x0F;
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

	GLFWwindow *window = glfwCreateWindow(WIDTH * 2, HEIGHT * 2, "Test Emu - GL", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwSetFramebufferSizeCallback(window, size_callback);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		glfwTerminate();
		return -1;
	}

	uint8_t fb[WIDTH * HEIGHT / 2];

	/* HORIZONTAL GRADIENT
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			uint8_t color = (x * 16) / WIDTH; // Scale x to 4-bit color
			set_pixel(fb, x, y, color);
		}
	}
	*/

	/* RADIAL GRADIENT
	int cx = WIDTH / 2;
	int cy = HEIGHT / 2;
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			int dx = x - cx;
			int dy = y - cy;
			int distance = (int)(sqrt(dx * dx + dy * dy) / 10) % 16;
			set_pixel(fb, x, y, distance);
		}
	}
	*/

	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			uint8_t color = ((x + y) / 10) % 16;
			set_pixel(fb, x, y, color);
		}
	}

	uint8_t pal[PALETTE_SIZE * 4] = {
		 33,  59,  37, 255,     58,  96,  74, 255,     79, 119,  84, 255,    161, 159, 124, 255,
		119, 116,  79, 255,    119,  92,  79, 255,     96,  59,  58, 255,     59,  33,  55, 255,
		 23,  14,  25, 255,     47,  33,  59, 255,     67,  58,  96, 255,     79,  82, 119, 255,
		101, 115, 140, 255,    124, 148, 161, 255,    160, 185, 186, 255,    192, 209, 204, 255
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

	unsigned int shader_program = glCreateProgram();
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

	unsigned int vao, vbo, ebo;
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
	unsigned int pal_texture, fb_texture;

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

	// MAIN LOOP
	double last_time = glfwGetTime();
	int frame_count = 0;

	while (!glfwWindowShouldClose(window)) {
		double current_time = glfwGetTime();
		double delta_time = current_time - last_time;
		last_time = current_time;

		frame_count++;

		static double fps_time_accum = 0.0;
		fps_time_accum += delta_time;
		if (fps_time_accum >= 1.0) {
			printf("FPS: %d\n", frame_count);
			frame_count = 0;
			fps_time_accum = 0.0;
		}

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
