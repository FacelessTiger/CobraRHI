#include <ImGuiBackend/imgui_impl_cobra.h>
#include <imgui.h>

#include "shaders/SharedShaderDefs.inl"

#include <array>

// include/ImGuiBackend/shaders/Imgui.slang
static constexpr auto SHADER_SPIRV = std::to_array<uint32_t>({
	0x00000002, 0x00000000, 0x74726576, 0x614d7865, 0x01006e69, 0x70000000, 0x6c657869, 0x6e69614d,
	0x00000000, 0x07230203, 0x00010500, 0x00000028, 0x0000006e, 0x00000000, 0x00020011, 0x0000115a,
	0x00020011, 0x000014e3, 0x00020011, 0x000014b6, 0x00020011, 0x000014b5, 0x00020011, 0x00000001,
	0x0008000a, 0x5f565053, 0x5f52484b, 0x69726176, 0x656c6261, 0x696f705f, 0x7265746e, 0x00000073,
	0x0009000a, 0x5f565053, 0x5f52484b, 0x73796870, 0x6c616369, 0x6f74735f, 0x65676172, 0x6675625f,
	0x00726566, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e,
	0x000014e4, 0x00000001, 0x000b000f, 0x00000000, 0x00000002, 0x74726576, 0x614d7865, 0x00006e69,
	0x00000003, 0x00000004, 0x00000005, 0x00000006, 0x00000007, 0x000c000f, 0x00000004, 0x00000008,
	0x65786970, 0x69614d6c, 0x0000006e, 0x00000003, 0x00000009, 0x0000000a, 0x0000000b, 0x0000000c,
	0x0000000d, 0x00030010, 0x00000008, 0x00000007, 0x00030003, 0x0000000b, 0x00000001, 0x00060005,
	0x0000000e, 0x74726556, 0x754f7865, 0x74757074, 0x00000000, 0x00060006, 0x0000000e, 0x00000000,
	0x69736f70, 0x6e6f6974, 0x00000000, 0x00050006, 0x0000000e, 0x00000001, 0x6f6c6f63, 0x00000072,
	0x00040006, 0x0000000e, 0x00000002, 0x00007675, 0x00090005, 0x0000000f, 0x72626f43, 0x6d492e61,
	0x48656761, 0x6c646e61, 0x616e5f65, 0x61727574, 0x0000006c, 0x00050006, 0x0000000f, 0x00000000,
	0x646e6168, 0x0000656c, 0x00090005, 0x00000010, 0x75476d49, 0x6c532e69, 0x2e676e61, 0x68737550,
	0x74616e5f, 0x6c617275, 0x00000000, 0x00060006, 0x00000010, 0x00000000, 0x74726576, 0x73656369,
	0x00000000, 0x00050006, 0x00000010, 0x00000001, 0x6c616373, 0x00000065, 0x00060006, 0x00000010,
	0x00000002, 0x6e617274, 0x74616c73, 0x00000065, 0x00050006, 0x00000010, 0x00000003, 0x67616d69,
	0x00000065, 0x00040005, 0x00000003, 0x68737570, 0x00000000, 0x00090005, 0x00000011, 0x75476d49,
	0x6c532e69, 0x2e676e61, 0x74726556, 0x6e5f7865, 0x72757461, 0x00006c61, 0x00060006, 0x00000011,
	0x00000000, 0x69736f70, 0x6e6f6974, 0x00000000, 0x00040006, 0x00000011, 0x00000001, 0x00007675,
	0x00050006, 0x00000011, 0x00000002, 0x6f6c6f63, 0x00000072, 0x00040005, 0x00000012, 0x74726576,
	0x00007865, 0x00070005, 0x00000013, 0x75476d49, 0x6c532e69, 0x2e676e61, 0x74726556, 0x00007865,
	0x00060006, 0x00000013, 0x00000000, 0x69736f70, 0x6e6f6974, 0x00000000, 0x00040006, 0x00000013,
	0x00000001, 0x00007675, 0x00050006, 0x00000013, 0x00000002, 0x6f6c6f63, 0x00000072, 0x000b0005,
	0x00000005, 0x72746e65, 0x696f5079, 0x6150746e, 0x5f6d6172, 0x74726576, 0x614d7865, 0x632e6e69,
	0x726f6c6f, 0x00000000, 0x000a0005, 0x00000006, 0x72746e65, 0x696f5079, 0x6150746e, 0x5f6d6172,
	0x74726576, 0x614d7865, 0x752e6e69, 0x00000076, 0x00050005, 0x00000002, 0x74726576, 0x614d7865,
	0x00006e69, 0x00070005, 0x00000014, 0x72626f43, 0x6d492e61, 0x48656761, 0x6c646e61, 0x00000065,
	0x00050006, 0x00000014, 0x00000000, 0x646e6168, 0x0000656c, 0x00050005, 0x0000000c, 0x74726576,
	0x752e7865, 0x00000076, 0x00060005, 0x0000000d, 0x65545f67, 0x72757478, 0x61654865, 0x00000070,
	0x00060005, 0x00000009, 0x61535f67, 0x656c706d, 0x61654872, 0x00000070, 0x00060005, 0x0000000b,
	0x74726576, 0x632e7865, 0x726f6c6f, 0x00000000, 0x00090005, 0x0000000a, 0x72746e65, 0x696f5079,
	0x6150746e, 0x5f6d6172, 0x65786970, 0x69614d6c, 0x0000006e, 0x00050005, 0x00000008, 0x65786970,
	0x69614d6c, 0x0000006e, 0x00050048, 0x0000000e, 0x00000000, 0x00000023, 0x00000000, 0x00050048,
	0x0000000e, 0x00000001, 0x00000023, 0x00000010, 0x00050048, 0x0000000e, 0x00000002, 0x00000023,
	0x00000020, 0x00040047, 0x00000007, 0x0000000b, 0x0000002a, 0x00040047, 0x00000015, 0x00000006,
	0x00000014, 0x00050048, 0x0000000f, 0x00000000, 0x00000023, 0x00000000, 0x00030047, 0x00000010,
	0x00000002, 0x00050048, 0x00000010, 0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x00000010,
	0x00000001, 0x00000023, 0x00000008, 0x00050048, 0x00000010, 0x00000002, 0x00000023, 0x00000010,
	0x00050048, 0x00000010, 0x00000003, 0x00000023, 0x00000018, 0x00050048, 0x00000011, 0x00000000,
	0x00000023, 0x00000000, 0x00050048, 0x00000011, 0x00000001, 0x00000023, 0x00000008, 0x00050048,
	0x00000011, 0x00000002, 0x00000023, 0x00000010, 0x00050048, 0x00000013, 0x00000000, 0x00000023,
	0x00000000, 0x00050048, 0x00000013, 0x00000001, 0x00000023, 0x00000008, 0x00050048, 0x00000013,
	0x00000002, 0x00000023, 0x00000010, 0x00040047, 0x00000004, 0x0000000b, 0x00000000, 0x00040047,
	0x00000005, 0x0000001e, 0x00000000, 0x00040047, 0x00000006, 0x0000001e, 0x00000001, 0x00050048,
	0x00000014, 0x00000000, 0x00000023, 0x00000000, 0x00040047, 0x0000000c, 0x0000001e, 0x00000001,
	0x00040047, 0x00000016, 0x00000006, 0x00000008, 0x00040047, 0x0000000d, 0x00000021, 0x00000002,
	0x00040047, 0x0000000d, 0x00000022, 0x00000000, 0x00040047, 0x00000017, 0x00000006, 0x00000008,
	0x00040047, 0x00000009, 0x00000021, 0x00000000, 0x00040047, 0x00000009, 0x00000022, 0x00000000,
	0x00040047, 0x0000000b, 0x0000001e, 0x00000000, 0x00040047, 0x0000000a, 0x0000001e, 0x00000000,
	0x00030047, 0x00000018, 0x000014b4, 0x00030047, 0x00000019, 0x000014b4, 0x00020013, 0x0000001a,
	0x00030021, 0x0000001b, 0x0000001a, 0x00030016, 0x0000001c, 0x00000020, 0x00040017, 0x0000001d,
	0x0000001c, 0x00000004, 0x00040017, 0x0000001e, 0x0000001c, 0x00000002, 0x0005001e, 0x0000000e,
	0x0000001d, 0x0000001d, 0x0000001e, 0x00040020, 0x0000001f, 0x00000007, 0x0000000e, 0x00040015,
	0x00000020, 0x00000020, 0x00000001, 0x00040020, 0x00000021, 0x00000001, 0x00000020, 0x00040015,
	0x00000022, 0x00000020, 0x00000000, 0x00030027, 0x00000015, 0x000014e5, 0x0003001e, 0x0000000f,
	0x00000022, 0x0006001e, 0x00000010, 0x00000015, 0x0000001e, 0x0000001e, 0x0000000f, 0x00040020,
	0x00000023, 0x00000009, 0x00000010, 0x0004002b, 0x00000020, 0x00000024, 0x00000000, 0x00040020,
	0x00000025, 0x00000009, 0x00000015, 0x0005001e, 0x00000011, 0x0000001e, 0x0000001e, 0x00000022,
	0x0005001e, 0x00000013, 0x0000001e, 0x0000001e, 0x00000022, 0x00040021, 0x00000026, 0x00000013,
	0x00000011, 0x00040020, 0x00000027, 0x00000007, 0x0000001d, 0x0004002b, 0x00000020, 0x00000028,
	0x00000001, 0x00040020, 0x00000029, 0x00000009, 0x0000001e, 0x0004002b, 0x00000020, 0x0000002a,
	0x00000002, 0x0004002b, 0x0000001c, 0x0000002b, 0x00000000, 0x0004002b, 0x0000001c, 0x0000002c,
	0x3f800000, 0x00040020, 0x0000002d, 0x00000007, 0x0000001e, 0x00040020, 0x0000002e, 0x00000007,
	0x0000001c, 0x0004002b, 0x0000001c, 0x0000002f, 0xbf800000, 0x00040020, 0x00000030, 0x00000003,
	0x0000001d, 0x00040020, 0x00000031, 0x00000003, 0x0000001e, 0x0004002b, 0x00000020, 0x00000032,
	0x00000003, 0x00040020, 0x00000033, 0x00000009, 0x0000000f, 0x0003001e, 0x00000014, 0x00000022,
	0x00040021, 0x00000034, 0x00000014, 0x0000000f, 0x00040020, 0x00000035, 0x00000001, 0x0000001e,
	0x00050021, 0x00000036, 0x0000001d, 0x00000014, 0x0000001e, 0x0004002b, 0x00000022, 0x00000037,
	0x000fffff, 0x00090019, 0x00000038, 0x0000001c, 0x00000001, 0x00000002, 0x00000000, 0x00000000,
	0x00000001, 0x00000000, 0x0003001d, 0x00000016, 0x00000038, 0x00040020, 0x00000039, 0x00000000,
	0x00000016, 0x00040020, 0x0000003a, 0x00000000, 0x00000038, 0x0004002b, 0x00000020, 0x0000003b,
	0x00000014, 0x0002001a, 0x0000003c, 0x0003001d, 0x00000017, 0x0000003c, 0x00040020, 0x0000003d,
	0x00000000, 0x00000017, 0x00040020, 0x0000003e, 0x00000000, 0x0000003c, 0x0003001b, 0x0000003f,
	0x00000038, 0x00040020, 0x00000040, 0x00000001, 0x0000001d, 0x00040020, 0x00000015, 0x000014e5,
	0x00000011, 0x0004003b, 0x00000021, 0x00000007, 0x00000001, 0x0004003b, 0x00000023, 0x00000003,
	0x00000009, 0x0004003b, 0x00000030, 0x00000004, 0x00000003, 0x0004003b, 0x00000030, 0x00000005,
	0x00000003, 0x0004003b, 0x00000031, 0x00000006, 0x00000003, 0x0004003b, 0x00000035, 0x0000000c,
	0x00000001, 0x0004003b, 0x00000039, 0x0000000d, 0x00000000, 0x0004003b, 0x0000003d, 0x00000009,
	0x00000000, 0x0004003b, 0x00000040, 0x0000000b, 0x00000001, 0x0004003b, 0x00000030, 0x0000000a,
	0x00000003, 0x00040020, 0x00000041, 0x00000007, 0x00000013, 0x00040020, 0x00000042, 0x00000007,
	0x00000014, 0x00030001, 0x0000001e, 0x00000043, 0x00030001, 0x00000022, 0x00000044, 0x00050036,
	0x0000001a, 0x00000002, 0x00000000, 0x0000001b, 0x000200f8, 0x00000045, 0x0004003b, 0x0000002d,
	0x00000046, 0x00000007, 0x0004003b, 0x00000027, 0x00000047, 0x00000007, 0x0004003b, 0x00000027,
	0x00000048, 0x00000007, 0x0004003d, 0x00000020, 0x00000049, 0x00000007, 0x0004007c, 0x00000022,
	0x0000004a, 0x00000049, 0x00050041, 0x00000025, 0x0000004b, 0x00000003, 0x00000024, 0x0004003d,
	0x00000015, 0x0000004c, 0x0000004b, 0x00050043, 0x00000015, 0x0000004d, 0x0000004c, 0x0000004a,
	0x0006003d, 0x00000011, 0x00000012, 0x0000004d, 0x00000002, 0x00000004, 0x00050051, 0x0000001e,
	0x0000004e, 0x00000012, 0x00000000, 0x00050051, 0x0000001e, 0x0000004f, 0x00000012, 0x00000001,
	0x00050051, 0x00000022, 0x00000050, 0x00000012, 0x00000002, 0x00060050, 0x00000013, 0x00000051,
	0x0000004e, 0x0000004f, 0x00000050, 0x00050041, 0x00000029, 0x00000052, 0x00000003, 0x00000028,
	0x0004003d, 0x0000001e, 0x00000053, 0x00000052, 0x00050085, 0x0000001e, 0x00000054, 0x0000004e,
	0x00000053, 0x00050041, 0x00000029, 0x00000055, 0x00000003, 0x0000002a, 0x0004003d, 0x0000001e,
	0x00000056, 0x00000055, 0x00050081, 0x0000001e, 0x00000057, 0x00000054, 0x00000056, 0x00060050,
	0x0000001d, 0x00000058, 0x00000057, 0x0000002b, 0x0000002c, 0x0003003e, 0x00000048, 0x00000058,
	0x0006000c, 0x0000001d, 0x00000059, 0x00000001, 0x00000040, 0x00000050, 0x0003003e, 0x00000047,
	0x00000059, 0x0003003e, 0x00000046, 0x0000004f, 0x00050041, 0x0000002e, 0x0000005a, 0x00000048,
	0x00000028, 0x0004003d, 0x0000001c, 0x0000005b, 0x0000005a, 0x00050085, 0x0000001c, 0x0000005c,
	0x0000005b, 0x0000002f, 0x0003003e, 0x0000005a, 0x0000005c, 0x0004003d, 0x0000001d, 0x0000005d,
	0x00000048, 0x00060050, 0x0000000e, 0x0000005e, 0x0000005d, 0x00000059, 0x0000004f, 0x0003003e,
	0x00000004, 0x0000005d, 0x0003003e, 0x00000005, 0x00000059, 0x0003003e, 0x00000006, 0x0000004f,
	0x000100fd, 0x00010038, 0x00050036, 0x0000001a, 0x00000008, 0x00000000, 0x0000001b, 0x000200f8,
	0x0000005f, 0x0004003b, 0x00000027, 0x00000060, 0x00000007, 0x00050041, 0x00000033, 0x00000061,
	0x00000003, 0x00000032, 0x0004003d, 0x0000000f, 0x00000062, 0x00000061, 0x00050051, 0x00000022,
	0x00000063, 0x00000062, 0x00000000, 0x00040050, 0x00000014, 0x00000064, 0x00000063, 0x0004003d,
	0x0000001e, 0x00000065, 0x0000000c, 0x000500c7, 0x00000022, 0x00000066, 0x00000063, 0x00000037,
	0x00050041, 0x0000003a, 0x00000067, 0x0000000d, 0x00000066, 0x0004003d, 0x00000038, 0x00000018,
	0x00000067, 0x000500c2, 0x00000022, 0x00000068, 0x00000063, 0x0000003b, 0x00050041, 0x0000003e,
	0x00000069, 0x00000009, 0x00000068, 0x0004003d, 0x0000003c, 0x00000019, 0x00000069, 0x00050056,
	0x0000003f, 0x0000006a, 0x00000018, 0x00000019, 0x00060057, 0x0000001d, 0x0000006b, 0x0000006a,
	0x00000065, 0x00000000, 0x0003003e, 0x00000060, 0x0000006b, 0x0004003d, 0x0000001d, 0x0000006c,
	0x0000000b, 0x00050085, 0x0000001d, 0x0000006d, 0x0000006c, 0x0000006b, 0x0003003e, 0x0000000a,
	0x0000006d, 0x000100fd, 0x00010038
});

struct ImGui_ImplCobra_FrameRenderBuffers
{
	std::unique_ptr<Cobra::Buffer> VertexBuffer = nullptr;
	std::unique_ptr<Cobra::Buffer> IndexBuffer = nullptr;
};

struct ImGui_ImplCobra_WindowRenderBuffers
{
	uint32_t Index;
	uint32_t Count;
	std::vector<ImGui_ImplCobra_FrameRenderBuffers> FrameRenderBuffers;
};

struct ImGui_ImplCobraH_Window
{
	std::unique_ptr<Cobra::Swapchain> Swapchain;
	Cobra::SyncPoint SyncPoint[2];

	uint32_t FrameIndex = 0;
	bool ClearEnable;
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
	wd->ClearEnable = (viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? false : true;
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
	if (wd->ClearEnable)
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

	bd->Shader = std::make_unique<Cobra::Shader>(*v->Context, SHADER_SPIRV);
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

	bd->FontImage = std::make_unique<Cobra::Image>(*v->Context, Cobra::uVec2 { (uint32_t)width, (uint32_t)height }, Cobra::ImageFormat::R8G8B8A8_UNORM, Cobra::ImageUsage::Sampled | Cobra::ImageUsage::TransferDst);
	bd->FontImage->SetDebugName("Imgui font image");
	bd->FontImage->Set(pixels);
	bd->FontImage->Transition(Cobra::ImageLayout::ReadOnlyOptimal);
	
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

	if (wrb->FrameRenderBuffers.empty())
	{
		wrb->Index = 0;
		wrb->Count = 2;

		wrb->FrameRenderBuffers.resize(wrb->Count);
		memset(wrb->FrameRenderBuffers.data(), 0, sizeof(ImGui_ImplCobra_FrameRenderBuffers) * wrb->Count);
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
			cmd.PushConstant(ImGui::Slang::Push {
				.vertices = rb->VertexBuffer->GetDeviceAddress(),
				.scale = scale,
				.translate = translate,
				.image = { (uint32_t)(uint64_t)pcmd->TextureId, bd->FontSampler->GetHandle() }
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