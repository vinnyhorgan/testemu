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
	"   gl_Position = vec4(aPos, 1.0);\n"
	"   TexCoord = aTexCoord;\n"
	"}\n";

const char *fragment_source =
	"#version 330 core\n"
	"out vec4 FragColor;\n"
	"in vec2 TexCoord;\n"
	"uniform sampler2D pal_tex;\n"
	"uniform sampler2D fb_tex;\n"
	"void main() {\n"
	"   FragColor = texture(fb_tex, TexCoord);\n"
	"}\n";

static void size_callback(GLFWwindow *window, int width, int height) {
	glViewport(0, 0, width, height);
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
	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		glfwTerminate();
		return -1;
	}

	uint8_t fb[WIDTH * HEIGHT * 4];

	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			fb[(y * WIDTH + x) * 4 + 0] = (uint8_t)(x * 255 / WIDTH);
			fb[(y * WIDTH + x) * 4 + 1] = (uint8_t)(y * 255 / HEIGHT);
			fb[(y * WIDTH + x) * 4 + 2] = 0;
			fb[(y * WIDTH + x) * 4 + 3] = 255;
		}
	}

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
		 0.5f,  0.5f, 0.0f,    1.0f, 1.0f,
		 0.5f, -0.5f, 0.0f,    1.0f, 0.0f,
		-0.5f, -0.5f, 0.0f,    0.0f, 0.0f,
		-0.5f,  0.5f, 0.0f,    0.0f, 1.0f
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

	// TEXTURESSS
	unsigned int pal_texture, fb_texture;

	glGenTextures(1, &pal_texture);
	glBindTexture(GL_TEXTURE_2D, pal_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glGenTextures(1, &fb_texture);
	glBindTexture(GL_TEXTURE_2D, fb_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb);

	glUseProgram(shader_program);
	glUniform1i(glGetUniformLocation(shader_program, "pal_tex"), 0);
	glUniform1i(glGetUniformLocation(shader_program, "fb_tex"), 1);

	// MAIN LOOP

	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pal_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, fb_texture);

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
