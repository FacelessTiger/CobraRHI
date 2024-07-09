VULKAN_SDK = os.getenv("VULKAN_SDK")

project "CobraRHI"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/Vulkan/InternalManagers/**.h",
		"src/Vulkan/InternalManagers/**.cpp",
		"src/Vulkan/Mappings/**.h",
		"src/Vulkan/Mappings/**.cpp",
		"vendor/xxHash/xxHash.c"
	}

	includedirs
	{
		"include",
		"%{VULKAN_SDK}/Include",
		"vendor/volk",
		"vendor/vma/include",
		"vendor/slang/include",
		"vendor/xxHash",
		"vendor/imgui"
	}

	links
	{
		"vendor/slang/bin/slang.lib",
		"imgui"
	}

	defines
	{
		"VK_USE_PLATFORM_WIN32_KHR",
		"VK_NO_PROTOTYPES",
		"WIN32_LEAN_AND_MEAN",
		"NOMINMAX"
	}

	filter "system:windows"
		files
		{
			"src/Vulkan/Platform/Windows/**.h",
			"src/Vulkan/Platform/Windows/**.cpp"
		}

	filter "configurations:Debug"
		defines "CB_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "CB_RELEASE"

	filter "configurations:Dist"
		defines "CB_DIST"

	filter "configurations:Release or Dist"
		runtime "Release"
		optimize "on"