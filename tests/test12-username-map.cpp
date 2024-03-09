#include <iostream>
#include <map>
#include <string>
#include <cstring>

std::string replaceUsername(const std::string& path, const std::string& username) {
  // Identify username presence
  size_t pos = path.find("\\Users\\");
  if (pos == std::string::npos) {
    return path; // Username not found, return original path
  }

  // Extract username (uncomment to get the username)
  // size_t start = pos + strlen("\\Users\\");
  // size_t end = path.find('\\', start);
  // std::string extractedUsername = path.substr(start, end - start);

  std::string replacementUsername = "new_username"; // Replace with your desired username

  // Replace username
  std::string newPath = path.substr(0, pos + strlen("\\Users\\"));
  newPath.append(replacementUsername);
  newPath.append(path.substr(pos + strlen("\\Users\\") + username.length() + 1));

  return newPath;
}

int main() {
  std::map<std::string, std::string> paths = {
    {"config", "C:\\Users\\markus\\AppData\\Local\\MyApp\\config.ini"},
    {"documents", "C:\\Users\\john\\Documents\\MyDocument.txt"}
  };

  std::string newUsername = "replaced_user";

  for (auto& pathPair : paths) {
    pathPair.second = replaceUsername(pathPair.second, newUsername);
    std::cout << pathPair.first << ": " << pathPair.second << std::endl;
  }

  return 0;
}