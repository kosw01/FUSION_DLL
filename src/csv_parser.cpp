#include "csv_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace fusion {

bool parse_csv(const std::string& file_path, std::vector<InputData>& data) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << file_path << std::endl;
        return false;
    }
    
    std::string line;
    bool is_first_line = true;
    
    while (std::getline(file, line)) {
        // 빈 줄 건너뛰기
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }
        
        // 헤더 줄 건너뛰기
        if (is_first_line) {
            is_first_line = false;
            // 헤더인지 확인 (DateTime으로 시작하는지)
            std::string lower_line = line;
            std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);
            if (lower_line.find("datetime") != std::string::npos) {
                continue;
            }
        }
        
        std::istringstream iss(line);
        std::string token;
        InputData row;
        
        // CSV 파싱 (쉼표로 구분)
        std::vector<std::string> tokens;
        std::string current_token;
        bool in_quotes = false;
        
        for (char c : line) {
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ',' && !in_quotes) {
                tokens.push_back(current_token);
                current_token.clear();
            } else {
                current_token += c;
            }
        }
        tokens.push_back(current_token); // 마지막 토큰
        
        // 토큰 정리 (앞뒤 공백 제거)
        for (auto& t : tokens) {
            // 앞뒤 공백 제거
            t.erase(0, t.find_first_not_of(" \t\r\n"));
            t.erase(t.find_last_not_of(" \t\r\n") + 1);
            // 따옴표 제거
            if (!t.empty() && t.front() == '"' && t.back() == '"') {
                t = t.substr(1, t.length() - 2);
            }
        }
        
        // 최소 6개 컬럼 필요
        if (tokens.size() < 6) {
            std::cerr << "Warning: Insufficient columns in line: " << line << std::endl;
            continue;
        }
        
        try {
            row.datetime = tokens[0];
            row.gps_y = tokens[1].empty() ? 0.0 : std::stod(tokens[1]);
            row.gps_z = tokens[2].empty() ? 0.0 : std::stod(tokens[2]);
            row.acc_y = tokens[3].empty() ? 0.0 : std::stod(tokens[3]);
            row.acc_z = tokens[4].empty() ? 0.0 : std::stod(tokens[4]);
            row.fix = tokens[5].empty() ? 0 : std::stoi(tokens[5]);
            
            data.push_back(row);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Error parsing line: " << line << " - " << e.what() << std::endl;
            continue;
        }
    }
    
    file.close();
    return true;
}

bool save_csv(const std::string& file_path, const std::vector<OutputData>& data) {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create file " << file_path << std::endl;
        return false;
    }
    
    // 헤더 작성
    file << "DateTime,Displacement_Y,Displacement_Z\n";
    
    // 데이터 작성
    for (const auto& row : data) {
        file << row.datetime << ","
             << row.displacement_y << ","
             << row.displacement_z << "\n";
    }
    
    file.close();
    return true;
}

} // namespace fusion

