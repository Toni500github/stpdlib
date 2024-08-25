#ifndef __STPDLIB_HPP__
#define __STPDLIB_HPP__

#include <utility>
#include <vector>
#include <sys/stat.h>
#include <string_view>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/color.h>
#include <sstream>
#include <array>

#define _BOLD_COLOR(x) (fmt::emphasis::bold | fmt::fg(x))
void ctrl_d_handler(std::istream& cin);

template <typename... Args>
void die(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, _BOLD_COLOR(fmt::rgb(fmt::color::red)), "ERROR: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    std::exit(1);
}

constexpr std::size_t operator""_len(const char*, std::size_t ln) noexcept
{ 
    return ln;
}

/** Ask the user a yes or no question.
 * @param def The default result
 * @param fmt The format string
 * @param args Arguments in the format
 * @returns the result, y = true, f = false, only returns def if the result is def
 */
template <typename... Args>
bool askUserYorN(bool def, const std::string_view fmt, Args&&... args)
{
    const std::string& inputs_str = fmt::format("[{}/{}]: ", (def ? 'Y' : 'y'), (!def ? 'n' : 'n'));
    std::string result;
    fmt::println("{} {}", fmt::format(fmt, std::forward<Args>(args)...), inputs_str);

    while (std::getline(std::cin, result) && (result.length() > 1))
        fmt::print(_BOLD_COLOR(fmt::rgb(fmt::color::yellow)), ("Please answear y or n {}"), inputs_str);

    ctrl_d_handler(std::cin);

    if (result.empty())
        return def;

    if (def ? tolower(result[0]) != 'n' : tolower(result[0]) != 'y')
        return def;

    return !def;
}

namespace
{

// https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c#874160
bool hasEnding(const std::string_view fullString, const std::string_view ending)
{
    if (ending.length() > fullString.length())
        return false;
    return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
}

bool hasStart(const std::string_view fullString, const std::string_view start)
{
    if (start.length() > fullString.length())
        return false;
    return (fullString.substr(0, start.size()) == start);
}

std::vector<std::string> split(const std::string_view text, char delim)
{
    std::string              line;
    std::vector<std::string> vec;
    std::stringstream        ss(text.data());
    while (std::getline(ss, line, delim))
        vec.push_back(line);

    return vec;
}

void ctrl_d_handler(const std::istream& cin)
{
    if (cin.eof())
        die("Exiting due to CTRL-D or EOF");
}

std::string which(const std::string& command)
{
    const std::string& env = std::getenv("PATH");
    struct stat sb;
    std::string fullPath;
    
    for (const std::string& dir : split(env, ':'))
    {
        fullPath = dir + "/" + command;
        if ((stat(fullPath.c_str(), &sb) == 0) && sb.st_mode & S_IXUSR)
            return fullPath;
    }
    
    return ""; // not found
}

// https://gist.github.com/GenesisFR/cceaf433d5b42dcdddecdddee0657292
void replace_str(std::string& str, const std::string_view from, const std::string_view to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();  // Handles case where 'to' is a substring of 'from'
    }
}

fmt::rgb hexStringToColor(const std::string_view hexstr)
{
    // convert the hexadecimal string to individual components
    std::stringstream ss;
    ss << std::hex << hexstr.substr(1).data();

    uint intValue;
    ss >> intValue;

    uint red   = (intValue >> 16) & 0xFF;
    uint green = (intValue >> 8) & 0xFF;
    uint blue  = intValue & 0xFF;

    return fmt::rgb(red, green, blue);
}

std::string str_tolower(std::string str)
{
    for (char& x : str)
        x = std::tolower(x);

    return str;
}

std::string str_toupper(std::string str)
{
    for (char& x : str)
        x = std::toupper(x);

    return str;
}

// http://stackoverflow.com/questions/478898/ddg#478960
std::string shell_exec(const std::string_view cmd)
{
    std::array<char, 1024> buffer;
    std::string            result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);

    if (!pipe)
        die("popen() failed: {}", std::strerror(errno));

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();

    // why there is a '\n' at the end??
    if (!result.empty() && result.back() == '\n')
        result.pop_back();
    return result;
}

/*
 * Get the user config directory
 * either from $XDG_CONFIG_HOME or from $HOME/.config/
 * @return user's config directory
 */
std::string getHomeConfigDir()
{
    const char* dir = std::getenv("XDG_CONFIG_HOME");
    if (dir != NULL && dir[0] != '\0' && std::filesystem::exists(dir))
    {
        std::string str_dir(dir);
        return hasEnding(str_dir, "/") ? str_dir.substr(0, str_dir.rfind('/')) : str_dir;
    }
    else
    {
        const char* home = std::getenv("HOME");
        if (home == nullptr)
            die("Failed to find $HOME, set it to your home directory!");

        return std::string(home) + "/.config";
    }
}

}
#endif
