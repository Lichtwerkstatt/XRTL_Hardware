Import("env")

from os.path import join
#shamelessly stolen from builder/main.py
#probably overwriting some properties, but only after the build is done
#hopefully won't cause problems down the line
def merge_binaries(source, target, env):
    platform = env.PioPlatform()
    board = env.BoardConfig()
    mcu = board.get("build.mcu", "esp32")
    env.Replace(
        UPLOADER=join(
            platform.get_package_dir("tool-esptoolpy") or "", "esptool.py"),
        UPLOADERFLAGS=[
            "--chip", mcu,
            "merge_bin", "-o", join("merged_binaries","${BOARD}.bin"),
            "--flash_mode", "${__get_board_flash_mode(__env__)}",
            "--flash_size", board.get("upload.flash_size", "detect")
        ],
        UPLOADCMD=[
            '"$PYTHONEXE" "$UPLOADER" $UPLOADERFLAGS $ESP32_APP_OFFSET',
            join("$BUILD_DIR", "firmware.bin")
        ]
    )

    for image in env.get("FLASH_EXTRA_IMAGES", []):
        env.Append(UPLOADERFLAGS=[image[0], env.subst(image[1])])

    #print("upload command: ", env.subst("$UPLOADCMD"))
    env.Execute("$UPLOADCMD")

env.AddPostAction("buildprog", merge_binaries)
env.AddPostAction("upload", merge_binaries)