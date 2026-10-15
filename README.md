
<p align="center">
  <img src="https://github.com/a8kj7sea/Flatty/blob/main/assets/banner.png" alt="Flatty Banner">
</p>

# Flatty

Flatty is a small, lightweight CLI utility written in C that collects a project's text files into one readable output file. It is designed to easily share project context with AI assistants (like ChatGPT or Claude) by generating a clean directory tree and concatenating file contents.

## Features
- **Fast & Lightweight:** Built in C using POSIX APIs (`nftw`, `fnmatch`).
- **Directory Tree:** Generates a clean visual representation of included files.
- **File Concatenation:** Appends file contents with clear relative-path headers.
- **Binary Detection:** Heuristically skips binary files (searches for NUL bytes in the first 8KB).
- **UTF-8 Safe:** Preserves Arabic text, emojis, and original file bytes.
- **Atomic Writes:** Uses `mkstemp` and `rename` to ensure previous outputs are not destroyed on failure.
- **Ignore Rules:** Supports `.extractignore` with `fnmatch` glob patterns.

## Installation

### Prerequisites (Fedora)
```bash
sudo dnf install meson ninja-build clang-tools-extra
```

### Build and Install
```bash
meson setup build
ninja -C build
sudo meson install -C build
```
This will install the `flatty` binary to `/usr/local/bin/` and default configs to `/usr/local/share/flatty/`.

## Usage

```bash
flatty [PATH] [OPTIONS]
```

### Options
- `-o, --output <file>`: Specify output file (default: `project_content.txt`)
- `-i, --ignore <file>`: Specify a custom ignore file instead of the default
- `-c, --config <file>`: Specify a custom config file
- `-h, --help`: Show the help message

### Examples
Pack the current directory:
```bash
flatty .
```

Pack a specific project into a custom output file:
```bash
flatty /path/to/project -o context.txt
```

## Configuration
Flatty looks for `config.conf` in its data directory (e.g., `/usr/local/share/flatty/`).
You can override these settings:
```ini
output_file=project_content.txt
skip_binary_files=true
verbose_output=false
```

## Ignore Rules (`.extractignore`)
You can place an `.extractignore` file in your project root to supplement the default ignore rules.
- `*.log`: Ignores all `.log` files at any depth.
- `build/`: Ignores the `build` directory and its contents.
- `src/generated.c`: Ignores a specific root-relative file.
