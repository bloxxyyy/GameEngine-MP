project "Engine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files {
	 "Source/**.h",
	 "Source/**.cpp",
	 "Source/**.vs",
	 "Source/**.fs",
	 "Source/Engine/Models/**.**",
 	 "Source/Engine/Images/**.**",
	 "Source/Engine/Headers/**.**"
   }

   includedirs
   {
      "Source",
	 "Source/Engine/Headers",
	 "Source/Engine/Images",
	 "Source/Engine/Models",
	 "../Libraries/Include/glm",
      "../Libraries/Include/GLFW",
	 "../Libraries/Include/glad",
	 "../Libraries/Include/KHR",
   	 "../Libraries/Include/stb_image"
   }

   libdirs {
      "../Libraries/Lib/GLFW"
   }

   links
   {
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