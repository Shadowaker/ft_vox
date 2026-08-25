#include "../include/headers.hpp"

static constexpr float FOV           = 80.0f;
static constexpr float CAMERA_SPEED  = 10.0f;
static constexpr float CAMERA_FAST   = 20.0f;
static constexpr float NEAR_PLANE    = 0.1f;
static constexpr float FAR_PLANE     = 3000.0f;

static Camera  camera(glm::vec3(0.0f, 130.0f, 0.0f));
static bool    mouse_init = true;
static float   lastX = 0.0f;
static float   lastY = 0.0f;

// Debug: press C to toggle the translucent underground-air (cave) view
static bool    g_showCaveDebug = false;
static bool    g_cKeyWasDown   = false;

static void framebuffer_size_callback(GLFWwindow*, int width, int height) {
	glViewport(0, 0, width, height);
}

static void mouse_callback(GLFWwindow*, double xpos, double ypos) {
	if (mouse_init) {
		lastX = static_cast<float>(xpos);
		lastY = static_cast<float>(ypos);
		mouse_init = false;
	}
	float xoffset = static_cast<float>(xpos) - lastX;
	float yoffset = lastY - static_cast<float>(ypos);
	lastX = static_cast<float>(xpos);
	lastY = static_cast<float>(ypos);
	camera.processMouseMovement(xoffset, yoffset);
}

static void processInput(GLFWwindow* window, float deltaTime) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float speed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		? CAMERA_FAST : CAMERA_SPEED;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.processKeyboard(CameraDirection::FORWARD,  speed * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.processKeyboard(CameraDirection::BACKWARD, speed * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.processKeyboard(CameraDirection::LEFT,     speed * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.processKeyboard(CameraDirection::RIGHT,    speed * deltaTime);

	// Edge-triggered toggle: flip once per physical press, not once per frame held
	bool cPressed = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
	if (cPressed && !g_cKeyWasDown)
		g_showCaveDebug = !g_showCaveDebug;
	g_cKeyWasDown = cPressed;
}

// One color 16x16 tile per block, defined in the shader: atlas_u = TexCoord.x / ATLAS_W  where u = typeIndex + corner_u.
// One extra tile past the real block types is reserved for the cave-debug
// visualization (Chunk::buildMesh tags it with tid == BlockType::COUNT).
static unsigned int createBlockAtlas() {
	static const unsigned char colors[][4] = {
		{   0,   0,   0,   0 }, // AIR
		{  87, 166,  57, 255 }, // GRASS
		{ 134,  96,  67, 255 }, // DIRT
		{ 128, 128, 128, 255 }, // STONE
		{ 210, 200, 140, 255 }, // SAND
		{ 240, 245, 255, 255 }, // SNOW
		{  30, 100, 220, 220 }, // WATER
		{ 255,  60,  60, 130 }, // DEBUG: cave air
	};
	constexpr int COUNT = static_cast<int>(BlockType::COUNT) + 1; // +1 for the debug tile
	constexpr int TILE  = 16;
	constexpr int W     = COUNT * TILE;
	constexpr int H     = TILE;

	std::vector<unsigned char> data(W * H * 4);
	for (int t = 0; t < COUNT; t++) {
		for (int py = 0; py < H; py++) {
			for (int px = 0; px < TILE; px++) {
				int i = ((py * W) + (t * TILE + px)) * 4;
				data[i+0] = colors[t][0];
				data[i+1] = colors[t][1];
				data[i+2] = colors[t][2];
				data[i+3] = colors[t][3];
			}
		}
	}

	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return tex;
}

int main() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW\n";
		return 1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode    = glfwGetVideoMode(monitor);
	int                width   = mode->width;
	int                height  = mode->height;

	GLFWwindow* window = glfwCreateWindow(width, height, "ft_vox", monitor, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window\n";
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (glewInit() != GLEW_OK) {
		std::cerr << "Failed to initialize GLEW\n";
		glfwTerminate();
		return 1;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glViewport(0, 0, width, height);

	try {
		Shader       shader("shaders/vertex.glsl", "shaders/fragment.glsl");
		World        world;
		Hud          hud;
		unsigned int atlas = createBlockAtlas();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlas);

		float deltaTime = 0.0f;
		float lastFrame = 0.0f;

		// FPS is averaged over a short window and refreshed a few times a
		// second rather than every frame, so the HUD number doesn't flicker.
		float fpsTimer   = 0.0f;
		int   fpsFrames  = 0;
		int   fpsDisplay = 0;

		while (!glfwWindowShouldClose(window)) {
			float currentFrame = static_cast<float>(glfwGetTime());
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			fpsTimer += deltaTime;
			fpsFrames++;
			if (fpsTimer >= 0.5f) {
				fpsDisplay = static_cast<int>(fpsFrames / fpsTimer);
				fpsFrames  = 0;
				fpsTimer   = 0.0f;
			}

			processInput(window, deltaTime);
			world.update(camera.getPosition());

			glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 view = camera.getViewMatrix();
			glm::mat4 proj = glm::perspective(
				glm::radians(FOV),
				static_cast<float>(width) / static_cast<float>(height),
				NEAR_PLANE, FAR_PLANE
			);

			shader.use();
			shader.setMat4("view", view);
			shader.setMat4("projection", proj);
			shader.setInt("textureAtlas", 0);

			if (g_showCaveDebug) {
				// Only the carved-out underground air renders (translucent);
				// normal solid terrain is skipped entirely, so it reads as invisible.
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable(GL_CULL_FACE); // see both sides of the hollowed-out shell
				glDepthMask(GL_FALSE);   // avoid translucent faces occluding each other

				world.render(shader, proj * view, true);

				glDepthMask(GL_TRUE);
				glEnable(GL_CULL_FACE);
				glDisable(GL_BLEND);
			} else {
				world.render(shader, proj * view);
			}

			// HUD: FPS counter and current biome, top-left corner, always on top
			{
				std::string fpsLine   = "FPS: " + std::to_string(fpsDisplay);
				std::string biomeLine = world.getBiomeAt(camera.getPosition());
				constexpr float scale      = 2.0f;
				constexpr float lineHeight = 8.0f * scale + 4.0f; // glyph cell + gap

				glDisable(GL_DEPTH_TEST); // always draw on top of the world
				glDisable(GL_CULL_FACE);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				hud.renderText(fpsLine,   10.0f, 10.0f,              scale, width, height);
				hud.renderText(biomeLine, 10.0f, 10.0f + lineHeight, scale, width, height);

				glDisable(GL_BLEND);
				glEnable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);
			}

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		glDeleteTextures(1, &atlas);
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		glfwTerminate();
		return 1;
	}

	glfwTerminate();
	return 0;
}