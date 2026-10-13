#include "renderers/vulkan/VulkanShaderCompiler.h"
#include <array>
#include <atomic>
#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {
class TemporaryDirectory {
    std::filesystem::path root_;
  public:
    std::filesystem::path path;
    TemporaryDirectory() : root_(std::filesystem::canonical(std::filesystem::temp_directory_path())) {
        static std::atomic<std::uint64_t> sequence{0};
        std::random_device random;
        for (unsigned attempt = 0; attempt < 128; ++attempt) {
            path = root_ / ("threecpp shader " + std::to_string(random()) + "-" +
                            std::to_string(sequence.fetch_add(1, std::memory_order_relaxed)));
            std::error_code error;
            if (std::filesystem::create_directory(path, error)) return;
            if (error && error != std::errc::file_exists)
                throw std::filesystem::filesystem_error("Cannot create shader compiler directory", path, error);
        }
        throw std::runtime_error("Cannot allocate shader compiler directory");
    }
    ~TemporaryDirectory() {
        // Only remove the exact new directory under the resolved temporary root.
        try {
            if (path.is_absolute() && path.parent_path() == root_ &&
                path.filename().string().starts_with("threecpp shader ")) {
                std::error_code ignored;
                std::filesystem::remove_all(path, ignored);
            }
        } catch (...) {} // Cleanup must not replace a compilation exception.
    }
    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
};
void save(const std::filesystem::path& path, const std::string& source) {
    std::ofstream stream(path, std::ios::binary);
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    stream.write(source.data(), static_cast<std::streamsize>(source.size()));
    stream.close(); // Report final buffered-write and close errors before launching glslang.
}
std::string readText(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot read shader compiler diagnostics");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
std::vector<std::uint32_t> readSpirv(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("Shader compiler did not produce SPIR-V");
    const auto length = stream.tellg();
    if (length < std::streamoff(20) || length % std::streamoff(4) != 0)
        throw std::runtime_error("Shader compiler produced truncated SPIR-V");
    std::vector<std::uint32_t> words(static_cast<std::size_t>(length / std::streamoff(4)));
    stream.seekg(0);
    stream.read(reinterpret_cast<char*>(words.data()), length);
    if (!stream || words[0] != 0x07230203u || words[3] == 0 || words[4] != 0)
        throw std::runtime_error("Shader compiler produced an invalid SPIR-V header");
    for (std::size_t at = 5; at < words.size();) {
        const auto count = words[at] >> 16;
        if (count == 0 || count > words.size() - at)
            throw std::runtime_error("Shader compiler produced an invalid SPIR-V instruction");
        at += count;
    }
    return words;
}

#ifdef _WIN32
class Handle {
  public:
    HANDLE value = INVALID_HANDLE_VALUE;
    explicit Handle(HANDLE handle = INVALID_HANDLE_VALUE) : value(handle) {}
    ~Handle() { if (value != INVALID_HANDLE_VALUE && value != nullptr) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};
std::wstring quote(const std::wstring& argument) {
    std::wstring result = L"\"";
    unsigned slashes = 0;
    for (const auto character : argument) {
        if (character == L'\\') { ++slashes; continue; }
        if (character == L'\"') result.append(slashes * 2 + 1, L'\\');
        else result.append(slashes, L'\\');
        slashes = 0;
        result.push_back(character);
    }
    result.append(slashes * 2, L'\\');
    result.push_back(L'\"');
    return result;
}
int run(const std::filesystem::path& executable, const std::filesystem::path& directory,
        const std::vector<std::string>& arguments) {
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    Handle output(CreateFileW((directory / "compiler.log").c_str(), GENERIC_WRITE,
                             FILE_SHARE_READ, &security, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    Handle input(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (output.value == INVALID_HANDLE_VALUE || input.value == INVALID_HANDLE_VALUE)
        throw std::system_error(GetLastError(), std::system_category(), "Cannot create shader compiler streams");
    SIZE_T attributeBytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
    std::vector<unsigned char> storage(attributeBytes);
    auto attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attributeBytes))
        throw std::system_error(GetLastError(), std::system_category(), "Cannot initialize compiler process attributes");
    struct Attributes {
        LPPROC_THREAD_ATTRIBUTE_LIST value;
        ~Attributes() { DeleteProcThreadAttributeList(value); }
    } guard{attributes};
    std::array<HANDLE, 2> handles{output.value, input.value};
    if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                   handles.data(), sizeof(handles), nullptr, nullptr))
        throw std::system_error(GetLastError(), std::system_category(), "Cannot restrict compiler process handles");
    STARTUPINFOEXW start{};
    start.StartupInfo.cb = sizeof(start);
    start.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    start.StartupInfo.hStdInput = input.value;
    start.StartupInfo.hStdOutput = output.value;
    start.StartupInfo.hStdError = output.value;
    start.lpAttributeList = attributes;
    std::wstring command = quote(executable.wstring());
    // All options and filenames here are ASCII constants; the executable and
    // working directory are passed through the native Unicode path APIs.
    for (const auto& argument : arguments) {
        command.push_back(L' ');
        command += quote(std::wstring(argument.begin(), argument.end()));
    }
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr,
                        directory.c_str(), &start.StartupInfo, &process))
        throw std::system_error(GetLastError(), std::system_category(), "Cannot launch glslang");
    Handle thread(process.hThread), child(process.hProcess);
    if (WaitForSingleObject(child.value, INFINITE) != WAIT_OBJECT_0)
        throw std::system_error(GetLastError(), std::system_category(), "Cannot wait for glslang");
    DWORD status = 0;
    if (!GetExitCodeProcess(child.value, &status))
        throw std::system_error(GetLastError(), std::system_category(), "Cannot read glslang exit code");
    return static_cast<int>(status);
}
#else
int compilerDescriptor(int descriptor) {
    if (descriptor < 0 || descriptor >= 3) return descriptor;
    const auto result = fcntl(descriptor, F_DUPFD_CLOEXEC, 3);
    const auto error = errno;
    close(descriptor);
    errno = error;
    return result;
}
int run(const std::filesystem::path& executable, const std::filesystem::path& directory,
        const std::vector<std::string>& arguments) {
    const auto filename = (directory / "compiler.log").string(), binary = executable.string(), cwd = directory.string();
    std::vector<std::string> owned{binary};
    owned.insert(owned.end(), arguments.begin(), arguments.end());
    std::vector<char*> argv;
    for (auto& argument : owned) argv.push_back(argument.data());
    argv.push_back(nullptr);
    // Keep the child stream descriptors above 2 even if the host has closed
    // stdin/stdout/stderr, so dup2 and close cannot clobber another redirection.
    const int output = compilerDescriptor(open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600));
    const int outputError = errno;
    const int input = compilerDescriptor(open("/dev/null", O_RDONLY | O_CLOEXEC));
    if (output < 0 || input < 0) {
        const int error = output < 0 ? outputError : errno;
        if (output >= 0) close(output);
        if (input >= 0) close(input);
        throw std::system_error(error, std::generic_category(), "Cannot create shader compiler streams");
    }
    const auto child = fork();
    if (child == 0) {
        // Only async-signal-safe operations after fork: no allocation, C++ IO,
        // locks, callbacks or changes to the parent process working directory.
        if (chdir(cwd.c_str()) != 0 || dup2(input, STDIN_FILENO) < 0 ||
            dup2(output, STDOUT_FILENO) < 0 || dup2(output, STDERR_FILENO) < 0) _exit(126);
        close(input); close(output);
        execv(binary.c_str(), argv.data());
        _exit(127);
    }
    const int error = errno;
    close(input); close(output);
    if (child < 0) throw std::system_error(error, std::generic_category(), "Cannot fork glslang");
    int status = 0;
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) throw std::system_error(errno, std::generic_category(), "Cannot wait for glslang");
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
}
#endif
}

VulkanShaderCompiler::VulkanShaderCompiler(std::filesystem::path executable)
    : executable_(std::filesystem::absolute(std::move(executable))) {
    if (!std::filesystem::is_regular_file(executable_))
        throw std::invalid_argument("glslang executable is not a regular file");
}
VulkanShaderCompiler::Result VulkanShaderCompiler::compile(const std::string& vertex,
                                                           const std::string& fragment) const {
    TemporaryDirectory directory;
    save(directory.path / "program.vert", vertex);
    save(directory.path / "program.frag", fragment);
    Result result;
    result.exitCode = run(executable_, directory.path,
        {"-V", "--target-env", "vulkan1.1", "-R", "--auto-map-bindings", "--auto-map-locations",
         "--set-default-uniform-block", "ThreeDefaultUniforms", "0", "0", "-l", "-q",
         "program.vert", "program.frag"});
    result.diagnostics = readText(directory.path / "compiler.log");
    result.linked = result.exitCode == 0;
    if (result.linked) {
        result.vertex = readSpirv(directory.path / "vert.spv");
        result.fragment = readSpirv(directory.path / "frag.spv");
    }
    return result;
}
