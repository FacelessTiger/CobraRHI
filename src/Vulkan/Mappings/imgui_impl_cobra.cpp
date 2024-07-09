#include <imgui_impl_cobra.h>
#include <imgui.h>

#include "imgui_shaders/SharedShaderDefs.inl"

#include <source_location>

struct ImGui_ImplCobra_FrameRenderBuffers
{
	std::unique_ptr<Cobra::Buffer> VertexBuffer = nullptr;
	std::unique_ptr<Cobra::Buffer> IndexBuffer = nullptr;
};

struct ImGui_ImplCobra_WindowRenderBuffers
{
	uint32_t Index;
	uint32_t Count;
	ImGui_ImplCobra_FrameRenderBuffers* FrameRenderBuffers = nullptr;
};

struct ImGui_ImplCobraH_Window
{
	std::unique_ptr<Cobra::Swapchain> Swapchain;
	Cobra::SyncPoint SyncPoint[2];

	uint32_t FrameIndex = 0;
};

struct ImGui_ImplCobra_ViewportData
{
	ImGui_ImplCobraH_Window Window; // Used by secondary viewports only
	ImGui_ImplCobra_WindowRenderBuffers RenderBuffers; // Used by all viewports
	bool WindowOwned = false;

	ImGui_ImplCobra_ViewportData() { memset(&RenderBuffers, 0, sizeof(RenderBuffers)); }
};

struct ImGui_ImplCobra_Data
{
	ImGui_ImplCobra_InitInfo CobraInitInfo;

	std::unique_ptr<Cobra::Sampler> FontSampler;
	std::unique_ptr<Cobra::Image> FontImage;
	std::unique_ptr<Cobra::Shader> Shader;
};

void ImGui_ImplCobra_DestroyWindowRenderBuffers(ImGui_ImplCobra_WindowRenderBuffers* buffers)
{
	if (buffers->FrameRenderBuffers)
		delete[] buffers->FrameRenderBuffers;
	buffers->FrameRenderBuffers = nullptr;
}

static ImGui_ImplCobra_Data* ImGui_ImplCobra_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplCobra_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

static void ImGui_ImplCobra_CreateWindow(ImGuiViewport* viewport)
{
	ImGui_ImplCobra_Data* bd = ImGui_ImplCobra_GetBackendData();
	ImGui_ImplCobra_ViewportData* vd = IM_NEW(ImGui_ImplCobra_ViewportData)();
	viewport->RendererUserData = vd;
	ImGui_ImplCobraH_Window* wd = &vd->Window;
	ImGui_ImplCobra_InitInfo* v = &bd->CobraInitInfo;

	wd->Swapchain = std::make_unique<Cobra::Swapchain>(*v->Context, Platform_GetNativeWindow(viewport), Cobra::uVec2{(uint32_t)viewport->Size.x, (uint32_t)viewport->Size.y}, false);
	vd->WindowOwned = true;
}

static void ImGui_ImplCobra_DestroyWindow(ImGuiViewport* viewport)
{
	// The main viewport (owned by the application) will always have RendererUserData == 0 since we didn't create the data for it.
	ImGui_ImplCobra_Data* bd = ImGui_ImplCobra_GetBackendData();
	ImGui_ImplCobra_ViewportData* vd = (ImGui_ImplCobra_ViewportData*)viewport->RendererUserData;

	if (vd)
	{
		ImGui_ImplCobra_InitInfo* v = &bd->CobraInitInfo;

		ImGui_ImplCobra_DestroyWindowRenderBuffers(&vd->RenderBuffers);
		IM_DELETE(vd);
	}

	viewport->RendererUserData = nullptr;
}

static void ImGui_ImplCobra_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
	ImGui_ImplCobra_Data* bd = ImGui_ImplCobra_GetBackendData();
	ImGui_ImplCobra_ViewportData* vd = (ImGui_ImplCobra_ViewportData*)viewport->RendererUserData;

	if (!vd) // Nullptr for the main viewport (users responsibility)
		return;

	ImGui_ImplCobraH_Window* wd = &vd->Window;
	Cobra::Swapchain& swapchain = *wd->Swapchain.get();
	swapchain.Resize({ (uint32_t)size.x, (uint32_t)size.y });
}

static void ImGui_ImplCobra_RenderWindow(ImGuiViewport* viewport, void*)
{
	ImGui_ImplCobra_Data* bd = ImGui_ImplCobra_GetBackendData();
	ImGui_ImplCobra_ViewportData* vd = (ImGui_ImplCobra_ViewportData*)viewport->RendererUserData;
	ImGui_ImplCobraH_Window* wd = &vd->Window;
	ImGui_ImplCobra_InitInfo* v = &bd->CobraInitInfo;

	Cobra::Swapchain& swapchain = *wd->Swapchain.get();
	Cobra::Queue& queue = *v->Queue;

	wd->FrameIndex = (wd->FrameIndex + 1) % 2;
	wd->SyncPoint[wd->FrameIndex].Wait();
	queue.Acquire(swapchain);

	auto cmd = queue.Begin();
	cmd.BeginRendering(swapchain.GetSize(), { swapchain.GetCurrent() });
	cmd.ClearColorAttachment(0, Cobra::Vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, swapchain.GetSize());
	ImGui_ImplCobra_RenderDrawData(viewport->DrawData, cmd);
	cmd.EndRendering();

	cmd.Present(swapchain);
	wd->SyncPoint[wd->FrameIndex] = queue.Submit(cmd, {});
	queue.Present(swapchain, {});
}

bool ImGui_ImplCobra_CreateDeviceObjects()
{
	ImGui_ImplCobra_Data* bd = ImGui_ImplCobra_GetBackendData();
	ImGui_ImplCobra_InitInfo* v = &bd->CobraInitInfo;

	if (!bd->FontSampler)
		bd->FontSampler = std::make_unique<Cobra::Sampler>(*v->Context, Cobra::Filter::Linear, Cobra::Filter::Linear);

	std::string filePath = __FILE__;
	bd->Shader = std::make_unique<Cobra::Shader>(*v->Context, filePath.substr(0, filePath.rfind("\\")) + "\\imgui_shaders\\ImGui.slang");

	return true;
}

void ImGui_ImplCobra_InitPlatformInterface()
{
	ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
	platformIO.Renderer_CreateWindow = ImGui_ImplCobra_CreateWindow;
	platformIO.Renderer_DestroyWindow = ImGui_ImplCobra_DestroyWindow;
	platformIO.Renderer_SetWindowSize = ImGui_ImplCobra_SetWindowSize;
	platformIO.Renderer_RenderWindow = ImGui_ImplCobra_RenderWindow;
	//platformIO.Renderer_SwapBuffers = [](ImGuiViewport* viewport, void*) {};
}

bool ImGui_ImplCobra_Init(ImGui_ImplCobra_InitInfo* info)
{
	ImGuiIO& io = ImGui::GetIO();
	IMGUI_CHECKVERSION();
	IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

	ImGui_ImplCobra_Data* bd = IM_NEW(ImGui_ImplCobra_Data);
	io.BackendRendererUserData = bd;
	io.BackendRendererName = "imgui_impl_cobra";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

	bd->CobraInitInfo = *info;
	ImGui_ImplCobra_CreateDeviceObjects();

	ImGuiViewport* mainViewport = ImGui::GetMainViewport();
	mainViewport->RendererUserData = IM_NEW(ImGui_ImplCobra_ViewportData)();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		ImGui_ImplCobra_InitPlatformInterface();

	return true;
}

bool ImGui_ImplCobra_CreateFontsTexture()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplCobra_Data* bd = ImGui_ImplCobra_GetBackendData();
	ImGui_ImplCobra_InitInfo* v = &bd->CobraInitInfo;

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	size_t uploadSize = width * height * 4 * sizeof(char);

	bd->FontImage = std::make_unique<Cobra::Image>(*v->Context, Cobra::uVec2 { (uint32_t)width, (uint32_t)height }, Cobra::ImageFormat::R8G8B8A8_UNORM, Cobra::ImageUsage::Sampled | Cobra::ImageUsage::TransferDst);
	Cobra::Buffer uploadBuffer(*v->Context, uploadSize, Cobra::BufferUsage::TransferSrc, Cobra::BufferFlags::Mapped);
	memcpy(uploadBuffer.GetHostAddress(), pixels, uploadSize);

	auto cmd = v->Queue->Begin();
	cmd.CopyToImage(uploadBuffer, *bd->FontImage);
	v->Queue->Submit(cmd, {});
	v->Queue->WaitIdle();
	
	io.Fonts->SetTexID((ImTextureID)(uint64_t)bd->FontImage->GetHandle());
	return true;
}

void ImGui_ImplCobra_NewFrame()
{
	ImGui_ImplCobra_Data* bd = ImGui_ImplCobra_GetBackendData();
	IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplCobra_Init()?");

	if (!bd->FontImage)
		ImGui_ImplCobra_CreateFontsTexture();
}

void ImGui_ImplCobra_RenderDrawData(ImDrawData* drawData, Cobra::CommandList& cmd)
{
	int fbWidth = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	int fbHeight = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (fbWidth <= 0 || fbHeight <= 0) return;
	if (drawData->TotalVtxCount <= 0) return;

	ImGui_ImplCobra_Data* bd = ImGui_ImplCobra_GetBackendData();
	ImGui_ImplCobra_InitInfo* v = &bd->CobraInitInfo;

	ImGui_ImplCobra_ViewportData* viewportRendererData = (ImGui_ImplCobra_ViewportData*)drawData->OwnerViewport->RendererUserData;
	ImGui_ImplCobra_WindowRenderBuffers* wrb = &viewportRendererData->RenderBuffers;

	if (!wrb->FrameRenderBuffers)
	{
		wrb->Index = 0;
		wrb->Count = 2;

		wrb->FrameRenderBuffers = new ImGui_ImplCobra_FrameRenderBuffers[wrb->Count];
		memset(wrb->FrameRenderBuffers, 0, sizeof(ImGui_ImplCobra_FrameRenderBuffers) * wrb->Count);
	}

	wrb->Index = (wrb->Index + 1) % wrb->Count;
	ImGui_ImplCobra_FrameRenderBuffers* rb = &wrb->FrameRenderBuffers[wrb->Index];

	size_t vertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
	size_t indexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

	if (!rb->VertexBuffer || rb->VertexBuffer->GetSize() < vertexSize)
		rb->VertexBuffer = std::make_unique<Cobra::Buffer>(*v->Context, vertexSize, Cobra::BufferUsage::StorageBuffer, Cobra::BufferFlags::Mapped);
	if (!rb->IndexBuffer || rb->IndexBuffer->GetSize() < indexSize)
		rb->IndexBuffer = std::make_unique<Cobra::Buffer>(*v->Context, indexSize, Cobra::BufferUsage::IndexBuffer, Cobra::BufferFlags::Mapped);

	ImDrawVert* vtxDst = (ImDrawVert*)rb->VertexBuffer->GetHostAddress();
	ImDrawIdx* idxDst = (ImDrawIdx*)rb->IndexBuffer->GetHostAddress();

	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* cmdList = drawData->CmdLists[n];
		memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

		vtxDst += cmdList->VtxBuffer.Size;
		idxDst += cmdList->IdxBuffer.Size;
	}

	cmd.SetDefaultState();
	cmd.SetViewport({ fbWidth, fbHeight });
	cmd.EnableColorBlend(Cobra::BlendFactor::SrcAlpha, Cobra::BlendFactor::OneMinusSrcAlpha, Cobra::BlendOp::Add, Cobra::BlendFactor::One, Cobra::BlendFactor::Zero, Cobra::BlendOp::Add);
	cmd.BindIndexBuffer(*rb->IndexBuffer, sizeof(ImDrawIdx) == sizeof(uint16_t) ? Cobra::IndexType::Uint16 : Cobra::IndexType::Uint32);
	cmd.BindShaders({ *bd->Shader });

	Cobra::Vec2 scale = { 2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y };
	Cobra::Vec2 translate = { -1.0f - drawData->DisplayPos.x * scale.x, -1.0f - drawData->DisplayPos.y * scale.y };

	ImVec2 clipOff = drawData->DisplayPos;
	ImVec2 clipScale = drawData->FramebufferScale;

	int globalVtxOffset = 0;
	int globalIdxOffset = 0;
	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* cmdList = drawData->CmdLists[n];
		for (int cmdi = 0; cmdi < cmdList->CmdBuffer.Size; cmdi++)
		{
			const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdi];
			
			// Project scissor/clipping rectangles into framebuffer space
			ImVec2 clip_min((pcmd->ClipRect.x - clipOff.x) * clipScale.x, (pcmd->ClipRect.y - clipOff.y) * clipScale.y);
			ImVec2 clip_max((pcmd->ClipRect.z - clipOff.x) * clipScale.x, (pcmd->ClipRect.w - clipOff.y) * clipScale.y);

			// Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
			if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
			if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
			if (clip_max.x > fbWidth) { clip_max.x = (float)fbWidth; }
			if (clip_max.y > fbHeight) { clip_max.y = (float)fbHeight; }
			if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
				continue;

			cmd.SetScissor({ int32_t(clip_max.x - clip_min.x), int32_t(clip_max.y - clip_min.y) }, { int32_t(clip_min.x), int32_t(clip_min.y) });
			cmd.PushConstant(Slang::DrawPush{
				.vertices = (Slang::Vertex*)rb->VertexBuffer->GetDeviceAddress(),
				.scale = scale,
				.translate = translate,
				.image = (uint32_t)(uint64_t)pcmd->TextureId | (bd->FontSampler->GetHandle() << 20)
			});
			cmd.DrawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + globalIdxOffset, pcmd->VtxOffset + globalVtxOffset, 0);
		}

		globalVtxOffset += cmdList->VtxBuffer.Size;
		globalIdxOffset += cmdList->IdxBuffer.Size;
	}
}

void ImGui_ImplCobra_Shutdown()
{
	ImGui_ImplCobra_Data* bd = ImGui_ImplCobra_GetBackendData();
	IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
	ImGuiIO& io = ImGui::GetIO();
	ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

	for (int n = 0; n < platformIO.Viewports.Size; n++)
	{
		ImGui_ImplCobra_ViewportData* vd = (ImGui_ImplCobra_ViewportData*)platformIO.Viewports[n]->RendererUserData;
		if (vd)
			ImGui_ImplCobra_DestroyWindowRenderBuffers(&vd->RenderBuffers);
	}

	ImGuiViewport* mainViewport = ImGui::GetMainViewport();
	ImGui_ImplCobra_ViewportData* vd = (ImGui_ImplCobra_ViewportData*)mainViewport->RendererUserData;
	if (vd)
		IM_DELETE(vd);
	mainViewport->RendererUserData = nullptr;

	ImGui::DestroyPlatformWindows();

	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
	io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasViewports);
	IM_DELETE(bd);
}