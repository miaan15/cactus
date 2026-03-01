#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_main.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <filesystem>
#include <iostream>

import Common;

namespace stdf = std::filesystem;

stdf::path cur_dir = cactus::root_dir / "demo/sdl_window";

int main(int argc, char *argv[]) {
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow("SDL3 GPU Window", 1280, 720, SDL_WINDOW_RESIZABLE);

    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create GPU device
    SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);

    if (!device) {
        std::cerr << "Failed to create GPU device: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Claim window
    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        std::cerr << "Failed to claim window: " << SDL_GetError() << std::endl;
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load shaders using your paths
    stdf::path vert_shader_path = cur_dir / "tri.vert.spv";
    stdf::path frag_shader_path = cur_dir / "tri.frag.spv";

    // Read vertex shader
    SDL_IOStream *vert_file = SDL_IOFromFile(vert_shader_path.string().c_str(), "rb");
    if (!vert_file) {
        std::cerr << "Failed to open vertex shader: " << SDL_GetError() << std::endl;
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Sint64 vert_size = SDL_GetIOSize(vert_file);
    void *vert_code = SDL_malloc(vert_size);
    SDL_ReadIO(vert_file, vert_code, vert_size);
    SDL_CloseIO(vert_file);

    // Read fragment shader
    SDL_IOStream *frag_file = SDL_IOFromFile(frag_shader_path.string().c_str(), "rb");
    if (!frag_file) {
        std::cerr << "Failed to open fragment shader: " << SDL_GetError() << std::endl;
        SDL_free(vert_code);
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Sint64 frag_size = SDL_GetIOSize(frag_file);
    void *frag_code = SDL_malloc(frag_size);
    SDL_ReadIO(frag_file, frag_code, frag_size);
    SDL_CloseIO(frag_file);

    // Create vertex shader
    SDL_GPUShaderCreateInfo vert_shader_info = {};
    vert_shader_info.code = (const Uint8 *)vert_code;
    vert_shader_info.code_size = vert_size;
    vert_shader_info.entrypoint = "main";
    vert_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vert_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vert_shader_info.num_samplers = 0;
    vert_shader_info.num_storage_textures = 0;
    vert_shader_info.num_storage_buffers = 0;
    vert_shader_info.num_uniform_buffers = 0;

    SDL_GPUShader *vert_shader = SDL_CreateGPUShader(device, &vert_shader_info);
    SDL_free(vert_code);

    if (!vert_shader) {
        std::cerr << "Failed to create vertex shader: " << SDL_GetError() << std::endl;
        SDL_free(frag_code);
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create fragment shader
    SDL_GPUShaderCreateInfo frag_shader_info = {};
    frag_shader_info.code = (const Uint8 *)frag_code;
    frag_shader_info.code_size = frag_size;
    frag_shader_info.entrypoint = "main";
    frag_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    frag_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    frag_shader_info.num_samplers = 0;
    frag_shader_info.num_storage_textures = 0;
    frag_shader_info.num_storage_buffers = 0;
    frag_shader_info.num_uniform_buffers = 0;

    SDL_GPUShader *frag_shader = SDL_CreateGPUShader(device, &frag_shader_info);
    SDL_free(frag_code);

    if (!frag_shader) {
        std::cerr << "Failed to create fragment shader: " << SDL_GetError() << std::endl;
        SDL_ReleaseGPUShader(device, vert_shader);
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create graphics pipeline
    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};

    // Vertex input state (empty for fullscreen triangle)
    SDL_GPUVertexInputState vertex_input_state = {};
    vertex_input_state.num_vertex_buffers = 0;
    vertex_input_state.num_vertex_attributes = 0;

    pipeline_info.vertex_input_state = vertex_input_state;
    pipeline_info.vertex_shader = vert_shader;
    pipeline_info.fragment_shader = frag_shader;

    // Primitive type
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    // Rasterizer state
    SDL_GPURasterizerState rasterizer_state = {};
    rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipeline_info.rasterizer_state = rasterizer_state;

    // Multisample state
    SDL_GPUMultisampleState multisample_state = {};
    multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    pipeline_info.multisample_state = multisample_state;

    // Depth stencil state
    SDL_GPUDepthStencilState depth_stencil_state = {};
    depth_stencil_state.enable_depth_test = false;
    depth_stencil_state.enable_depth_write = false;
    depth_stencil_state.enable_stencil_test = false;
    pipeline_info.depth_stencil_state = depth_stencil_state;

    // Color target
    SDL_GPUColorTargetDescription color_target = {};
    color_target.format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUColorTargetBlendState blend_state = {};
    blend_state.enable_blend = false;
    blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    blend_state.color_write_mask = 0xF;

    color_target.blend_state = blend_state;

    SDL_GPUGraphicsPipelineTargetInfo target_info = {};
    target_info.num_color_targets = 1;
    target_info.color_target_descriptions = &color_target;
    target_info.has_depth_stencil_target = false;

    pipeline_info.target_info = target_info;

    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);

    if (!pipeline) {
        std::cerr << "Failed to create graphics pipeline: " << SDL_GetError() << std::endl;
        SDL_ReleaseGPUShader(device, frag_shader);
        SDL_ReleaseGPUShader(device, vert_shader);
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Main loop
    bool running = true;
    SDL_Event event;

    std::cout << "Window created successfully! Press ESC or close window to exit." << std::endl;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        // Acquire swapchain texture
        SDL_GPUCommandBuffer *cmd_buffer = SDL_AcquireGPUCommandBuffer(device);
        if (!cmd_buffer) {
            std::cerr << "Failed to acquire command buffer" << std::endl;
            continue;
        }

        SDL_GPUTexture *swapchain_texture;
        if (!SDL_AcquireGPUSwapchainTexture(cmd_buffer, window, &swapchain_texture, nullptr,
                                            nullptr)) {
            std::cerr << "Failed to acquire swapchain texture" << std::endl;
            SDL_SubmitGPUCommandBuffer(cmd_buffer);
            continue;
        }

        if (swapchain_texture) {
            // Begin render pass
            SDL_GPUColorTargetInfo color_target_info = {};
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass *render_pass =
                SDL_BeginGPURenderPass(cmd_buffer, &color_target_info, 1, nullptr);

            // Bind pipeline and draw
            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
            SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

            // End render pass
            SDL_EndGPURenderPass(render_pass);
        }

        // Submit command buffer
        SDL_SubmitGPUCommandBuffer(cmd_buffer);
    }

    // Cleanup
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseGPUShader(device, frag_shader);
    SDL_ReleaseGPUShader(device, vert_shader);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
