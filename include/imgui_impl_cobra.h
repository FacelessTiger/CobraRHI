#pragma once

#include "CobraRHI.h"

struct ImDrawData;
struct ImGuiViewport;

void* Platform_GetNativeWindow(ImGuiViewport* viewport);

struct ImGui_ImplCobra_InitInfo
{
	Cobra::GraphicsContext* Context;
	Cobra::Queue* Queue;
};

bool ImGui_ImplCobra_Init(ImGui_ImplCobra_InitInfo* info);
bool ImGui_ImplCobra_CreateFontsTexture();
void ImGui_ImplCobra_NewFrame();
void ImGui_ImplCobra_RenderDrawData(ImDrawData* drawData, Cobra::CommandList& cmd);
void ImGui_ImplCobra_Shutdown();