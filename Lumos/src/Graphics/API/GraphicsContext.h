#pragma once
#include "lmpch.h"
#include "Core/OS/Window.h"

namespace Lumos
{
	namespace Graphics
	{

        enum class RenderAPI : u32
		{
			OPENGL = 0,
			VULKAN,
			DIRECT3D, //Unsupported
			METAL, //Unsupported
			NONE, //Unsupported
		};

		class LUMOS_EXPORT GraphicsContext
		{
		public:
			virtual ~GraphicsContext();

			static void Create(const WindowProperties& properties, void* deviceContext);
			static void Release();

			static RenderAPI GetRenderAPI() { return s_RenderAPI; }
			static void SetRenderAPI(RenderAPI api);

			virtual void Init() = 0;
			virtual void Present() = 0;

			virtual size_t GetMinUniformBufferOffsetAlignment() const = 0;

			static GraphicsContext* GetContext() { return s_Context; }
            virtual bool FlipImGUITexture() const = 0;
            virtual void WaitIdle() const = 0;
			virtual void OnImGui() = 0;

        protected:
            static GraphicsContext* (*CreateFunc)(const WindowProperties&, void*);

			static GraphicsContext* s_Context;
			static RenderAPI s_RenderAPI;

		};
	}
}



