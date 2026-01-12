#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include <ui/ui.hpp>

UI::UI(SDL_Window *window) {
	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |=
			ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |=
			ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable
	// Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(
			main_scale); // Bake a fixed style scale. (until we have a solution for
						 // dynamic style scaling, changing this requires resetting
						 // Style + calling this again)
	style.FontScaleDpi =
			main_scale; // Set initial font scale. (using
						// io.ConfigDpiScaleFonts=true makes this unnecessary. We
						// leave both here for documentation purpose)
	io.ConfigDpiScaleFonts =
			true; // [Experimental] Automatically overwrite style.FontScaleDpi in
				  // Begin() when Monitor DPI changes. This will scale fonts but
				  // _NOT_ scale sizes/padding for now.
	// io.ConfigDpiScaleViewports =
	//     true; // [Experimental] Scale Dear ImGui and Platform Windows when
	// Monitor DPI changes.

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform
	// windows can look identical to regular ones.
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	SDL_ClaimWindowForGPUDevice(RE::getGPUDevice(),
			window); //! remove once imgui backend changed
	ImGui_ImplSDL3_InitForSDLGPU(window);
	ImGui_ImplSDLGPU3_InitInfo init_info = {};
	init_info.Device = RE::getGPUDevice();
	init_info.ColorTargetFormat =
			SDL_GetGPUSwapchainTextureFormat(init_info.Device, window);
	init_info.MSAASamples =
			SDL_GPU_SAMPLECOUNT_1; // Only used in multi-viewports mode.
	init_info.SwapchainComposition =
			SDL_GPU_SWAPCHAINCOMPOSITION_SDR; // Only used in multi-viewports mode.
	init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
	ImGui_ImplSDLGPU3_Init(&init_info);
}

UI::~UI() {
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplSDLGPU3_Shutdown();
	ImGui::DestroyContext();
}

SDL_AppResult UI::processEvent(SDL_Event *event) {
	return ImGui_ImplSDL3_ProcessEvent(event) ? SDL_APP_CONTINUE
											  : SDL_APP_SUCCESS;
}

void UI::beginFrame() {
	// Start the Dear ImGui frame
	ImGui_ImplSDLGPU3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair
	// to create a named window.

	ImGui::DockSpaceOverViewport();
	ImGuiIO &io = ImGui::GetIO();
}

void UI::endFrame(SDL_Window *window) {
	// Rendering
	ImGuiIO &io = ImGui::GetIO();
	ImGui::Render();
	ImDrawData *draw_data = ImGui::GetDrawData();
	const bool is_minimized =
			(draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

	SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(
			RE::getGPUDevice()); // Acquire a GPU command buffer

	SDL_GPUTexture *swapchain_texture;
	SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window,
			&swapchain_texture, nullptr,
			nullptr); // Acquire a swapchain texture

	if (swapchain_texture != nullptr && !is_minimized) {
		// This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the
		// vertex/index buffer!
		ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

		// Setup and start a render pass
		SDL_GPUColorTargetInfo target_info = {};
		target_info.texture = swapchain_texture;
		target_info.clear_color = SDL_FColor{ 0.0, 0.0, 0.0, 1.0 };
		target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		target_info.store_op = SDL_GPU_STOREOP_STORE;
		target_info.mip_level = 0;
		target_info.layer_or_depth_plane = 0;
		target_info.cycle = false;
		SDL_GPURenderPass *render_pass =
				SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

		// Render ImGui
		ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

		SDL_EndGPURenderPass(render_pass);
	}

	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	// Submit the command buffer
	SDL_SubmitGPUCommandBuffer(command_buffer);
}