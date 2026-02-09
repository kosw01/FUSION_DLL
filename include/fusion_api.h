#ifndef FUSION_API_H
#define FUSION_API_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #ifdef FUSION_DLL_EXPORTS
        #define FUSION_API __declspec(dllexport)
    #else
        #define FUSION_API __declspec(dllimport)
    #endif
#else
    #define FUSION_API
#endif

// 오류 코드
typedef enum {
    FUSION_SUCCESS = 0,
    FUSION_ERROR_FILE_NOT_FOUND = -1,
    FUSION_ERROR_INVALID_DATA = -2,
    FUSION_ERROR_INSUFFICIENT_DATA = -3,
    FUSION_ERROR_MEMORY = -4,
    FUSION_ERROR_UNKNOWN = -99
} FusionErrorCode;

/**
 * CSV 파일을 읽어서 GNSS-ACC 융합을 수행하고 결과를 저장
 * 
 * @param input_file_path 입력 CSV 파일 경로 (DateTime, GPS_Y, GPS_Z, Acc_Y, Acc_Z, Fix)
 * @param output_file_path 출력 CSV 파일 경로 (DateTime, Displacement_Y, Displacement_Z)
 * @param Q 프로세스 노이즈 공분산 (기본값: 0.1)
 * @param R 측정 노이즈 공분산 (기본값: 0.01)
 * @return 성공 시 FUSION_SUCCESS, 실패 시 오류 코드
 */
FUSION_API int fusion_process_csv(
    const char* input_file_path,
    const char* output_file_path,
    double Q,
    double R
);

/**
 * CSV 파일을 배치 단위로 읽어서 GNSS-ACC 융합을 수행하고 결과를 저장
 * 
 * @param input_file_path 입력 CSV 파일 경로 (DateTime, GPS_Y, GPS_Z, Acc_Y, Acc_Z, Fix)
 * @param output_file_path 출력 CSV 파일 경로 (DateTime, Displacement_Y, Displacement_Z)
 * @param Q 프로세스 노이즈 공분산
 * @param R 측정 노이즈 공분산
 * @param batch_size 배치 크기 (기본값: 100)
 * @param save_intermediate 중간 결과 저장 여부 (1: 저장, 0: 저장 안함)
 * @return 성공 시 FUSION_SUCCESS, 실패 시 오류 코드
 */
FUSION_API int fusion_process_csv_batch(
    const char* input_file_path,
    const char* output_file_path,
    double Q,
    double R,
    size_t batch_size,
    int save_intermediate
);

/**
 * 실시간 모드로 CSV를 처리하고 상태를 파일로 저장/복원
 *
 * @param input_file_path 입력 CSV 파일 경로 (DateTime, GPS_Y, GPS_Z, Acc_Y, Acc_Z, Fix)
 * @param output_file_path 출력 CSV 파일 경로 (DateTime, Displacement_Y, Displacement_Z)
 * @param Q 프로세스 노이즈 공분산
 * @param R 측정 노이즈 공분산
 * @return 성공 시 FUSION_SUCCESS, 실패 시 오류 코드
 */
FUSION_API int fusion_process_csv_realtime(
    const char* input_file_path,
    const char* output_file_path,
    double Q,
    double R
);

/**
 * 오류 코드를 문자열로 변환
 * 
 * @param error_code 오류 코드
 * @return 오류 메시지 문자열
 */
FUSION_API const char* fusion_get_error_message(int error_code);

#ifdef __cplusplus
}
#endif

#endif // FUSION_API_H


