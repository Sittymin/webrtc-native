#!python

from builders import build_deps

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
        env.Append(CCFLAGS=["/std:c++17", "/DDEBUG_ENABLED"])
        if env["target"] == "debug":
            env.Append(CCFLAGS=["/DDEBUG_ENABLED"])
    else:
        env.Append(CCFLAGS=["-std=c++17"])
        if env["target"] == "debug":
            env.Append(CCFLAGS=["-DDEBUG_ENABLED"])
else:
    env = SConscript("godot-cpp/SConstruct").Clone()
opts.Update(env)

# TODO Should upstream this as an option.
if env["platform"] == "ios":
    if env["ios_simulator"]:
        env.Append(CCFLAGS=["-mios-simulator-version-min=11.0"])
    else:
        env.Append(CCFLAGS=["-miphoneos-version-min=11.0"])
elif env["platform"] == "windows" and env["use_mingw"]:
    env["SHLIBSUFFIX"] = ".dll"

# Add method that joins/compiles our Engine files.
env.AddMethod(build_deps, "BuildDeps")
# Add method to generated gdnative lib
env.Append(BUILDERS={"GDNativeLibBuilder": Builder(action=gen_gdnative_lib)})

target = env["target"]
result_path = os.path.join("bin", "webrtc" if env["target"] == "release" else "webrtc_debug", "lib")

# Dependencies
deps_source_dir = "deps"

# Build libdatachannel dependencies
deps = env.BuildDeps("build-deps", deps_source_dir)

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
env.Depends(sources, deps)
library = env.SharedLibrary(target=os.path.join(result_path, result_name), source=sources)
Default(library)

# GDNativeLibrary
gdnlib = "webrtc"
if target != "release":
    gdnlib += "_debug"
Default(env.GDNativeLibBuilder([os.path.join("bin", gdnlib, gdnlib + ".tres")], ["misc/gdnlib.tres"]))
