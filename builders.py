import os


def get_android_api(env):
    return env["android_api_level"] if int(env["android_api_level"]) > 28 else "28"


def build_deps(env, target, sources):
    source_dir = sources
    build_dir = os.path.join(sources, "build", "{}.{}.{}.dir".format(env["platform"], env["target"], env["arch_suffix"]))

    ssl_source_dir = env.Dir(os.path.join(source_dir, "openssl")).abspath
    ssl = build_ssl(env, ssl_source_dir, ssl_source_dir)

    rtc_source_dir = os.path.join(source_dir, "libdatachannel")
    rtc_build_dir = os.path.join(build_dir, "libdatachannel")
    rtc = build_rtc(env, rtc_build_dir, rtc_source_dir, ssl_source_dir)
    env.Depends(rtc, ssl)

    return ssl + rtc


def build_ssl(env, build_dir, source_dir):
    cfg_cmd = "cd %s && ./Configure " % source_dir
    args = [
        "no-ssl2",
        "no-ssl3",
        "no-shared",
    ]
    if env["platform"] == "android":
        args.extend([
            {
                "arm64v8": "android-arm64",
                "armv7": "android-arm",
                "x86": "android-x86",
                "x86_64": "android-x86_64",
            }[env["android_arch"]],
            "-D__ANDROID_API__=%s" % get_android_api(env),
        ])
        env["ENV"]["ANDROID_NDK_ROOT"] = env["ANDROID_NDK_ROOT"]
    configure = env.Command(os.path.join(build_dir, "Makefile"), "", cfg_cmd + " ".join(args))
    libs = [":libcrypto.a.a", ":libssl.a.a"] # FIXME
    env.Prepend(CPPPATH=[os.path.join(source_dir, "include")])
    env.Prepend(LIBPATH=[build_dir])
    env.Append(LIBS=libs)
    make = env.Command(libs, configure, "make -C %s -j%s" % (build_dir, env.GetOption("num_jobs")))
    return make


def build_rtc(env, build_dir, source_dir, ssl_root):
    rtc_includes = os.path.join(source_dir, "include")
    env.Append(CPPPATH=rtc_includes)

    args = [
        "-B",
        build_dir,
        "-DUSE_NICE=0",
        "-DNO_WEBSOCKET=1",
        "-DNO_MEDIA=1",
        "-DNO_EXAMPLES=1",
        "-DNO_WEBSOCKET=1",
        "-DNO_TESTS=1",
        "-DOPENSSL_USE_STATIC_LIBS=1",
        "-DOPENSSL_ROOT_DIR=%s" % ssl_root,
    ]
    if env["platform"] == "android":
        abi = {
            "arm64v8": "arm64-v8a",
            "armv7": "armeabi-v7a",
            "x86": "x86",
            "x86_64": "x86_64",
        }[env["android_arch"]]
        args.extend([
            "-DCMAKE_SYSTEM_NAME=Android",
            "-DCMAKE_SYSTEM_VERSION=%s" % get_android_api(env),
            "-DCMAKE_ANDROID_ARCH_ABI=%s" % abi,
            "-DCMAKE_ANDROID_NDK='%s'" % env["ANDROID_NDK_ROOT"],
            "-DCMAKE_ANDROID_STL_TYPE=c++_static",
        ])
    args.append(source_dir)
    libs = ["datachannel-static", "libjuice-static", "usrsctp"]
    lib_paths = [
        build_dir,
        os.path.join(build_dir, "deps/libjuice"),
        os.path.join(build_dir, "deps/usrsctp/usrsctplib"),
    ]
    env.Append(LIBPATH=lib_paths)
    env.Prepend(LIBS=libs)
    cmake = env.Command(os.path.join(build_dir, "Makefile"), "", "cmake " + " ".join(args))
    env.AlwaysBuild(cmake)
    make = env.Command(libs, cmake, "make -C %s datachannel-static" % build_dir)
    return make
