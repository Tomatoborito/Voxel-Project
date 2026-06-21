#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>

inline std::string readFile(const std::string& path) {
	std::ifstream file(path);
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}
