#include <glad/glad.h>
#include <glfw/glfw3.h>
// #include <utility/Model.h>
#include <iostream>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>	
#include "model.h"
#include <utility/Shader.h>
#include <utility/Camera.h>	

int window_width = 1200;
int window_height = 650;
float deltaTime;

// function prototypes
void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

int main() {
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 8);

	// Window Creation
	GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Framebuffers", nullptr, nullptr);
	if (window == NULL) {
		std::cout << "ERROR::FAILED TO CREATE WINDOW\n";
		return -1;
	}
	glfwMakeContextCurrent(window);

	// set resize callback
	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

	// Load OpenGL functions
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "ERROR::FAILE TO INITIALIZE GLAD\n";
		return -1;
	}
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	// UI Initialization
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// ImGUI style
	ImGui::StyleColorsDark();
	// setup Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	// Camera
	Camera sceneCamera(glm::vec3(0.0f, 15.0f, 20.0f));

	// Shaders
	Shader modelShader("model.vert", "model.frag");
	Shader framebufferShader("framebuffer.vert", "framebuffer_normal.frag");

	// 3D Models
	Model scene("models/space_station/Space Station Scene.obj");
	// Model watchtower("models/watch_tower/wooden watch tower2.obj");

	float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};
	// screen quad VAO
	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	// Secondary Framebuffer
	GLuint FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	// generate texture
	GLuint textureColorbuffer;
	glGenTextures(1, &textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	// attach it to currently bound framebuffer object
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Game Loop
	float lastFrame = 0.0f;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// setup DeltaTime
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Input processing
		sceneCamera.processKeyInputs(window, deltaTime);
		sceneCamera.ProcessMouseMovement(window, deltaTime);

		// Setting up transformation matrices
		glm::mat4 view = sceneCamera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)window_width / window_height, 0.1f, 1000.0f);

		// binding the framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f); 
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Drawing the environment
		modelShader.use();
		modelShader.setMat4("view", view);
		modelShader.setMat4("projection", projection);
		
		// environment
		glm::mat4 environmentModel(1.0f);
		modelShader.setMat4("model", environmentModel);
		scene.render(modelShader);

		// start ImGUI frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGUI rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


		// back to default framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		framebufferShader.use();
		glBindVertexArray(quadVAO);
		glDisable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
		glDrawArrays(GL_TRIANGLES, 0, 6);


		// ------------------------
		glfwSwapBuffers(window);
	}

	return 0;
}


void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
	window_width = width;
	window_height = height;
	glViewport(0, 0, width, height);
}