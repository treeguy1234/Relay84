#!/usr/bin/env python3
import os
import sys
import shutil
import subprocess

try:
    from serial.tools import list_ports
except Exception:
    list_ports = None

from SCons.Script import DefaultEnvironment, COMMAND_LINE_TARGETS

env = DefaultEnvironment()

# This environment performs its own per-port uploads. Make the built-in
# `upload` action a no-op to avoid an extra duplicate upload at the end.
if env.get("PIOENV") == "upload_all":
    env.Replace(UPLOADCMD='echo "[upload_all] Skipping default upload action"')

def before_upload(source, target, env):
    # only run when user explicitly invoked upload and when this env is used
    if "upload" not in COMMAND_LINE_TARGETS:
        return
    if env.get("PIOENV") != "upload_all":
        return

    # enumerate available serial ports
    ports = []
    if list_ports:
        ports = [p.device for p in list_ports.comports()]

    if not ports:
        print("[upload_all] No serial ports found. Nothing to upload.")
        return

    # find platformio/pio executable
    pio_cmd = None
    for cmd in ("pio", "platformio", "platformio.exe", "pio.exe"):
        if shutil.which(cmd):
            pio_cmd = cmd
            break

    # fallback to python -m platformio
    use_module = False
    if not pio_cmd:
        pio_cmd = [sys.executable, "-m", "platformio"]
        use_module = True

    # run uploads sequentially, using the `base` env to perform the actual upload
    for port in ports:
        if port != "COM3" and port != "COM4":
            print(f"[upload_all] Uploading to {port} ...")
            if use_module:
                cmd = pio_cmd + ["run", "-e", "base", "-t", "upload", "--upload-port", port]
            else:
                cmd = [pio_cmd, "run", "-e", "base", "-t", "upload", "--upload-port", port]

            try:
                subprocess.check_call(cmd)
            except subprocess.CalledProcessError as e:
                print(f"[upload_all] Upload to {port} failed: {e}")
        


env.AddPreAction("upload", before_upload)
