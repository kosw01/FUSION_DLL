# GNSS-ACC Fusion DLL

GNSS와 가속도계 데이터를 칼만 필터로 융합하는 DLL 라이브러리입니다.

## 파일 구조

```
FUSION/
├── README.md              # 이 파일
├── lib/                   # 라이브러리 파일
│   ├── fusion_dll.dll    # DLL 파일 (런타임 필요)
│   └── fusion_dll.lib    # Import library (링크 시 필요)
├── bin/                   # 실행 파일 (테스트용)
│   ├── test_fusion.exe   # 테스트 프로그램
│   └── fusion_dll.dll    # DLL 파일 (test_fusion.exe 실행 시 필요)
└── include/              # 헤더 파일
    └── fusion_api.h      # API 헤더 파일
```

## 빠른 시작

### 1. DLL 파일 위치

프로젝트에서 DLL을 사용하려면:
- **옵션 1**: `fusion_dll.dll`을 실행 파일과 같은 디렉토리에 복사
- **옵션 2**: 시스템 PATH에 `lib` 디렉토리 추가

### 2. 프로젝트 설정

#### Visual Studio 프로젝트 설정

1. **헤더 파일 경로 추가**
   - 프로젝트 속성 → C/C++ → 일반 → 추가 포함 디렉토리
   - `FUSION\include` 경로 추가

2. **라이브러리 경로 추가**
   - 프로젝트 속성 → 링커 → 일반 → 추가 라이브러리 디렉토리
   - `FUSION\lib` 경로 추가

3. **라이브러리 링크**
   - 프로젝트 속성 → 링커 → 입력 → 추가 종속성
   - `fusion_dll.lib` 추가

#### CMake 프로젝트 설정

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_project)

add_executable(my_app main.cpp)

# 헤더 경로 추가
target_include_directories(my_app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/FUSION/include
)

# 라이브러리 링크
target_link_directories(my_app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/FUSION/lib
)
target_link_libraries(my_app fusion_dll)
```

### 3. 사용 예제

```c
#include "fusion_api.h"
#include <stdio.h>

int main() {
    // 일반 처리 모드
    int result = fusion_process_csv(
        "input.csv",      // 입력 CSV 파일 경로
        "output.csv",     // 출력 CSV 파일 경로
        0.1,              // Q (프로세스 노이즈 공분산)
        0.01              // R (측정 노이즈 공분산)
    );
    
    if (result == FUSION_SUCCESS) {
        printf("처리 완료!\n");
    } else {
        const char* error_msg = fusion_get_error_message(result);
        printf("오류: %s\n", error_msg);
    }
    
    return 0;
}
```

## API 문서

### 함수 목록

#### `fusion_process_csv`

일반 처리 모드로 CSV 파일을 처리합니다.

```c
int fusion_process_csv(
    const char* input_file_path,   // 입력 CSV 파일 경로
    const char* output_file_path,   // 출력 CSV 파일 경로
    double Q,                       // 프로세스 노이즈 공분산 (기본값: 0.1)
    double R                        // 측정 노이즈 공분산 (기본값: 0.01)
);
```

**반환값:**
- `FUSION_SUCCESS` (0): 성공
- 음수 값: 오류 코드 (오류 메시지는 `fusion_get_error_message()`로 확인)

#### `fusion_process_csv_batch`

배치 처리 모드로 CSV 파일을 처리합니다. 대용량 파일에 적합합니다.

```c
int fusion_process_csv_batch(
    const char* input_file_path,   // 입력 CSV 파일 경로
    const char* output_file_path,   // 출력 CSV 파일 경로
    double Q,                       // 프로세스 노이즈 공분산
    double R,                       // 측정 노이즈 공분산
    size_t batch_size,              // 배치 크기 (기본값: 100)
    int save_intermediate           // 중간 결과 저장 여부 (1: 저장, 0: 저장 안함)
);
```

**반환값:**
- `FUSION_SUCCESS` (0): 성공
- 음수 값: 오류 코드

#### `fusion_get_error_message`

오류 코드를 문자열로 변환합니다.

```c
const char* fusion_get_error_message(int error_code);
```

### 오류 코드

```c
typedef enum {
    FUSION_SUCCESS = 0,                    // 성공
    FUSION_ERROR_FILE_NOT_FOUND = -1,      // 파일을 찾을 수 없음
    FUSION_ERROR_INVALID_DATA = -2,         // 잘못된 데이터 형식
    FUSION_ERROR_INSUFFICIENT_DATA = -3,   // 데이터 부족 (최소 20행 필요)
    FUSION_ERROR_MEMORY = -4,               // 메모리 할당 오류
    FUSION_ERROR_UNKNOWN = -99              // 알 수 없는 오류
} FusionErrorCode;
```

## 입력 데이터 형식

### CSV 파일 형식

**헤더:**
```csv
DateTime,GPS_Y,GPS_Z,Acc_Y,Acc_Z,Fix
```

**데이터 형식:**
- `DateTime`: 날짜/시간 (형식: `yyyy-mm-dd HH:MM:SS.fff`)
- `GPS_Y`: Y축 GPS 위치 (단위: 미터)
- `GPS_Z`: Z축 GPS 위치 (단위: 미터)
- `Acc_Y`: Y축 가속도 (단위: m/s²)
- `Acc_Z`: Z축 가속도 (단위: m/s²)
- `Fix`: GPS Fix 상태 (1 이상: 유효, 0: 무효)

**중요:** Fix >= 1인 데이터만 GPS 측정값으로 사용됩니다.

**예시:**
```csv
DateTime,GPS_Y,GPS_Z,Acc_Y,Acc_Z,Fix
2024-01-01 12:00:00.000,100.5,113.196,0.1,0.2,1
2024-01-01 12:00:00.010,100.6,113.197,0.15,0.25,1
2024-01-01 12:00:00.020,100.7,113.198,0.12,0.22,0
```

## 출력 데이터 형식

### CSV 파일 형식

**헤더:**
```csv
DateTime,Displacement_Y,Displacement_Z
```

**데이터 형식:**
- `DateTime`: 입력 데이터의 날짜/시간
- `Displacement_Y`: Y축 변위 (칼만 필터 융합 결과)
- `Displacement_Z`: Z축 변위 (칼만 필터 융합 결과)

**예시:**
```csv
DateTime,Displacement_Y,Displacement_Z
2024-01-01 12:00:00.000,100.5,113.196
2024-01-01 12:00:00.010,100.6,113.197
2024-01-01 12:00:00.020,100.7,113.198
```

## 테스트 프로그램 사용법

### 기본 사용법

```bash
cd FUSION\bin
test_fusion.exe input.csv output.csv
```

### 파라미터 지정

```bash
test_fusion.exe input.csv output.csv 0.1 0.01
```

- `0.1`: Q (프로세스 노이즈 공분산)
- `0.01`: R (측정 노이즈 공분산)

### 배치 처리 모드

```bash
test_fusion.exe --batch input.csv output.csv 0.1 0.01 100
```

- `--batch`: 배치 모드 활성화
- `100`: 배치 크기 (100줄씩 처리)

## 알고리즘 설명

### 칼만 필터

본 라이브러리는 칼만 필터를 사용하여 GNSS 위치 데이터와 가속도 데이터를 융합합니다.

**상태 벡터:**
- 위치 (position)
- 속도 (velocity)

**처리 과정:**
1. **예측 단계**: 가속도 데이터를 사용하여 다음 시점의 위치와 속도를 예측
   - 등가속도 운동 방정식 사용: `x = x0 + v0*dt + 0.5*a*dt²`
2. **업데이트 단계**: GPS 측정값이 유효할 때 (Fix >= 1) 예측값을 보정
   - 칼만 게인을 사용하여 예측값과 측정값을 융합

**특징:**
- GPS 데이터가 없는 경우 (Fix < 1) 예측값만 사용
- 첫 번째 데이터는 GPS 값을 그대로 사용 (초기화)
- 이후 데이터부터 예측-업데이트 반복

## 시스템 요구사항

- **운영체제**: Windows 10 이상
- **아키텍처**: x64
- **런타임**: Visual C++ Redistributable (필요한 경우)


## 버전 정보

- **버전**: 1.0.0
- **빌드 날짜**: 2025-12-16
- **컴파일러**: MSVC 19.50 (Visual Studio 2026)

