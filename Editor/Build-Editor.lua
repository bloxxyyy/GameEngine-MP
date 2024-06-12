project "Editor"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files {
	 "Source/**.h",
	 "Source/**.cpp", 
	 "../Libraries/Source/glad/**.c", 
    	 "../Libraries/Include/imgui/*.cpp",
 	 "../Libraries/Include/imgui/*.h",
 	 "../Libraries/Include/stb_image.h",  
 	 "../Libraries/Source/stb_image/stb_image.cpp",  	 "../Libraries/Include/imgui/backends/imgui_impl_opengl3.cpp", 
"../Libraries/Include/imgui/backends/imgui_impl_glfw.cpp",
"../Libraries/Include/imgui/backends/imgui_impl_opengl3.h", 
"../Libraries/Include/imgui/backends/imgui_impl_glfw.h"

   }

   includedirs
   {
      "Source",
	 "../Engine/Source",
	 "../Libraries/Include/GLFW",
	 "../Libraries/Include/glad",
	 "../Libraries/Include/KHR",
	 "../Libraries/Include/imgui",
   	 "../Libraries/Include/glm",
   	 "../Libraries/Include/stb_image"
   }

   libdirs {
      "../Libraries/Lib/GLFW"
   }

   links
   {
      "Engine",
      "glfw3"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter { "system:not windows" }
       links { "GL" }

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }
       links { "OpenGL32" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"