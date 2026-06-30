import sys
import os
import subprocess
import argparse
import glob

def main():
    parser = argparse.ArgumentParser(description="Doodle UI Engine Command Line Utility")
    subparsers = parser.add_subparsers(dest="command", required=True)
    
    # Subparser for the 'package' command
    pack_parser = subparsers.add_parser("package", help="Package a Doodle application into a standalone executable")
    pack_parser.add_argument("entrypoint", help="Path to the main Python script (e.g. main.py)")
    pack_parser.add_argument("-n", "--name", help="Name of the generated executable file")
    pack_parser.add_argument("--console", action="store_true", help="Keep the console window visible on launch")
    
    args = parser.parse_args()
    
    if args.command == "package":
        entrypoint = args.entrypoint
        if not os.path.exists(entrypoint):
            print(f"Error: Entrypoint file '{entrypoint}' not found.")
            sys.exit(1)
            
        print(f"=== Doodle Standalone Packager ===")
        print(f"Packing '{entrypoint}' into a standalone executable...")
        
        # Check if PyInstaller is installed
        try:
            import PyInstaller
        except ImportError:
            print("PyInstaller is not installed. Installing it via pip...")
            subprocess.run([sys.executable, "-m", "pip", "install", "pyinstaller"], check=True)
            
        # Build command line arguments
        # On Windows, path separator is ;. On Unix it is :.
        sep = ";" if os.name == "nt" else ":"
        
        import doodle
        doodle_dir = os.path.dirname(os.path.abspath(doodle.__file__))
        
        # Locate the compiled binary extension module (_doodle.pyd or _doodle.so)
        doodle_binary = None
        for pattern in ["_doodle*.pyd", "_doodle*.so", "_doodle*.dll"]:
            matches = glob.glob(os.path.join(doodle_dir, pattern))
            if matches:
                doodle_binary = matches[0]
                break
        if not doodle_binary:
            # Fallback to default
            doodle_binary = os.path.join(doodle_dir, "_doodle.pyd")
            
        entrypoint_dir = os.path.dirname(os.path.abspath(entrypoint))
        
        layout_path = os.path.join(entrypoint_dir, "layout.html")
        styles_path = os.path.join(entrypoint_dir, "styles.css")
        shaders_path = os.path.join(entrypoint_dir, "shaders")
        assets_path = os.path.join(entrypoint_dir, "assets")
        
        pyinstaller_args = [
            sys.executable,
            "-m",
            "PyInstaller",
            "--onefile",
        ]
        
        if not args.console:
            pyinstaller_args.append("--noconsole")
            
        if args.name:
            pyinstaller_args.extend(["--name", args.name])
            
        # Add the doodle compiled binary
        pyinstaller_args.append(f"--add-binary={doodle_binary}{sep}doodle")
        
        # Add layout and styles if they exist
        if os.path.exists(layout_path):
            pyinstaller_args.append(f"--add-data={layout_path}{sep}.")
        if os.path.exists(styles_path):
            pyinstaller_args.append(f"--add-data={styles_path}{sep}.")
            
        # Add folders if they exist
        if os.path.exists(shaders_path):
            pyinstaller_args.append(f"--add-data={shaders_path}{sep}shaders")
        if os.path.exists(assets_path):
            pyinstaller_args.append(f"--add-data={assets_path}{sep}assets")
            
        # Link against raylib dll if found locally
        if os.path.exists("raylib.dll"):
            pyinstaller_args.append(f"--add-binary=raylib.dll{sep}.")
        elif os.path.exists("src/raylib.dll"):
            pyinstaller_args.append(f"--add-binary=src/raylib.dll{sep}.")
            
        pyinstaller_args.append(entrypoint)
        
        print(f"Running: {' '.join(pyinstaller_args)}")
        subprocess.run(pyinstaller_args)
        print(f"Packager completed. Check the 'dist/' folder for your executable.")

if __name__ == "__main__":
    main()
