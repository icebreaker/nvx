solution "nvx"
  if _ACTION == "clean" then
    os.rmdir("build")
  end

  location "build"
  objdir "build"
  targetdir "build"
  language "C"

  configurations { "debug", "release" }

  configuration "debug"
    defines { "NVX_DEBUG" }

  configuration "release"
    defines { "NVX_NDEBUG" }

  configuration { "release", "gmake" }
    buildoptions { "-O3" }

  configuration { "gmake" }
    buildoptions { "-fdiagnostics-show-option", "-Wall", "-Wextra", "-Werror", "-ansi", "-pedantic" }
    defines { "_GNU_SOURCE" }

  project "nvx"
    kind "ConsoleApp"
    files { "src/*.c" }
