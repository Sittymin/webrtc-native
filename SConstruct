#!python

import os, sys, platform, json, subprocess

if sys.version_info < (3,):

    def decode_utf8(x):
        return x


else:
    import codecs

    def decode_utf8(x):
        return codecs.utf_8_decode(x)[0]


def add_sources(sources, dirpath, extension):
    for f in os.listdir(dirpath):
        if f.endswith("." + extension):
            sources.append(dirpath + "/" + f)


def gen_gdnative_lib(target, source, env):
    for t in target:
        with open(t.srcnode().path, "w") as w:
            w.write(
                decode_utf8(source[0].get_contents())
                .replace("{GDNATIVE_PATH}", os.path.splitext(t.name)[0])
                .replace("{TARGET}", env["target"])
            )


env = Environment()
opts = Variables(["customs.py"], ARGUMENTS)
opts.Add(EnumVariable("godot_version", "The Godot target version", "4", ["3", "4"]))
opts.Update(env)

if env["godot_version"] == "3":
    env = SConscript("godot-cpp-3.x/SConstruct").Clone()
    # Require C++17
    if sys.platform == "win32" or sys.platform == "msys" and env["platform"] == "windows" and not env["use_mingw"]:
        # MSVC
        env.Append(CCFLAGS=["/std:c++17"])
    else:
        env.Append(CCFLAGS=["-std=c++17"])
else:
    env = SConscript("godot-cpp/SConstruct").Clone()
opts.Update(env)

env.Append(BUILDERS={"GDNativeLibBuilder": Builder(action=gen_gdnative_lib)})

target = env["target"]
result_path = os.path.join("bin", "webrtc" if env["target"] == "release" else "webrtc_debug", "lib")

# Convenience check to enforce the use_llvm overrides when CXX is clang(++)
if "CXX" in env and "clang" in os.path.basename(env["CXX"]):
    env["use_llvm"] = True

# WebRTC stuff
rtc_dir = "libdatachannel"
rtc_includes = [rtc_dir + "/include"]
libs = ['ssl', 'usrsctp', 'libjuice-static', 'datachannel-static']
libs.reverse()
lib_path = os.path.join(rtc_dir, env["platform"])

target_platform = env["platform"]
target_arch = env["bits"]
if target_platform == "android":
    target_arch = env["android_arch"]
elif target_platform == "ios":
    target_arch = env["ios_arch"]
elif target_platform == "osx":
    if env["macos_arch"] != "universal":
        target_arch = env["macos_arch"]

lib_path += {
    "32": "/x86",
    "64": "/x64",
    "armv7": "/arm",
    "arm64v8": "/arm64",
    "arm64": "/arm64",
    "x86": "/x86",
    "x86_64": "/x64",
}[target_arch]

if target == "debug":
    lib_path += "/Debug"
else:
    lib_path += "/Release"

env.Append(CPPPATH=rtc_includes)

#if target_platform == "linux":
#    env.Append(LIBS=["atomic"])
#    env.Append(LIBPATH=[lib_path])
#    env.Append(CCFLAGS=["-DWEBRTC_POSIX", "-DWEBRTC_LINUX"])
#    env.Append(CCFLAGS=["-DRTC_UNUSED=''", "-DNO_RETURN=''"])
#
#elif target_platform == "windows":
#    # Mostly VisualStudio
#    if env["CC"] == "cl":
#        env.Append(CCFLAGS=["/DWEBRTC_WIN", "/DWIN32_LEAN_AND_MEAN", "/DNOMINMAX", "/DRTC_UNUSED=", "/DNO_RETURN="])
#        env.Append(LINKFLAGS=[p + env["LIBSUFFIX"] for p in ["secur32", "advapi32", "winmm"] + libs])
#    # Mostly "gcc"
#    else:
#        env.Append(
#            CCFLAGS=[
#                "-DWINVER=0x0603",
#                "-D_WIN32_WINNT=0x0603",
#                "-DWEBRTC_WIN",
#                "-DWIN32_LEAN_AND_MEAN",
#                "-DNOMINMAX",
#                "-DRTC_UNUSED=",
#                "-DNO_RETURN=",
#            ]
#        )
#        env.Append(LINKFLAGS=[p + env["LIBSUFFIX"] for p in ["secur32", "advapi32", "winmm"] + libs])
#
#elif target_platform == "osx":
#    env.Append(CCFLAGS=["-DWEBRTC_POSIX", "-DWEBRTC_MAC"])
#    env.Append(CCFLAGS=["-DRTC_UNUSED=''", "-DNO_RETURN=''"])
#
#elif target_platform == "ios":
#    env.Append(CCFLAGS=["-DWEBRTC_POSIX", "-DWEBRTC_MAC", "-DWEBRTC_IOS"])
#    env.Append(CCFLAGS=["-DRTC_UNUSED=''", "-DNO_RETURN=''"])
#
#elif target_platform == "android":
#    env.Append(LIBS=["log"])
#    env.Append(CCFLAGS=["-DWEBRTC_POSIX", "-DWEBRTC_LINUX", "-DWEBRTC_ANDROID"])
#    env.Append(CCFLAGS=["-DRTC_UNUSED=''", "-DNO_RETURN=''"])
#
#    if target_arch == "arm64v8":
#        env.Append(CCFLAGS=["-DWEBRTC_ARCH_ARM64", "-DWEBRTC_HAS_NEON"])
#    elif target_arch == "armv7":
#        env.Append(CCFLAGS=["-DWEBRTC_ARCH_ARM", "-DWEBRTC_ARCH_ARM_V7", "-DWEBRTC_HAS_NEON"])


env.Append(LIBPATH=[lib_path])
if target_platform != "windows":
    env.Append(LIBS=libs)

# Our includes and sources
env.Append(CPPPATH=["src/"])
sources = []
sources.append(
    [
        "src/WebRTCLibDataChannel.cpp",
        "src/WebRTCLibPeerConnection.cpp",
    ]
)
if env["godot_version"] == "4":
    sources.append("src/init_gdextension.cpp")
else:
    env.Append(CPPDEFINES=["GDNATIVE_WEBRTC"])
    sources.append("src/init_gdnative.cpp")
    add_sources(sources, "src/net/", "cpp")

# Make the shared library
result_name = "webrtc_native.{}.{}.{}{}".format(env["platform"], env["target"], env["arch_suffix"], env["SHLIBSUFFIX"])
library = env.SharedLibrary(target=os.path.join(result_path, result_name), source=sources)
Default(library)

# GDNativeLibrary
gdnlib = "webrtc"
if target != "release":
    gdnlib += "_debug"
Default(env.GDNativeLibBuilder([os.path.join("bin", gdnlib, gdnlib + ".tres")], ["misc/gdnlib.tres"]))
