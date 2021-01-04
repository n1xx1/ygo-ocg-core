{
    "targets": [
        {
            "target_name": "native",
            "sources": [
                "cpp/native.cc",
                "cpp/buffer.cc",
                "cpp/lua/lapi.c",
                "cpp/lua/lauxlib.c",
                "cpp/lua/lbaselib.c",
                "cpp/lua/lcode.c",
                "cpp/lua/lcorolib.c",
                "cpp/lua/lctype.c",
                "cpp/lua/ldblib.c",
                "cpp/lua/ldebug.c",
                "cpp/lua/ldo.c",
                "cpp/lua/ldump.c",
                "cpp/lua/lfunc.c",
                "cpp/lua/lgc.c",
                "cpp/lua/linit.c",
                "cpp/lua/liolib.c",
                "cpp/lua/llex.c",
                "cpp/lua/lmathlib.c",
                "cpp/lua/lmem.c",
                "cpp/lua/loadlib.c",
                "cpp/lua/lobject.c",
                "cpp/lua/lopcodes.c",
                "cpp/lua/loslib.c",
                "cpp/lua/lparser.c",
                "cpp/lua/lstate.c",
                "cpp/lua/lstring.c",
                "cpp/lua/lstrlib.c",
                "cpp/lua/ltable.c",
                "cpp/lua/ltablib.c",
                "cpp/lua/ltm.c",
                "cpp/lua/lundump.c",
                "cpp/lua/lutf8lib.c",
                "cpp/lua/lvm.c",
                "cpp/lua/lzio.c",
                "cpp/ygo/card.cpp",
                "cpp/ygo/duel.cpp",
                "cpp/ygo/effect.cpp",
                "cpp/ygo/field.cpp",
                "cpp/ygo/group.cpp",
                "cpp/ygo/interpreter.cpp",
                "cpp/ygo/libcard.cpp",
                "cpp/ygo/libdebug.cpp",
                "cpp/ygo/libduel.cpp",
                "cpp/ygo/libeffect.cpp",
                "cpp/ygo/libgroup.cpp",
                "cpp/ygo/ocgapi.cpp",
                "cpp/ygo/operations.cpp",
                "cpp/ygo/playerop.cpp",
                "cpp/ygo/processor.cpp",
                "cpp/ygo/scriptlib.cpp"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                "cpp/lua"
            ],
            "dependencies": [
                "<!(node -p \"require('node-addon-api').gyp\")"
            ],
            'cflags': [
                '/TP',
            ],
            'conditions': [
                ['OS=="win"', {
                    'cflags': [
                        '/TP',
                    ],
                }],
            ],
            "xcode_settings": {
                "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
                "CLANG_CXX_LIBRARY": "libc++",
                "MACOSX_DEPLOYMENT_TARGET": "10.7"
            },
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "ExceptionHandling": 1,
                    "AdditionalOptions": ["-TP"]
                }
            }
        }
    ]
}
