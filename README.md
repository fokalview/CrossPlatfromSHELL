# 🖥️ Lightweight Cross-Platform Shell

A minimalist shell application built in C that works on both Linux and Windows. Designed as a hands-on deep dive into how shells really work, this project avoids `system()` calls and instead uses raw syscalls, process creation, and resource tracking to manage user commands interactively.

## 🚀 Features

* **Cross-Platform**: Works on both Linux and Windows with platform-specific process handling.
* **Interactive Shell Mode**: Accepts and executes user commands from a custom prompt.
* **Built-in Commands**:
  * `cd [dir]` – change directories
  * `set prompt = new_prompt:` – update the prompt
  * `echo [text]` – print to terminal
  * `dir / ls` – list directory contents
  * `pwd` – print working directory
  * `sleep [n] &` – run in background
  * `jobs` – view background processes
  * `exit` – terminate the shell
* **Background Job Support**: Execute commands asynchronously and track status.
* **System Statistics**: Capture CPU usage, elapsed time, and memory stats after each command.

## 🛠️ Technologies Used

* **C**: Core programming language
* **Windows API / Unix Syscalls**: For process and resource management
* **POSIX / Win32 conditionals**: Handle platform-specific behavior

## 📂 Project Structure

```
shell_project/
├── P2_Checkpoint_MB.c       # Main source file
```

## 📸 Demo Preview

![Shell Screenshot](screenshot.png)

> *Note: Replace with an actual screenshot if desired.*

## 📈 Potential Enhancements

* **Command history navigation**
* **I/O redirection and piping**
* **Better error messages and logging**
* **Unit tests and CI pipeline**

## 📄 License

This project is open-source and available under the [MIT License](LICENSE).
