#include "Discovery.h"
#include "config.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl.h"
#include "backends/imgui_impl_opengl3.h"

#include <cassert>
#include <ctime>
#include <iomanip>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <sstream>

SDL_Window   *window;
SDL_Surface  *final_screen;
SDL_Surface  *original_screen;
SDL_Rect      scale_rect;
SDL_GLContext gl_context;

const auto BG_COLOR = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void init();
void loop(Discovery &emulator);
void debugLoop(Discovery &emulator);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        log(LogLevel::Error, "Error: No ROM file given\n");
        log("Usage: ./discovery /path/to/rom\n");
        return 1;
    }

    Discovery emulator;

    // collect command line args
    for (int i = 1; i < argc; ++i)
        emulator.argv.push_back(argv[i]);

    // parse command line args
    emulator.parseArgs();
	if(config::show_help)
	{
		emulator.printArgHelp();
		return 0;
	}

    init();

	log("Welcome to Discovery!\n");

    // load bios, rom, and launch game loop
    emulator.mem->loadBios(config::bios_name);
    emulator.mem->loadRom(config::rom_name);
    
	if (config::debug)
		debugLoop(emulator);
	else
		loop(emulator);

    emulator.shutdown();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void init()
{
    assert(SDL_Init(SDL_INIT_VIDEO) >= 0);

	int window_width, window_height, window_flags;
	if (config::debug)
	{
		window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
		window_width = 1280;
		window_height = 720;
	}

	else
	{
		window_flags = 0;
		window_width = SCREEN_WIDTH * 2;
		window_height = SCREEN_HEIGHT * 2;
	}
	
	window = SDL_CreateWindow("discovery", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, window_flags);
    assert(window);

	final_screen = SDL_GetWindowSurface(window);
    original_screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    
    scale_rect.w = SCREEN_WIDTH  * 2;
    scale_rect.h = SCREEN_HEIGHT * 2;
    scale_rect.x = 0;
    scale_rect.y = 0;

	// debug only initialization
	if (!config::debug)
		return;

	// GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);
	glewExperimental = GL_TRUE;
	glewInit();

	// Set up Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

	ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void loop(Discovery &emulator)
{
	bool running = true;
    SDL_Event e;
    int frame = 0;
    clock_t old_time = std::clock();

	while (running)
    {
        emulator.frame();

        original_screen->pixels = (u32 *) emulator.ppu->screen_buffer;
        // scale screen buffer
        SDL_BlitScaled(original_screen, nullptr, final_screen, &scale_rect);

        // draw final_screen pixels on screen
        SDL_UpdateWindowSurface(window);
        
        // calculate fps
        if (++frame == 60)
        {
            frame = 0;

            double duration;
            clock_t new_time = std::clock();
            duration = (new_time - old_time) / (double) CLOCKS_PER_SEC;
            old_time = new_time;

            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) << (60 / duration);
            std::string title("");
            title += "discovery - ";
            title += stream.str();
            title += " fps";
            SDL_SetWindowTitle(window, title.c_str());
        }

        while (SDL_PollEvent(&e))
        {
            // Click X on window
            if (e.type == SDL_QUIT)
                running = false;
        
            // Key press event
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    running = false;

                emulator.gamepad->poll();
            }
        }
    }
}

void debugLoop(Discovery &emulator)
{
	bool running = true;
    SDL_Event e;
    int frame = 0;
    clock_t old_time = std::clock();
	ImGuiIO& io = ImGui::GetIO();

	u32 buff[240 * 160] = {0};

	while (running)
    {
		ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

		ImGui::Begin("Screen");
		GLuint screen_texture;
		glGenTextures(1, &screen_texture);
		glBindTexture(GL_TEXTURE_2D, screen_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, buff);
		ImGui::Image((void*) (intptr_t) screen_texture, ImVec2(SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2));
		ImGui::End();

		ImGui::Begin("State");
        ImGui::Text("r0: 0x%x", emulator.cpu->getRegister(r0));
		ImGui::Text("r1: 0x%x", emulator.cpu->getRegister(r1));
		ImGui::Text("r2: 0x%x", emulator.cpu->getRegister(r2));
		ImGui::Text("r3: 0x%x", emulator.cpu->getRegister(r3));
		ImGui::Text("r4: 0x%x", emulator.cpu->getRegister(r4));
		ImGui::Text("r5: 0x%x", emulator.cpu->getRegister(r5));
		ImGui::Text("r6: 0x%x", emulator.cpu->getRegister(r6));
		ImGui::Text("r7: 0x%x", emulator.cpu->getRegister(r7));
		ImGui::Text("r8: 0x%x", emulator.cpu->getRegister(r8));
		ImGui::Text("r9: 0x%x", emulator.cpu->getRegister(r9));
		ImGui::Text("r10: 0x%x", emulator.cpu->getRegister(r10));
		ImGui::Text("r11: 0x%x", emulator.cpu->getRegister(r11));
		ImGui::Text("r12: 0x%x", emulator.cpu->getRegister(r12));
		ImGui::Text("r13: 0x%x", emulator.cpu->getRegister(r13));
		ImGui::Text("r14: 0x%x", emulator.cpu->getRegister(r14));
		ImGui::Text("r15: 0x%x", emulator.cpu->getRegister(r15));
		ImGui::Text("CPSR: 0x%x", emulator.cpu->getRegister(16));
		ImGui::SameLine();
		std::string flags = "";
		if (emulator.cpu->getConditionCodeFlag(ConditionFlag::N)) flags += "N";
		if (emulator.cpu->getConditionCodeFlag(ConditionFlag::Z)) flags += "Z";
		if (emulator.cpu->getConditionCodeFlag(ConditionFlag::C)) flags += "C";
		if (emulator.cpu->getConditionCodeFlag(ConditionFlag::V)) flags += "V";
		ImGui::Text(flags.c_str());
		std::string state = emulator.cpu->getState() == State::ARM ? "ARM" : "THUMB";
		ImGui::Text("%s mode", state.c_str());
		ImGui::End();

		ImGui::Begin("Instructions");
		if (ImGui::Button("Step"))
			;
		ImGui::SameLine();
		if (ImGui::Button("Frame"))
		{
			emulator.frame();
			for (int y = 0; y < SCREEN_HEIGHT; y++)
			{
				for (int x = 0; x < SCREEN_WIDTH; x++)
				{
					buff[y * SCREEN_WIDTH + x] = emulator.ppu->screen_buffer[y][x] | 0xFF000000;
				}
			}
		}

		for (int i = -8; i < 8; i++)
		{
			//read32(registers.r15, false);
			u32 pc = emulator.cpu->getRegister(r15) + i * 4;

			if (i == 0)
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0xFF, 0, 0xFF));

			ImGui::Text("0x%x: ", pc);

			if (i == 0)
				ImGui::PopStyleColor();
		}

		ImGui::End();

        while (SDL_PollEvent(&e))
        {
			ImGui_ImplSDL2_ProcessEvent(&e);

            // Click X on window
            if (e.type == SDL_QUIT)
                running = false;
        
            // Key press event
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    running = false;

                emulator.gamepad->poll();
            }
        }

		ImGui::Render();
        glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
       	glClearColor(BG_COLOR.x * BG_COLOR.w, BG_COLOR.y * BG_COLOR.w, BG_COLOR.z * BG_COLOR.w, BG_COLOR.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

	ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
}