#include "fusion_api.h"
#include "csv_parser.h"
#include "kalman_filter.h"
#include "data_structures.h"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

// filesystem 헤더 호환성 처리
#if __cplusplus >= 201703L && defined(__has_include)
    #if __has_include(<filesystem>)
        #include <filesystem>
        namespace fs = std::filesystem;
    #elif __has_include(<experimental/filesystem>)
        #include <experimental/filesystem>
        namespace fs = std::experimental::filesystem;
    #endif
#elif defined(_WIN32) && _MSC_VER >= 1914
    #include <filesystem>
    namespace fs = std::filesystem;
#else
    // filesystem을 사용할 수 없는 경우, 문자열 조작으로 대체
    #include <algorithm>
    namespace fs {
        struct path {
            std::string p;
            path(const std::string& s) : p(s) {}
            std::string stem() const {
                size_t last_slash = p.find_last_of("/\\");
                size_t last_dot = p.find_last_of(".");
                if (last_dot != std::string::npos && (last_slash == std::string::npos || last_dot > last_slash)) {
                    return p.substr(last_slash + 1, last_dot - last_slash - 1);
                }
                return p.substr(last_slash + 1);
            }
            std::string extension() const {
                size_t last_dot = p.find_last_of(".");
                if (last_dot != std::string::npos) {
                    return p.substr(last_dot);
                }
                return "";
            }
            std::string parent_path() const {
                size_t last_slash = p.find_last_of("/\\");
                if (last_slash != std::string::npos) {
                    return p.substr(0, last_slash);
                }
                return "";
            }
        };
    }
#endif

#define FUSION_DLL_EXPORTS

namespace fusion {

// 내부 구현 함수
int process_fusion_internal(
    const std::string& input_file_path,
    const std::string& output_file_path,
    double Q,
    double R) {
    
    // 최소 데이터 요구사항 확인
    const size_t MIN_ROWS = 20;
    
    // CSV 파일 파싱
    std::vector<InputData> input_data;
    if (!parse_csv(input_file_path, input_data)) {
        return FUSION_ERROR_FILE_NOT_FOUND;
    }
    
    // 최소 데이터 개수 확인
    if (input_data.size() < MIN_ROWS) {
        std::cerr << "Error: Insufficient data. Minimum " << MIN_ROWS 
                  << " rows required, but got " << input_data.size() << std::endl;
        return FUSION_ERROR_INSUFFICIENT_DATA;
    }
    
    // 데이터 추출
    std::vector<double> gps_y_data;
    std::vector<double> gps_z_data;
    std::vector<double> acc_y_data;
    std::vector<double> acc_z_data;
    std::vector<int> fix_data;
    std::vector<std::string> datetime_data;
    
    for (const auto& row : input_data) {
        datetime_data.push_back(row.datetime);
        gps_y_data.push_back(row.gps_y);
        gps_z_data.push_back(row.gps_z);
        acc_y_data.push_back(row.acc_y);
        acc_z_data.push_back(row.acc_z);
        fix_data.push_back(row.fix);
    }
    
    // 칼만 필터 파라미터 설정
    KalmanParams params(Q, R);
    
    // Y 방향 칼만 필터 처리
    KalmanFilter filter_y(params);
    std::vector<double> displacement_y = filter_y.process(gps_y_data, acc_y_data, fix_data);
    
    // Z 방향 칼만 필터 처리
    KalmanFilter filter_z(params);
    std::vector<double> displacement_z = filter_z.process(gps_z_data, acc_z_data, fix_data);
    
    // 출력 데이터 생성
    std::vector<OutputData> output_data;
    size_t n = datetime_data.size();
    
    for (size_t i = 0; i < n; i++) {
        OutputData output_row;
        output_row.datetime = datetime_data[i];
        output_row.displacement_y = displacement_y[i];
        output_row.displacement_z = displacement_z[i];
        output_data.push_back(output_row);
    }
    
    // CSV 파일로 저장
    if (!save_csv(output_file_path, output_data)) {
        return FUSION_ERROR_FILE_NOT_FOUND;
    }
    
    return FUSION_SUCCESS;
}

// 배치 처리 내부 구현 함수
int process_fusion_batch_internal(
    const std::string& input_file_path,
    const std::string& output_file_path,
    double Q,
    double R,
    size_t batch_size,
    bool save_intermediate) {
    
    // 최소 데이터 요구사항 확인
    const size_t MIN_ROWS = 20;
    
    // CSV 파일 파싱
    std::vector<InputData> input_data;
    if (!parse_csv(input_file_path, input_data)) {
        return FUSION_ERROR_FILE_NOT_FOUND;
    }
    
    // 최소 데이터 개수 확인
    if (input_data.size() < MIN_ROWS) {
        std::cerr << "Error: Insufficient data. Minimum " << MIN_ROWS 
                  << " rows required, but got " << input_data.size() << std::endl;
        return FUSION_ERROR_INSUFFICIENT_DATA;
    }
    
    // 배치 크기 검증
    if (batch_size < MIN_ROWS) {
        std::cerr << "Error: Batch size must be at least " << MIN_ROWS << std::endl;
        return FUSION_ERROR_INVALID_DATA;
    }
    
    // 출력 파일 경로에서 기본 이름 추출 (중간 파일명 생성용)
    fs::path output_path(output_file_path);
    std::string output_base = output_path.stem().string();
    std::string output_dir = output_path.parent_path().string();
    std::string output_ext = output_path.extension().string();
    
    // 중간 파일 저장 디렉토리 설정
    if (output_dir.empty()) {
        output_dir = ".";
    }
    
    // 칼만 필터 파라미터 설정
    KalmanParams params(Q, R);
    
    // 칼만 필터 초기화
    KalmanFilter filter_y(params);
    KalmanFilter filter_z(params);
    
    // 전체 출력 데이터 저장
    std::vector<OutputData> all_output_data;
    
    // 배치 개수 계산
    size_t total_rows = input_data.size();
    size_t num_batches = (total_rows + batch_size - 1) / batch_size;  // 올림 계산
    
    std::cout << "Processing " << total_rows << " rows in " << num_batches 
              << " batch(es) of " << batch_size << " rows each" << std::endl;
    
    // 첫 번째 배치에서 초기화
    bool is_first_batch = true;
    
    // 배치 단위로 처리
    for (size_t batch_idx = 0; batch_idx < num_batches; batch_idx++) {
        size_t start_idx = batch_idx * batch_size;
        size_t end_idx = std::min(start_idx + batch_size, total_rows);
        size_t current_batch_size = end_idx - start_idx;
        
        std::cout << "Processing batch " << (batch_idx + 1) << "/" << num_batches 
                  << " (rows " << start_idx << "-" << (end_idx - 1) << ")" << std::endl;
        
        // 현재 배치 데이터 추출
        std::vector<double> gps_y_batch;
        std::vector<double> gps_z_batch;
        std::vector<double> acc_y_batch;
        std::vector<double> acc_z_batch;
        std::vector<int> fix_batch;
        std::vector<std::string> datetime_batch;
        
        for (size_t i = start_idx; i < end_idx; i++) {
            datetime_batch.push_back(input_data[i].datetime);
            gps_y_batch.push_back(input_data[i].gps_y);
            gps_z_batch.push_back(input_data[i].gps_z);
            acc_y_batch.push_back(input_data[i].acc_y);
            acc_z_batch.push_back(input_data[i].acc_z);
            fix_batch.push_back(input_data[i].fix);
        }
        
        // 첫 번째 배치인 경우 초기화
        if (is_first_batch) {
            // 첫 번째 유효한 GPS 측정값(Fix >= 1)으로 초기화
            double initial_y = gps_y_batch[0];
            double initial_z = gps_z_batch[0];
            for (size_t i = 0; i < gps_y_batch.size(); i++) {
                if (fix_batch[i] >= 1 && !std::isnan(gps_y_batch[i]) && std::isfinite(gps_y_batch[i])) {
                    initial_y = gps_y_batch[i];
                    break;
                }
            }
            for (size_t i = 0; i < gps_z_batch.size(); i++) {
                if (fix_batch[i] >= 1 && !std::isnan(gps_z_batch[i]) && std::isfinite(gps_z_batch[i])) {
                    initial_z = gps_z_batch[i];
                    break;
                }
            }
            filter_y.reset(initial_y);
            filter_z.reset(initial_z);
            is_first_batch = false;
        } else {
            // 상태 복원 (이전 배치에서 저장된 상태 사용)
            // 상태는 이미 필터에 유지되어 있음
        }
        
        // Y 방향 칼만 필터 처리
        std::vector<double> displacement_y_batch = filter_y.processBatch(gps_y_batch, acc_y_batch, fix_batch);
        
        // Z 방향 칼만 필터 처리
        std::vector<double> displacement_z_batch = filter_z.processBatch(gps_z_batch, acc_z_batch, fix_batch);
        
        // 배치 출력 데이터 생성
        std::vector<OutputData> batch_output_data;
        for (size_t i = 0; i < current_batch_size; i++) {
            OutputData output_row;
            output_row.datetime = datetime_batch[i];
            output_row.displacement_y = displacement_y_batch[i];
            output_row.displacement_z = displacement_z_batch[i];
            batch_output_data.push_back(output_row);
        }
        
        // 전체 결과에 추가
        all_output_data.insert(all_output_data.end(), 
                              batch_output_data.begin(), 
                              batch_output_data.end());
        
        // 중간 결과 저장
        if (save_intermediate) {
            std::ostringstream intermediate_filename;
            intermediate_filename << output_dir << "/" << output_base 
                                  << "_batch_" << std::setfill('0') << std::setw(3) 
                                  << (batch_idx + 1) << output_ext;
            
            std::string intermediate_path = intermediate_filename.str();
            if (save_csv(intermediate_path, batch_output_data)) {
                std::cout << "  Intermediate result saved: " << intermediate_path << std::endl;
            } else {
                std::cerr << "  Warning: Failed to save intermediate result: " 
                          << intermediate_path << std::endl;
            }
        }
        
        // 상태 저장 (다음 배치를 위해)
        // 상태는 이미 필터 객체에 유지되어 있음
        
        std::cout << "  Batch " << (batch_idx + 1) << " completed: " 
                  << current_batch_size << " rows processed" << std::endl;
    }
    
    // 최종 결과 저장
    if (!save_csv(output_file_path, all_output_data)) {
        return FUSION_ERROR_FILE_NOT_FOUND;
    }
    
    std::cout << "Final result saved: " << output_file_path << std::endl;
    std::cout << "Total rows processed: " << all_output_data.size() << std::endl;
    
    return FUSION_SUCCESS;
}

} // namespace fusion

// C API 구현
extern "C" {

FUSION_API int fusion_process_csv(
    const char* input_file_path,
    const char* output_file_path,
    double Q,
    double R) {
    
    if (!input_file_path || !output_file_path) {
        return FUSION_ERROR_INVALID_DATA;
    }
    
    try {
        return fusion::process_fusion_internal(
            std::string(input_file_path),
            std::string(output_file_path),
            Q,
            R
        );
    } catch (const std::exception& e) {
        std::cerr << "Exception in fusion_process_csv: " << e.what() << std::endl;
        return FUSION_ERROR_UNKNOWN;
    } catch (...) {
        std::cerr << "Unknown exception in fusion_process_csv" << std::endl;
        return FUSION_ERROR_UNKNOWN;
    }
}

FUSION_API int fusion_process_csv_batch(
    const char* input_file_path,
    const char* output_file_path,
    double Q,
    double R,
    size_t batch_size,
    int save_intermediate) {
    
    if (!input_file_path || !output_file_path) {
        return FUSION_ERROR_INVALID_DATA;
    }
    
    if (batch_size == 0) {
        batch_size = 100;  // 기본값
    }
    
    try {
        return fusion::process_fusion_batch_internal(
            std::string(input_file_path),
            std::string(output_file_path),
            Q,
            R,
            batch_size,
            save_intermediate != 0
        );
    } catch (const std::exception& e) {
        std::cerr << "Exception in fusion_process_csv_batch: " << e.what() << std::endl;
        return FUSION_ERROR_UNKNOWN;
    } catch (...) {
        std::cerr << "Unknown exception in fusion_process_csv_batch" << std::endl;
        return FUSION_ERROR_UNKNOWN;
    }
}

FUSION_API const char* fusion_get_error_message(int error_code) {
    switch (error_code) {
        case FUSION_SUCCESS:
            return "Success";
        case FUSION_ERROR_FILE_NOT_FOUND:
            return "File not found or cannot be opened";
        case FUSION_ERROR_INVALID_DATA:
            return "Invalid data format";
        case FUSION_ERROR_INSUFFICIENT_DATA:
            return "Insufficient data (minimum 20 rows required)";
        case FUSION_ERROR_MEMORY:
            return "Memory allocation error";
        case FUSION_ERROR_UNKNOWN:
        default:
            return "Unknown error";
    }
}

} // extern "C"

