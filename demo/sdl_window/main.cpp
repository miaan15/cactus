#include <SDL3/SDL.h>

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow("SDL_GPU Example", 800, 600, 0);

    // 1. Create the GPU Device
    // We specify which shader formats we support (SPIRV for Vulkan, MSL for Metal, DXIL for DX12)
    SDL_GPUDevice *device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_DXIL,
        true, // Enable debug mode (very helpful!)
        NULL);

    // 2. Claim the window for the device
    SDL_ClaimWindowForGPUDevice(device, window);

    bool quit = false;
    SDL_Event event;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) quit = true;
        }

        // --- THE RENDER PASS ---

        // A. Acquire a Command Buffer (the "to-do list" for the GPU)
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);

        // B. Acquire the Swapchain Texture (the actual "backbuffer" image)
        SDL_GPUTexture *swapchainTexture;
        if (SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchainTexture, NULL, NULL)) {

            // C. Setup the Render Pass (Defining the "Clear" color)
            SDL_GPUColorTargetInfo color_target = {0};
            color_target.texture = swapchainTexture;
            color_target.clear_color = (SDL_FColor){0.2f, 0.4f, 0.8f, 1.0f}; // Cornflower Blue
            color_target.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmd, &color_target, 1, NULL);

            /* THIS IS WHERE YOU DRAW:
               SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
               SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
            */

            SDL_EndGPURenderPass(render_pass);
        }

        // D. Submit the commands to the hardware
        SDL_SubmitGPUCommandBuffer(cmd);
    }

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
