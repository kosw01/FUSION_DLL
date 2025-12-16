#include "kalman_filter.h"
#include <cmath>
#include <iostream>

namespace fusion {

KalmanFilter::KalmanFilter(const KalmanParams& params) 
    : params_(params) {
    // 초기 상태는 나중에 reset()에서 설정됨
    state_.position = 0.0;
    state_.velocity = 0.0;
    
    // 초기 공분산 행렬 (단위 행렬)
    covariance_.p00 = 1.0;
    covariance_.p01 = 0.0;
    covariance_.p10 = 0.0;
    covariance_.p11 = 1.0;
}

void KalmanFilter::reset(double initial_position) {
    state_.position = initial_position;
    state_.velocity = 0.0;
    
    // 초기 공분산 행렬
    covariance_.p00 = 1.0;
    covariance_.p01 = 0.0;
    covariance_.p10 = 0.0;
    covariance_.p11 = 1.0;
}

void KalmanFilter::predict(double acc_input) {
    double dt = params_.dt;
    
    // 상태 전이 행렬 F
    // F = [[1.0, dt],
    //      [0.0, 1.0]]
    
    // 제어 입력 행렬 B
    // B = [[0.5 * dt^2],
    //      [dt]]
    
    // 예측 단계: x_pred = F * x + B * u
    double new_position = state_.position + dt * state_.velocity + 0.5 * dt * dt * acc_input;
    double new_velocity = state_.velocity + dt * acc_input;
    
    // 공분산 예측: P_pred = F * P * F^T + Q
    // Q는 대각 행렬이므로 Q_matrix = [[Q, 0], [0, Q]]
    double p00_new = covariance_.p00 + dt * (covariance_.p01 + covariance_.p10) + dt * dt * covariance_.p11 + params_.Q;
    double p01_new = covariance_.p01 + dt * covariance_.p11;
    double p10_new = covariance_.p10 + dt * covariance_.p11;
    double p11_new = covariance_.p11 + params_.Q;
    
    // 상태 업데이트
    state_.position = new_position;
    state_.velocity = new_velocity;
    
    // 공분산 업데이트
    covariance_.p00 = p00_new;
    covariance_.p01 = p01_new;
    covariance_.p10 = p10_new;
    covariance_.p11 = p11_new;
}

void KalmanFilter::update(double gps_measurement) {
    // 측정 행렬 H = [1.0, 0.0] (위치만 측정)
    
    // 잔차 계산: y = z - H * x_pred
    double y = gps_measurement - state_.position;
    
    // 잔차 공분산: S = H * P * H^T + R
    double S = covariance_.p00 + params_.R;
    
    // 칼만 게인: K = P * H^T * S^(-1)
    double K0 = covariance_.p00 / S;  // 위치에 대한 게인
    double K1 = covariance_.p10 / S;  // 속도에 대한 게인
    
    // 상태 업데이트: x = x_pred + K * y
    state_.position = state_.position + K0 * y;
    state_.velocity = state_.velocity + K1 * y;
    
    // 공분산 업데이트: P = (I - K * H) * P_pred
    double I_minus_KH_00 = 1.0 - K0;
    double I_minus_KH_01 = 0.0;
    double I_minus_KH_10 = -K1;
    double I_minus_KH_11 = 1.0;
    
    double p00_new = I_minus_KH_00 * covariance_.p00 + I_minus_KH_01 * covariance_.p10;
    double p01_new = I_minus_KH_00 * covariance_.p01 + I_minus_KH_01 * covariance_.p11;
    double p10_new = I_minus_KH_10 * covariance_.p00 + I_minus_KH_11 * covariance_.p10;
    double p11_new = I_minus_KH_10 * covariance_.p01 + I_minus_KH_11 * covariance_.p11;
    
    covariance_.p00 = p00_new;
    covariance_.p01 = p01_new;
    covariance_.p10 = p10_new;
    covariance_.p11 = p11_new;
}

std::vector<double> KalmanFilter::process(
    const std::vector<double>& gps_data,
    const std::vector<double>& acc_data,
    const std::vector<int>& fix_data) {
    
    size_t n = gps_data.size();
    if (n != acc_data.size() || n != fix_data.size()) {
        std::cerr << "Error: GPS, ACC, and Fix data size mismatch" << std::endl;
        return std::vector<double>();
    }
    
    if (n == 0) {
        return std::vector<double>();
    }
    
    // 초기화: 첫 번째 유효한 GPS 측정값(Fix >= 1)을 초기 위치로 사용
    double initial_position = gps_data[0];
    for (size_t i = 0; i < n; i++) {
        if (fix_data[i] >= 1 && !std::isnan(gps_data[i]) && std::isfinite(gps_data[i])) {
            initial_position = gps_data[i];
            break;
        }
    }
    reset(initial_position);
    
    std::vector<double> displacement(n);
    displacement[0] = state_.position;
    
    // 칼만 필터 처리
    for (size_t i = 1; i < n; i++) {
        // 예측 단계 (가속도 입력 사용)
        predict(acc_data[i]);
        
        // 업데이트 단계 (GPS 측정값 사용)
        // Fix >= 1이고 GPS 데이터가 유효한 경우에만 업데이트
        if (fix_data[i] >= 1 && !std::isnan(gps_data[i]) && std::isfinite(gps_data[i])) {
            update(gps_data[i]);
        }
        
        displacement[i] = state_.position;
    }
    
    return displacement;
}

std::vector<double> KalmanFilter::processBatch(
    const std::vector<double>& gps_data,
    const std::vector<double>& acc_data,
    const std::vector<int>& fix_data) {
    
    size_t n = gps_data.size();
    if (n != acc_data.size() || n != fix_data.size()) {
        std::cerr << "Error: GPS, ACC, and Fix data size mismatch" << std::endl;
        return std::vector<double>();
    }
    
    if (n == 0) {
        return std::vector<double>();
    }
    
    std::vector<double> displacement(n);
    
    // 첫 번째 데이터는 현재 상태 사용 (초기화 없음)
    displacement[0] = state_.position;
    
    // 칼만 필터 처리
    for (size_t i = 1; i < n; i++) {
        // 예측 단계 (가속도 입력 사용)
        predict(acc_data[i]);
        
        // 업데이트 단계 (GPS 측정값 사용)
        // Fix >= 1이고 GPS 데이터가 유효한 경우에만 업데이트
        if (fix_data[i] >= 1 && !std::isnan(gps_data[i]) && std::isfinite(gps_data[i])) {
            update(gps_data[i]);
        }
        
        displacement[i] = state_.position;
    }
    
    return displacement;
}

} // namespace fusion

