workspace "TooSquared"
   configurations { "Debug", "Profile", "Release" }
   platforms { "x64" }

project "TooSquared"
   --symbolspath '$(OutDir)$(TargetName).pdb'
   kind "WindowedApp" --WindowedApp --ConsoleApp
   language "C++"
   cppdialect "C++latest"
   targetdir "Build/%{cfg.platform}/%{cfg.buildcfg}"
   objdir "Build/obj/%{cfg.platform}/%{cfg.buildcfg}"
   editandcontinue "Off"
   characterset "ASCII"
   links {
       "SDL2",
       "SDL2main",
       "OpenGL32",
   }

   libdirs {
       "Contrib/SDL/lib/%{cfg.platform}/",
       "Contrib/imgui",
       "Contrib/tracy-master",
       --"Contrib/**",
   }
   includedirs {
       "Contrib",
       --"Contrib/*",
       "Contrib/imgui",
       "Contrib/SDL/include",
       "Contrib/tracy-master",
       "Contrib/Glew/include",
       "Contrib/Glew/include/GL",
       --"Contrib/**"
   }
   flags {
       "MultiProcessorCompile",
       "FatalWarnings",
       "NoPCH",
   }
   defines {
       "_CRT_SECURE_NO_WARNINGS",
       "GLEW_STATIC",
       "CAMERA", --DO WE NEED THIS?
   }
   files {
       "Source/**",
       "Contrib/Glew/src/glew.c",
       "Contrib/tracy-master/TracyClient.cpp",
       "Contrib/imgui/*.cpp",
       "Contrib/imgui/*.h",
   }



    postbuildcommands
    {
        "{COPY} Contrib/SDL/lib/%{cfg.platform}/SDL2.dll %{cfg.targetdir}"
    }


   filter "configurations:Debug"
      defines { "_DEBUG", "TRACY_ENABLE"}
      symbols  "On"
      optimize "Off"

   filter "configurations:Profile"
      defines { "NDEBUG", "TRACY_ENABLE"}
      symbols  "off"
      optimize "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      symbols  "off"
      optimize "On"
