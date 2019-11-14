#import <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>

#include "os/os.h"
#include "graphics.h"

struct GraphicsData
{
    MTKView *view;
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
    id<MTLCommandBuffer> command_buffer;
    id<MTLRenderCommandEncoder> command_encoder;
    MTLRenderPassDescriptor *pass_descriptor;
    id<MTLLibrary> library;
    id<MTLDepthStencilState> depth_testing_on;
    id<MTLDepthStencilState> depth_testing_off;
};

struct Shader
{
    id<MTLRenderPipelineState> pipeline_state;
};

struct Mesh
{
    id<MTLBuffer> vertex_buffers[16];
    id<MTLBuffer> index_buffer;
    int num_indices;
    int num_vertex_buffers;
};

struct Texture
{
    id<MTLTexture> texture;
};


int graphics_value_sizes[] = {
#define GRAPHICS_VALUE_TYPE(name,num,size) num*size,
    GRAPHICS_VALUE_TYPES
#undef GRAPHICS_VALUE_TYPE
};

int graphics_value_nums[] = {
#define GRAPHICS_VALUE_TYPE(name,num,size) num,
    GRAPHICS_VALUE_TYPES
#undef GRAPHICS_VALUE_TYPE
};

struct GraphicsData *graphics_create(void * param)
{
    struct GraphicsData *graphics = calloc(1, sizeof(struct GraphicsData));
    graphics->view = (__bridge MTKView *)(param);
    //[graphics->mtk_view ]
    graphics->device = MTLCreateSystemDefaultDevice();
    [graphics->view setDevice:graphics->device];
    [graphics->view setColorPixelFormat:MTLPixelFormatBGRA8Unorm];
    [graphics->view setDepthStencilPixelFormat:MTLPixelFormatDepth32Float];
    graphics->command_queue = [graphics->device newCommandQueue];
    graphics->library = [graphics->device newDefaultLibrary];
    {
        MTLDepthStencilDescriptor *ds_descriptor = [MTLDepthStencilDescriptor new];
        [ds_descriptor setDepthCompareFunction:MTLCompareFunctionLess];
        [ds_descriptor setDepthWriteEnabled:true];
        graphics->depth_testing_on = [graphics->device newDepthStencilStateWithDescriptor:ds_descriptor];
    }
    {
        MTLDepthStencilDescriptor *ds_descriptor = [MTLDepthStencilDescriptor new];
        [ds_descriptor setDepthCompareFunction:MTLCompareFunctionAlways];
        [ds_descriptor setDepthWriteEnabled:false];
        graphics->depth_testing_off = [graphics->device newDepthStencilStateWithDescriptor:ds_descriptor];
    }
    return graphics;
}

void graphics_begin_render_pass(float *clear_rgba, struct GraphicsData *graphics)
{
    graphics->command_buffer = [graphics->command_queue commandBuffer];
    graphics->pass_descriptor = [graphics->view currentRenderPassDescriptor];
    [graphics->pass_descriptor.colorAttachments[0]
     setClearColor:MTLClearColorMake(clear_rgba[0], clear_rgba[1],
        clear_rgba[2], clear_rgba[3])];
    graphics->command_encoder = [graphics->command_buffer renderCommandEncoderWithDescriptor:graphics->pass_descriptor];
    [graphics->command_encoder setDepthStencilState:graphics->depth_testing_off];
   /*
    let commandBuffer = commandQueue.makeCommandBuffer()!
    if let renderPassDescriptor = view.currentRenderPassDescriptor, let drawable = view.currentDrawable {
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(1.0, 0, 1.0, 0)
        let commandEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)!
        */
        

}

void graphics_end_render_pass(struct GraphicsData *graphics)
{
    [graphics->command_encoder endEncoding];
    [graphics->command_buffer presentDrawable:[graphics->view currentDrawable] ];
    [graphics->command_buffer commit];
        /*
        commandEncoder.endEncoding()
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
         */
}

void graphics_clear(struct GraphicsData *graphics)
{
    //NOTE(Vidar):Metal will allways clear the screen I think
}

void graphics_render_mesh(struct Mesh *mesh, struct Shader *shader, struct GraphicsValueSpec *uniform_specs, int num_uniform_specs, struct GraphicsData *graphics)
{
    [graphics->command_encoder setRenderPipelineState:shader->pipeline_state];
    int buffer_index = 0;
    for(int i=0;i<mesh->num_vertex_buffers;i++){
        [graphics->command_encoder setVertexBuffer:mesh->vertex_buffers[i] offset:0 atIndex:buffer_index++];
    }
    for(int i=0;i<num_uniform_specs;i++){
        struct GraphicsValueSpec *spec = uniform_specs + i;
        //TODO(Vidar):Do we really need to set it both for the vertex and fragment stages?
        switch(spec->type){
            case GRAPHICS_VALUE_TEX2:
            {
                struct Texture *tex = spec->data;
                [graphics->command_encoder setFragmentTexture:tex->texture atIndex:buffer_index++];
                break;
            }
            default:
                [graphics->command_encoder setVertexBytes:spec->data length:spec->num*graphics_value_sizes[spec->type] atIndex:buffer_index];
                [graphics->command_encoder setFragmentBytes:spec->data length:spec->num*graphics_value_sizes[spec->type] atIndex:buffer_index++];
        }
    }
    [graphics->command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:mesh->num_indices indexType:MTLIndexTypeUInt32 indexBuffer:mesh->index_buffer indexBufferOffset:0];
}


struct Shader *graphics_compile_shader(const char *vert_filename, const char *frag_filename, enum GraphicsBlendMode blend_mode, char *error_buffer, int error_buffer_len, struct GraphicsData *graphics)
{
    struct Shader *shader = calloc(1, sizeof(struct Shader));
    id<MTLFunction> vert_function = [graphics->library newFunctionWithName:[[NSString stringWithUTF8String:vert_filename] stringByAppendingString:@"_vert"]];
    id<MTLFunction> frag_function = [graphics->library newFunctionWithName:[[NSString stringWithUTF8String:frag_filename] stringByAppendingString:@"_frag"]];
    
    MTLRenderPipelineDescriptor *pipeline_descriptor = [MTLRenderPipelineDescriptor new];
    [pipeline_descriptor setVertexFunction:vert_function];
    [pipeline_descriptor setFragmentFunction:frag_function];
    [pipeline_descriptor.colorAttachments[0] setPixelFormat:MTLPixelFormatBGRA8Unorm];
    switch(blend_mode){
        case GRAPHICS_BLEND_MODE_PREMUL:
            [pipeline_descriptor.colorAttachments[0] setBlendingEnabled:true];
            [pipeline_descriptor.colorAttachments[0] setRgbBlendOperation:MTLBlendOperationAdd];
            [pipeline_descriptor.colorAttachments[0] setAlphaBlendOperation:MTLBlendOperationAdd];
            [pipeline_descriptor.colorAttachments[0] setSourceRGBBlendFactor:MTLBlendFactorOne];
            [pipeline_descriptor.colorAttachments[0] setSourceAlphaBlendFactor:MTLBlendFactorOne];
            [pipeline_descriptor.colorAttachments[0] setDestinationRGBBlendFactor:MTLBlendFactorOneMinusSourceAlpha];
            [pipeline_descriptor.colorAttachments[0] setDestinationAlphaBlendFactor:MTLBlendFactorOneMinusSourceAlpha];
            break;
        case GRAPHICS_BLEND_MODE_MULTIPLY:
            [pipeline_descriptor.colorAttachments[0] setBlendingEnabled:true];
            [pipeline_descriptor.colorAttachments[0] setRgbBlendOperation:MTLBlendOperationAdd];
            [pipeline_descriptor.colorAttachments[0] setAlphaBlendOperation:MTLBlendOperationAdd];
            [pipeline_descriptor.colorAttachments[0] setSourceRGBBlendFactor:MTLBlendFactorDestinationColor];
            [pipeline_descriptor.colorAttachments[0] setSourceAlphaBlendFactor:MTLBlendFactorDestinationAlpha];
            [pipeline_descriptor.colorAttachments[0] setDestinationRGBBlendFactor:MTLBlendFactorZero];
            [pipeline_descriptor.colorAttachments[0] setDestinationAlphaBlendFactor:MTLBlendFactorZero];
            break;
        case GRAPHICS_BLEND_MODE_NONE:
            [pipeline_descriptor.colorAttachments[0] setBlendingEnabled:false];
            break;
            
    }
    [pipeline_descriptor setDepthAttachmentPixelFormat:MTLPixelFormatDepth32Float];
    NSError *error;
    shader->pipeline_state = [graphics->device newRenderPipelineStateWithDescriptor:pipeline_descriptor error:&error];
    if(error){
        NSString *error_string = error.localizedDescription;
        [error_string
            getBytes:error_buffer
            maxLength:(error_buffer_len - 1)
            usedLength:NULL
            encoding:NSUTF8StringEncoding
            options:0
            range:NSMakeRange(0, [error_string length])
            remainingRange:NULL
         ];
    }
    
    /*
    let defaultLibrary = device.makeDefaultLibrary()!
    let fragmentProgram = defaultLibrary.makeFunction(name: "basic_fragment")
    let vertexProgram = defaultLibrary.makeFunction(name: "basic_vertex")
     
     let pipelineStateDescriptor = MTLRenderPipelineDescriptor()
     pipelineStateDescriptor.vertexFunction = vertexProgram
     pipelineStateDescriptor.fragmentFunction = fragmentProgram
     pipelineStateDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
     
     // 3
     pipelineState = try! device.makeRenderPipelineState(descriptor: pipelineStateDescriptor)
     */
    return shader;
}

/*
static MTLVertexFormat get_uniform_format(enum ShaderUniformType type)
{
    switch (type) {
        case SHADER_UNIFORM_INT: return MTLVertexFormatInt;
        case SHADER_UNIFORM_FLOAT: return MTLVertexFormatFloat;
        case SHADER_UNIFORM_MAT4: return -1;
        case SHADER_UNIFORM_MAT3: return -1;
        case SHADER_UNIFORM_VEC4: return MTLVertexFormatFloat4;
        case SHADER_UNIFORM_VEC3: return -1;
        case SHADER_UNIFORM_VEC2: return MTLVertexFormatFloat2;
        case SHADER_UNIFORM_HALF: return MTLVertexFormatHalf;
        case SHADER_UNIFORM_HALF2: return MTLVertexFormatHalf2;
        case SHADER_UNIFORM_HALF3: return -1;
        default: return -1;
    }
}
*/


struct Mesh *graphics_create_mesh(struct GraphicsValueSpec *value_specs, uint32_t num_value_specs, uint32_t num_verts, int *index_data, uint32_t num_indices, struct GraphicsData *graphics)
{
    struct Mesh *mesh = calloc(1, sizeof(struct Mesh));
    mesh->num_vertex_buffers = num_value_specs;
    mesh->num_indices = num_indices;
    
    assert(num_value_specs < 16);
    
    for (int i = 0; i < num_value_specs; i++) {
        struct GraphicsValueSpec spec = value_specs[i];
        mesh->vertex_buffers[i] = [graphics->device newBufferWithBytes:spec.data
            length:graphics_value_sizes[spec.type]*num_verts
            options:MTLResourceCPUCacheModeDefaultCache];
    }
    mesh->index_buffer = [graphics->device newBufferWithBytes:index_data length:num_indices*4 options:MTLResourceCPUCacheModeDefaultCache];
    return mesh;
}



struct Texture *graphics_create_texture(uint8_t *texture_data, uint32_t w, uint32_t h, uint32_t format, struct GraphicsData *graphics)
{
    struct Texture *tex = calloc(1, sizeof(struct Texture));
    
    MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];

    //TODO(Vidar): specify format depending on "format" parameter
    textureDescriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;

    // Set the pixel dimensions of the texture
    textureDescriptor.width = w;
    textureDescriptor.height = h;

    tex->texture = [graphics->device newTextureWithDescriptor:textureDescriptor];
    
    MTLRegion region = {
        { 0, 0, 0 },// MTLOrigin
        {w, h, 1}   // MTLSize
    };
    
    [tex->texture replaceRegion:region mipmapLevel:0 withBytes:texture_data
                    bytesPerRow:4*w];
    
    return tex;
}


void graphics_set_depth_test(int enabled, struct GraphicsData *graphics)
{
    if(enabled){
            [graphics->command_encoder setDepthStencilState:graphics->depth_testing_on];
    }else{
            [graphics->command_encoder setDepthStencilState:graphics->depth_testing_off];
    }
}

