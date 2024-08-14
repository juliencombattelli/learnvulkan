#include "Shader.hpp" // IWYU pragma: keep

#include <stdexcept>

struct GlslangContext {
    GlslangContext()
    {
        if (!glslang_initialize_process()) {
            throw std::runtime_error("Unable to initialize Glslang");
        }
    }
    ~GlslangContext()
    {
        glslang_finalize_process();
    }
};

GlslangContext& makeGlslangContext()
{
    static GlslangContext context;
    return context;
}

static GlslangContext& context = makeGlslangContext();
