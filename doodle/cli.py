import sys
import os
import subprocess

def main():
    if len(sys.argv) < 2:
        print("Usage: python -m doodle.cli package <entrypoint.py>")
        sys.exit(1)
        
    cmd = sys.argv[1]
    if cmd != "package":
        print(f"Unknown command: {cmd}. Available commands: package")
        sys.exit(1)
        
    if len(sys.argv) < 3:
        print("Please specify the entrypoint script (e.g. main.py)")
        sys.exit(1)
        
    entrypoint = sys.argv[2]
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
    
    pyinstaller_args = [
        "pyinstaller",
        "--onefile",
        f"--add-binary=_doodle.pyd{sep}.",
        f"--add-data=layout.html{sep}.",
        f"--add-data=styles.css{sep}.",
    ]
    
    # Add folders if they exist
    if os.path.exists("shaders"):
        pyinstaller_args.append(f"--add-data=shaders{sep}shaders")
    if os.path.exists("assets"):
        pyinstaller_args.append(f"--add-data=assets{sep}assets")
        
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
