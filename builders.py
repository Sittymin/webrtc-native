import os


def get_android_api(env):
    return env["android_api_level"] if int(env["android_api_level"]) > 28 else "28"


def get_deps_dir(env):
    return env.Dir("deps").abspath

def get_deps_build_dir(env):
    return get_deps_dir(env) + "/build/{}.{}.{}.dir".format(env["platform"], env["target"], env["arch_suffix"])


def get_ssl_source_dir(env):
    return get_deps_dir(env) + "/openssl"


def get_ssl_build_dir(env):
    return get_deps_build_dir(env) + "/openssl"


def get_ssl_install_dir(env):
    return get_ssl_build_dir(env) + "/dest"


def get_ssl_include_dir(env):
    return get_ssl_install_dir(env) + "/include"


def build_deps(env, target, sources):
    source_dir = sources
    build_dir = get_deps_build_dir(env)

    ssl_source_dir = get_ssl_source_dir(env)
    ssl_build_dir = get_ssl_build_dir(env)
    ssl = build_ssl(env, ssl_build_dir, ssl_source_dir)

    rtc_source_dir = os.path.join(source_dir, "libdatachannel")
    rtc_build_dir = os.path.join(build_dir, "libdatachannel")
    env.Depends(rtc_build_dir, ssl)
    rtc = build_rtc(env, rtc_build_dir, rtc_source_dir)
    env.Depends(rtc, ssl)

    return ssl + rtc


def build_ssl(env, build_dir, source_dir):
    cfg_cmd = "mkdir -p %s && cd %s && %s/Configure " % (build_dir, build_dir, source_dir)
    ssl_env = env.Clone()
    install_dir = get_ssl_install_dir(env)
    args = [
        "no-ssl3",
        "no-weak-ssl-ciphers",
        "no-legacy",
        "--prefix=%s" % install_dir,
        "--openssldir=%s" % install_dir,
    ]
    if env["platform"] != "windows":
        args.append("no-shared")  # Windows "app" doesn't like static-only builds.
    if env["platform"] == "linux":
        if env["bits"] == "32":
            args.extend(["linux-x86"])
        else:
            args.extend(["linux-x86_64"])
    elif env["platform"] == "android":
        args.extend([
            {
                "arm64v8": "android-arm64",
                "armv7": "android-arm",
                "x86": "android-x86",
                "x86_64": "android-x86_64",
            }[env["android_arch"]],
            "-D__ANDROID_API__=%s" % get_android_api(env),
        ])
        ssl_env["ENV"]["ANDROID_NDK_ROOT"] = env["ANDROID_NDK_ROOT"]
    elif env["platform"] == "osx":
        if env["macos_arch"] == "x86_64":
            args.extend(["darwin64-x86_64"])
        elif env["macos_arch"] == "arm64":
            args.extend(["darwin64-arm64"])
        else:
            raise ValueError("OSX architecture not supported: %s" % env["macos_arch"])
    elif env["platform"] == "windows":
        if env["bits"] == "32":
            if env["use_mingw"]:
                args.extend([
                    "mingw",
                    "--cross-compile-prefix=i686-w64-mingw32-",
                ])
            else:
                args.extend(["VC-WIN32"])
        else:
            if env["use_mingw"]:
                args.extend([
                    "mingw64",
                    "--cross-compile-prefix=x86_64-w64-mingw32-",
                ])
            else:
                args.extend(["VC-WIN64A"])
    configure = ssl_env.Command(get_ssl_build_dir(env) + "/Makefile", "", cfg_cmd + " ".join(args))
    libs = ["libssl.a", "libcrypto.a"]
    ssl_include = os.path.join(source_dir, "include")
    env.Prepend(CPPPATH=[get_ssl_include_dir(env)])
    env.Prepend(LIBPATH=[get_ssl_build_dir(env)])
    env.Append(LIBS=libs)
    jobs = env.GetOption("num_jobs")
    make = ssl_env.Command(libs, configure,
            "make -C %s -j%s && make -C %s install_sw install_ssldirs -j%s && cp -r %s/include/* %s/include/" % (
                build_dir, jobs, build_dir, jobs, install_dir, build_dir
        ))
    return [configure, make]


def build_rtc(env, build_dir, source_dir):
    rtc_includes = os.path.join(source_dir, "include")
    env.Append(CPPPATH=rtc_includes)

    args = [
        "-B",
        build_dir,
        "-DUSE_NICE=0",
        "-DNO_WEBSOCKET=1",
        #"-DNO_MEDIA=1", # Windows builds fail without it.
        "-DNO_EXAMPLES=1",
        "-DNO_WEBSOCKET=1",
        "-DNO_TESTS=1",
        "-DOPENSSL_USE_STATIC_LIBS=1",
        "-DOPENSSL_INCLUDE_DIR=%s" % get_ssl_include_dir(env),
        "-DOPENSSL_SSL_LIBRARY=%s/libssl.a" % get_ssl_build_dir(env),
        "-DOPENSSL_CRYPTO_LIBRARY=%s/libcrypto.a" % get_ssl_build_dir(env),
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
    elif env["platform"] == "linux":
        if env["bits"] == "32":
            args.extend([
                "-DCMAKE_C_FLAGS=-m32",
                "-DCMAKE_CXX_FLAGS=-m32"
            ])
        else:
            args.extend([
                "-DCMAKE_C_FLAGS=-m64",
                "-DCMAKE_CXX_FLAGS=-m64"
            ])
    elif env["platform"] == "osx":
        if env["macos_arch"] == "x86_64":
            args.extend(["-DCMAKE_OSX_ARCHITECTURES=x86_64"])
        elif env["macos_arch"] == "arm64":
            args.extend(["-DCMAKE_OSX_ARCHITECTURES=arm64"])
        else:
            raise ValueError("OSX architecture not supported: %s" % env["macos_arch"])
    elif env["platform"] == "windows":
        args.extend(["-DOPENSSL_ROOT_DIR=%s" % get_ssl_build_dir(env)])
        if env["use_mingw"]:
            env.Append(LIBS=["iphlpapi", "ws2_32", "bcrypt"])
        if env["bits"] == "32":
            if env["use_mingw"]:
                args.extend([
                    "-G 'Unix Makefiles'",
                    "-DCMAKE_C_COMPILER=i686-w64-mingw32-gcc",
                    "-DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++",
                    "-DCMAKE_SYSTEM_NAME=Windows"
                ])
        else:
            if env["use_mingw"]:
                args.extend([
                    "-G 'Unix Makefiles'",
                    "-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc",
                    "-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++",
                    "-DCMAKE_SYSTEM_NAME=Windows"
                ])

    args.append(source_dir)
    libs = ["datachannel-static", "libjuice-static", "libsrtp2", "usrsctp"]
    lib_paths = [
        build_dir,
        os.path.join(build_dir, "deps/libjuice"),
        os.path.join(build_dir, "deps/libsrtp"),
        os.path.join(build_dir, "deps/usrsctp/usrsctplib"),
    ]
    env.Append(LIBPATH=lib_paths)
    env.Prepend(LIBS=libs)
    cmake = env.Command(build_dir, "", "cmake " + " ".join(args))
    jobs = env.GetOption("num_jobs")
    make = env.Command(libs, cmake, "make -C %s datachannel-static -j%s" % (build_dir, jobs))
    return [cmake, make]
