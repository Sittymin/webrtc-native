#!python

import os, sys, platform, json, subprocess

import builders

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
                .replace("{ARCH}", env["arch"] if "arch" in env else "")
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

    opts.Add(("arch", "", ""))
    opts.Update(env)
else:
    ARGUMENTS["ios_min_version"] = "11.0"
    env = SConscript("godot-cpp/SConstruct").Clone()
opts.Update(env)

# Add method to generated gdnative lib
env.Append(BUILDERS={"GDNativeLibBuilder": Builder(action=gen_gdnative_lib)})

target = env["target"]
result_path = os.path.join("bin", "webrtc" if env["target"] == "release" else "webrtc_debug", "lib")

# Dependencies
deps_source_dir = "deps"
env.Append(BUILDERS={
    "BuildOpenSSL": env.Builder(action=builders.ssl_action, emitter=builders.ssl_emitter),
    "BuildLibDataChannel": env.Builder(action=builders.rtc_action, emitter=builders.rtc_emitter),
})

# SSL
ssl = env.BuildOpenSSL(env.Dir(builders.get_ssl_build_dir(env)), env.Dir(builders.get_ssl_source_dir(env)))

env.Prepend(CPPPATH=[builders.get_ssl_include_dir(env)])
env.Prepend(LIBPATH=[builders.get_ssl_build_dir(env)])
env.Append(LIBS=[ssl])

# RTC
rtc = env.BuildLibDataChannel(env.Dir(builders.get_rtc_build_dir(env)), [env.Dir(builders.get_rtc_source_dir(env))] + ssl)

env.Append(LIBPATH=[builders.get_rtc_build_dir(env)])
env.Append(CPPPATH=[builders.get_rtc_include_dir(env)])
env.Prepend(LIBS=[rtc])

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

env.Depends(sources, [ssl, rtc])

# Make the shared library
result_name = "webrtc_native.{}.{}.{}{}".format(env["platform"], env["target"], env["arch_suffix"], env["SHLIBSUFFIX"])
env.Depends(sources, ssl)
library = env.SharedLibrary(target=os.path.join(result_path, result_name), source=sources)
Default(library)

# GDNativeLibrary
gdnlib = "webrtc"
if target != "release":
    gdnlib += "_debug"
ext = ".tres" if env["godot_version"] == "3" else ".gdextension"
Default(env.GDNativeLibBuilder([os.path.join("bin", gdnlib, gdnlib + ext)], ["misc/webrtc" + ext]))
