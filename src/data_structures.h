#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <vector>
#include <string>

namespace fusion {

// CSV 입력 데이터 구조 (6개 컬럼)
struct InputData {
    std::string datetime;  // yyyy-mm-dd HH:MM:SS.fff
    double gps_y;          // GPS_Y
    double gps_z;          // GPS_Z
    double acc_y;          // Acc_Y
    double acc_z;          // Acc_Z
    int fix;               // Fix
};

// 칼만 필터 상태 벡터 [위치, 속도]
struct KalmanState {
    double position;  // 위치 (displacement)
    double velocity;  // 속도
};

// 칼만 필터 공분산 행렬 (2x2)
struct KalmanCovariance {
    double p00, p01;
    double p10, p11;
};

// 출력 데이터 구조
struct OutputData {
    std::string datetime;
    double displacement_y;
    double displacement_z;
};

// 칼만 필터 파라미터
struct KalmanParams {
    double Q;  // 프로세스 노이즈 공분산 (기본값: 0.1)
    double R;  // 측정 노이즈 공분산 (기본값: 0.01)
    double dt; // 시간 간격 (고정값: 0.01)
    
    KalmanParams() : Q(0.1), R(0.01), dt(0.01) {}
    KalmanParams(double q, double r) : Q(q), R(r), dt(0.01) {}
};

} // namespace fusion

#endif // DATA_STRUCTURES_H

