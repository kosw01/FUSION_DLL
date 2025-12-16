#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include "data_structures.h"
#include <vector>

namespace fusion {

/**
 * 칼만 필터 클래스
 */
class KalmanFilter {
public:
    KalmanFilter(const KalmanParams& params);
    
    /**
     * 데이터를 처리하여 변위(displacement)를 계산
     * 
     * @param gps_data GNSS 측정값 벡터 (GPS_Y 또는 GPS_Z)
     * @param acc_data 가속도 데이터 벡터 (Acc_Y 또는 Acc_Z)
     * @param fix_data Fix 값 벡터 (Fix >= 1일 때만 GPS 유효)
     * @return 계산된 변위 벡터
     */
    std::vector<double> process(
        const std::vector<double>& gps_data,
        const std::vector<double>& acc_data,
        const std::vector<int>& fix_data
    );
    
    /**
     * 필터 상태 초기화
     */
    void reset(double initial_position);
    
    /**
     * 현재 상태 가져오기
     */
    KalmanState getState() const { return state_; }
    
    /**
     * 상태 설정하기
     */
    void setState(const KalmanState& state) { state_ = state; }
    
    /**
     * 현재 공분산 가져오기
     */
    KalmanCovariance getCovariance() const { return covariance_; }
    
    /**
     * 공분산 설정하기
     */
    void setCovariance(const KalmanCovariance& cov) { covariance_ = cov; }
    
    /**
     * 배치 처리를 위한 데이터 처리 (초기화 없이)
     * 
     * @param gps_data GNSS 측정값 벡터
     * @param acc_data 가속도 데이터 벡터
     * @param fix_data Fix 값 벡터 (Fix >= 1일 때만 GPS 유효)
     * @return 계산된 변위 벡터
     */
    std::vector<double> processBatch(
        const std::vector<double>& gps_data,
        const std::vector<double>& acc_data,
        const std::vector<int>& fix_data
    );

private:
    KalmanParams params_;
    KalmanState state_;
    KalmanCovariance covariance_;
    
    void predict(double acc_input);
    void update(double gps_measurement);
};

} // namespace fusion

#endif // KALMAN_FILTER_H

