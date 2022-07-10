#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <learnopengl/shader_m.h>
#include <learnopengl/shader_c.h>
#include <learnopengl/camera.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
unsigned int createLookUp();

#define rand01() ((float)rand() / (float)(RAND_MAX))

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// particles 
const unsigned int NUM_PARTICLES = 1000000;
const unsigned int LOCAL_SIZE_OF_WORK_GROUPS = 1000;

// attractor
glm::vec2 attractorPosition = glm::vec2(0, 0);
float attractorForce = 0;

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

int main(int argc, char* argv[])
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// get window size
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	// set the point size for rendering
	glPointSize(4);
	// set background to black
	glClearColor(0, 0, 0, 1);
	// enable blending
	glEnable(GL_BLEND);
	// set blend function
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// create Look Up Table
	// -----------------------------------
	unsigned int LUT = createLookUp();

	// initialize particle simulation buffers
	// -----------------------------------
	unsigned int particlePositionBuffer;
	unsigned int particleVelocityBuffer;

	glGenBuffers(1, &particlePositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, particlePositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * sizeof(glm::vec2), NULL, GL_STREAM_DRAW);
	glm::vec2 * positions =  (glm::vec2*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	
	for (int i = 0; i < NUM_PARTICLES; i++) {
		std::memcpy(&positions[i], &glm::vec2(rand01()* width, rand01()* height), sizeof(glm::vec2));
	}
	
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &particleVelocityBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, particleVelocityBuffer);
	glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * sizeof(glm::vec2), NULL, GL_STREAM_DRAW);
	glm::vec2* velocities = (glm::vec2*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	for (int i = 0; i < NUM_PARTICLES; i++) {
		std::memcpy(&velocities[i], &glm::vec2(0, 0), sizeof(glm::vec2*));
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// create particle VAO
	// -------------------
	unsigned int particlesVAO;

	// erstellen des Vertex Attribute Arrays
	glGenVertexArrays(1, &particlesVAO);
	glBindVertexArray(particlesVAO);

	// binden des Positionsbuffers
	glBindBuffer(GL_ARRAY_BUFFER, particlePositionBuffer);
	// linken des Positionsbuffers an Location 0
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	// binden des Geschwindigkeitsbuffers
	glBindBuffer(GL_ARRAY_BUFFER, particleVelocityBuffer);
	// linken des Geschwindigkeitsbuffers an Location 1
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	// entbinden des Array Buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// build and compile shaders
	// -------------------------
	Shader renderParticles("renderParticles.vs", "renderParticles.fs");
	ComputeShader computeShader("simulateParticles.cs");

	computeShader.use();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particlePositionBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particleVelocityBuffer);

	// time variables
	double deltaTime = 0.0;				// time between last frame and current frame
	double prevTime = glfwGetTime();	// 
	double lastFrame = glfwGetTime();	//

	int frameRate = 0;					// frames per second
	int frameCounter = 0;				// counter for frames per second

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, LUT);

	glBindVertexArray(particlesVAO);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// time operations
		float currentTime = glfwGetTime();
		deltaTime = currentTime - prevTime;
		prevTime = currentTime;
		
		frameCounter++;
		if (currentTime - lastFrame >= 1.0) {
			frameRate = frameCounter;
			frameCounter = 0;					// reset frame counter
			lastFrame += 1.0;
		}

		// get width and height of frame window/framebuffer
		glfwGetWindowSize(window, &width, &height);		

		glm::vec2 framebufferSize((float)width, (float)height);

		computeShader.use();
		computeShader.setFloat("dt", deltaTime);
		computeShader.setVec2("frameBufferSize", framebufferSize);
		computeShader.setVec2("attractorPosition", attractorPosition);
		computeShader.setFloat("attractorForce", attractorForce);

		glDispatchCompute(NUM_PARTICLES / LOCAL_SIZE_OF_WORK_GROUPS, 1, 1);

		// create projection matrix everyframe in case of rescaling the window
		glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);

		// set static states/variables
		renderParticles.use();
		renderParticles.setMat4("projectionMatrix", projection);
		
		// clear frame buffer
		glClear(GL_COLOR_BUFFER_BIT);

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// draws the particles 
		glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteBuffers(1, &particlePositionBuffer);
	glDeleteBuffers(1, &particleVelocityBuffer);

	glDeleteProgram(renderParticles.ID);
	glDeleteProgram(computeShader.ID);

	glfwTerminate();

	return EXIT_SUCCESS;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	attractorPosition.x = xpos;
	attractorPosition.y = ypos;
}

// glfw: whenever a mouse button is pressed
// -------------------------------------------------------
void mouse_button_callback(GLFWwindow* window,
						int button,
	int action,
	int mods)
{
	attractorForce = 0;
	if (action == GLFW_PRESS) {
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			attractorForce = -1.2f;
		}
		else if (button == GLFW_MOUSE_BUTTON_LEFT) {
			attractorForce = 1;
		}
	}
}

/**
 * @brief 1D-Lookup Tabelle erstellen
 */
unsigned int createLookUp() {
	unsigned int lookup;
	// erstellen einer Textur
	glGenTextures(1, &lookup);
	// Binden der Textur
	glBindTexture(GL_TEXTURE_1D, lookup);

	// Daten fuer die Textur erstellen 3 Felder (breite = 3) mit jeweils RGBA-Wert
	GLubyte data[4 * 3] = {
			255, 46 , 15, 25,
			255, 86 , 31, 50,
			255, 100, 61, 50
	};

	// Setzen der Textur-Parameter
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	// Erstellen der OpenGL 1D Textur mit den werten 
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_1D, 0);

	return lookup;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}
