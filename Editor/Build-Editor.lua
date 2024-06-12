project "Editor"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files {
	 "Source/**.h",
	 "Source/**.cpp", 
	 "../Libraries/Source/glad/**.c" 
   }

   includedirs
   {
      "Source",
	 "../Engine/Source",
	 "../Libraries/Include/GLFW",
	 "../Libraries/Include/glad",
	 "../Libraries/Include/KHR"
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