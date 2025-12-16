#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "data_structures.h"
#include <vector>
#include <string>

namespace fusion {

/**
 * CSV 파일을 파싱하여 InputData 벡터로 변환
 * 
 * @param file_path CSV 파일 경로
 * @param data 파싱된 데이터를 저장할 벡터
 * @return 성공 시 true, 실패 시 false
 */
bool parse_csv(const std::string& file_path, std::vector<InputData>& data);

/**
 * OutputData 벡터를 CSV 파일로 저장
 * 
 * @param file_path 출력 CSV 파일 경로
 * @param data 저장할 데이터 벡터
 * @return 성공 시 true, 실패 시 false
 */
bool save_csv(const std::string& file_path, const std::vector<OutputData>& data);

} // namespace fusion

#endif // CSV_PARSER_H

