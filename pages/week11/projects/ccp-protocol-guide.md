# Chaton Communication Protocol (CCP) 명세서

본 문서는 ChatOn CLI 채팅 프로그램의 서버와 클라이언트 간 통신에 사용되는 구조체 기반 바이너리 프로토콜인 **Chaton Communication Protocol (CCP)**의 세부 명세서입니다.

VEDA 리눅스 시스템 프로그래밍 네트워크 통신을 복습하기 위해 가상의 프로젝트 샤통 (ChatOn) 통신 프로토콜을 만들어 보았습니다.

프로젝트의 핵심 목표가 프로토콜의 유연성 및 구현 난이도 최소화이므로, 복잡한 동적 데이터 직렬화(Serialization)나 정교한 메모리 효율성 설계 대신 **고정 크기 구조체 기반의 단순하고 직관적인 통신 방식**을 채택했습니다.

---

## 1. 메시지 포맷 (Client ↔ Raspberry Pi Server)

클라이언트와 서버는 서로 다른 복잡한 데이터 직렬화나 동적 파싱을 배제하고, 메시지 타입에 관계없이 항상 **고정 크기 단일 구조체**를 사용하여 송수신한다.
OS와 컴파일러 간 바이트 패딩 차이를 제거하기 위해 `#pragma pack(push, 1)` 지시자를 사용하여 바이트 정렬을 1바이트 크기로 강제하며, 4바이트 정렬 기준(총 1108바이트)에 맞추어 필드를 구성한다.

```
[공통 패킷 구조 (ccp_packet)]
┌────────────────────────────────────────────────────────┐
│  room_id  : 4 bytes (uint32_t)                         │
│  status   : 4 bytes (int32_t)                          │
│  usr_id   : 4 bytes (uint32_t)                         │
│  nickname : 64 bytes (char[])                          │
│  payload  : 1024 bytes (char[])                        │
│  type     : 4 bytes (uint32_t)                         │
│  flag     : 4 bytes (char[4])                          │
└────────────────────────────────────────────────────────┘
```

### 패킷 송수신 전략

고정 크기의 패킷을 사용하므로, TCP 상에서 스트림 처리가 매우 단순해진다. 동적 메모리 할당(malloc)이나 부분 수신 대기 루프가 불필요하며, 단 한 번의 read/write 함수 호출로 정확히 전체 구조체 크기만큼 패킷을 송수신할 수 있다.

```c
// [클라이언트 송신 예시]
ccp_packet req;
memset(&req, 0, sizeof(ccp_packet));
req.room_id = 1;
req.usr_id = 42;
strncpy(req.nickname, "방랑자", SLICE_SIZE - 1);
strncpy(req.payload, "안녕하세요!", PAYLOAD_SIZE - 1);
req.type = CCP_TYPE_MESSAGE;

// 고정 크기 전체를 바로 송신
send(sock, &req, sizeof(ccp_packet), 0);

// [서버 수신 예시]
ccp_packet req;
recv_exact(sock, &req, sizeof(ccp_packet)); // 정확히 sizeof(ccp_packet) 만큼 수신
if (req.type == CCP_TYPE_MESSAGE) {
    // 메시지 브로드캐스트 로직 수행
}
```

---

## 2. Chaton Communication Protocol (CCP) 명세

공통 프로토콜의 C언어 구조체 및 메시지 타입 상수는 `common/include/protocol.h`에 작성하여 서버와 클라이언트가 동일하게 공유한다.
멀티캐릭터 상수(`'MSG'` 등) 사용 시 발생하는 컴파일러 경고를 방지하기 위해 헤더 최상단에 `#pragma GCC diagnostic ignored "-Wmultichar"` 지시자를 포함한다.

```c
#ifndef CHATON_PROTOCOL_H
#define CHATON_PROTOCOL_H

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wmultichar"
#endif

#include <stdint.h>

#define PAYLOAD_SIZE 1024
#define SLICE_SIZE 64
#define MAX_SLICES 16

// 메시지 타입 정의 (ccp_packet.type 값)
#define CCP_TYPE_REGISTER    'REG'  // 회원가입 요청 / 결과
#define CCP_TYPE_LOGIN       'LGN'  // 로그인 요청 / 결과
#define CCP_TYPE_ROOM_LIST   'RLL'  // 채팅방 목록 요청 / 결과
#define CCP_TYPE_ROOM_CREATE 'RCR'  // 채팅방 생성 요청 / 결과
#define CCP_TYPE_ROOM_JOIN   'RJN'  // 채팅방 입장 요청 (ID or 코드로 입장) / 결과
#define CCP_TYPE_ROOM_LEAVE  'RLV'  // 채팅방 퇴장 요청 / 결과
#define CCP_TYPE_MESSAGE     'MSG'  // 일반 채팅 메시지 송수신 (다중 채팅방 브로드캐스트)
#define CCP_TYPE_CMD         'CMD'  // Vim 스타일 명령어 입력 (: 프리픽스 제거 후 payload 전송)
#define CCP_TYPE_AI_CHAT     'AIK'  // AI 키라라 자유 대화 (/chat 프리픽스 제거 후 payload 전송)
#define CCP_TYPE_AI_CMD      'AIC'  // AI 키라라 명령 실행 (/cmd 프리픽스 제거 후 payload 전송)
#define CCP_TYPE_FILE        'FIL'  // 파일 전송 프로토콜 (시작, 승인, 청크, 알림)
#define CCP_TYPE_SYSTEM      'SYS'  // 시스템 메시지 (입장/퇴장 등의 이벤트)
#define CCP_TYPE_ERROR       'ERR'  // 에러 알림

#pragma pack(push, 1)

// === 1. 특수 용도 payload 캐스팅용 구조체 ===

// [채팅방 정보 구조체 - 정확히 64바이트, 패딩 불필요]
typedef struct {
    uint32_t room_id;    // 4 bytes
    char name[32];       // 32 bytes
    char code[24];       // 24 bytes (입장 코드, 최대 23자 + null)
    uint32_t member_count; // 4 bytes
} ccp_room_info; // size: 4 + 32 + 24 + 4 = 64 bytes



// === 2. 기본 패킷 구조체 ===

// 공통 패킷 구조체 (4바이트 정렬 보장, 1108 bytes)
typedef struct { 
    uint32_t room_id;             // 현재 있는/보내는 채팅방 ID
    int32_t status;               // 요청 처리 상태 코드 (성공: 0, 음수 에러코드)
    uint32_t usr_id;              // 유저 고유 ID (인증 전에는 0)
    char nickname[SLICE_SIZE];    // 닉네임 (DB와 대조하여 회원가입 여부 검증용)
    union {
        char msg[PAYLOAD_SIZE];                       // 일반 텍스트용 (1024 bytes)
        char slices[MAX_SLICES][SLICE_SIZE];          // 가변 인자 슬라이스 (16 * 64 = 1024 bytes)
        ccp_room_info rooms[16];                      // 방 목록 캐스팅용 (16 * 64 = 1024 bytes)
    } payload;
    uint32_t type;                // 메시지 타입 (CCP_TYPE_*)
    char flag[4];                 // 옵션 플래그 (flag[0] = is_last, 파일 전송 마지막 청크 여부)
} ccp_packet;

#pragma pack(pop)

#endif // CHATON_PROTOCOL_H
```

---

## 3. 메시지 타입별 사용법

### 3.1 세부 매핑 목록

헤더 필드(`usr_id`, `room_id`, `status` 등)를 적극 재활용하여 payload 데이터를 최대한 단순한 단일 텍스트 형태로 유지한다. 여러 개별 인자가 들어갈 때만 `payload.slices` 공용체 멤버를 통해 접근하여 사용한다.

#### 1. 인증 관련
- **회원가입 요청 (`type = CCP_TYPE_REGISTER`)**
  - 클라이언트 전송: 헤더 (`usr_id = 0`, `nickname = 원하는_닉네임`) / `payload.slices` 매핑:
    - `payload.slices[0]` = `username` (최대 63바이트)
    - `payload.slices[1]` = `password` (최대 63바이트)
    - (닉네임 정보는 헤더의 `nickname` 필드로 직접 전달하므로 슬라이스를 절약)
  - 서버 응답:
    - 성공 시: 헤더 (`status = 0`) / `payload.msg` = `""`
    - 실패 시: 헤더 (`status = 에러코드(음수)`) / `payload.msg` = `"[error_message]"`
- **로그인 요청 (`type = CCP_TYPE_LOGIN`)**
  - 클라이언트 전송: 헤더 (`usr_id = 0`) / `payload.slices` 매핑:
    - `payload.slices[0]` = `username`
    - `payload.slices[1]` = `password`
  - 서버 응답:
    - 성공 시: 헤더 (`status = 0`, `usr_id = 발급된_고유_ID`, `nickname = 유저_닉네임`) / `payload.msg` = `"[session_token]"`
    - 실패 시: 헤더 (`status = 에러코드(음수)`) / `payload.msg` = `"[error_message]"`

#### 2. 채팅방 관리
- **채팅방 목록 요청 (`type = CCP_TYPE_ROOM_LIST`)**
  - 클라이언트 전송: 헤더 (`room_id = 0`) / `payload.msg` = `""`
  - 서버 응답: 헤더 (`status = 0 또는 에러코드(음수)`) / `payload.rooms` 매핑:
    - `payload.rooms` 배열에 `ccp_room_info`에 최대 16개 정보를 직접 담아 전달한다.
    - 서버는 최대 16개의 방만 동시에 유지할 수 있다.
- **채팅방 생성 요청 (`type = CCP_TYPE_ROOM_CREATE`)**
  - 클라이언트 전송: 헤더 (`room_id = 0`) / `payload.rooms[0]` 매핑:
    - `payload.rooms[0].name` = `생성할 방 이름`
    - `payload.rooms[0].code` = `입장 코드`
  - 서버 응답:
    - 성공 시: 헤더 (`status = 생성된_room_id` (양수 ID)) / `payload.rooms[0]` 매핑:
      - `payload.rooms[0].room_id` = `생성된 방 ID`
      - `payload.rooms[0].name` = `생성된 방 이름`
      - `payload.rooms[0].code` = `설정된 입장 코드`
    - 실패 시: 헤더 (`status = 에러코드(음수)`) / `payload.msg` = `"[error_message]"`
- **채팅방 입장 요청 (`type = CCP_TYPE_ROOM_JOIN`)**
  - 클라이언트 전송: 헤더 (`room_id = 0`) / `payload.rooms[0]` 매핑:
    - `payload.rooms[0].room_id` = `입장하려는 방 ID` (숫자로 입장 시)
    - `payload.rooms[0].code` = `입장 코드` (코드로 입장 시)
  - 서버 응답:
    - 성공 시: 헤더 (`status = 최종_입장된_room_id` (양수 ID)) / `payload.rooms[0]` 매핑:
      - `payload.rooms[0].room_id` = `최종 입장된 방 ID`
      - `payload.rooms[0].name` = `입장된 방 이름`
      - `payload.rooms[0].code` = `입장 코드`
      - `payload.rooms[0].member_count` = `입장 인원수`
    - 실패 시: 헤더 (`status = 에러코드(음수)`) / `payload.msg` = `"[error_message]"`
- **채팅방 퇴장 요청 (`type = CCP_TYPE_ROOM_LEAVE`)**
  - 클라이언트 전송: 헤더 (`room_id = 퇴장할_방_ID`) / `payload.msg` = `""`
  - 서버 응답: 헤더 (`status = 0 또는 에러코드(음수)`) / `payload.msg` = `""`

#### 3. 채팅 및 명령어
- **일반 채팅 메시지 (`type = CCP_TYPE_MESSAGE`)**
  - 클라이언트 전송: 헤더 (`room_id, usr_id, nickname`) / `payload.msg` = `"[채팅 메시지 내용]"`
  - 서버 응답: 헤더 (`status = 메시지가_속한_room_id` (양수 ID), `usr_id, nickname`) / `payload.msg` = `"[채팅 메시지 내용]"` (해당 채팅방에 속한 모든 클라이언트에 브로드캐스트)
- **Vim 명령어 (`type = CCP_TYPE_CMD`)**
  - 클라이언트 전송: 헤더 (`room_id, usr_id`) / `payload.msg` = `"[명령어]"` (`:` 프리픽스를 제거한 순수 명령 문자열. 예: `:dd -l 3` → `"dd -l 3"`)
  - 서버 응답: 헤더 (`status = 0 또는 에러코드(음수)`) / `payload.msg` = `"[명령 수행 결과 메시지]"` (또는 삭제의 경우 `type = CCP_TYPE_CMD` 형태의 패킷을 전달하여 클라이언트 UI 갱신을 지시)
  - 서버에서 클라이언트에게 주는 명령은 미정
- **AI 키라라 자유 대화 (`type = CCP_TYPE_AI_CHAT`)**
  - 클라이언트 전송: 헤더 (`room_id, usr_id`) / `payload.msg` = `"[질문 텍스트]"` (`/chat` 프리픽스를 제거한 순수 질문 문자열. 예: `/chat 오늘 날씨 어때?` → `"오늘 날씨 어때?"`)
  - 서버 응답: 헤더 (`status = 0 또는 에러코드(음수), usr_id = 0, nickname = "키라라"`) / `payload.msg` = `"[키라라의 최종 답변 텍스트]"` (서버가 Oracle AI 미들웨어를 거쳐 최종 응답을 키라라의 `usr_id = 0`, `nickname = 키라라` 패킷으로 전송)
- **AI 키라라 명령 실행 (`type = CCP_TYPE_AI_CMD`)**
  - 클라이언트 전송: 헤더 (`room_id, usr_id`) / `payload.msg` = `"[명령 텍스트]"` (`/cmd` 프리픽스를 제거한 순수 명령 문자열. 예: `/cmd 나히다가 보낸 메시지 다 찾아줘` → `"나히다가 보낸 메시지 다 찾아줘"`)
  - 서버 응답: 헤더 (`status = 0 또는 에러코드(음수), usr_id = 0, nickname = "키라라"`) / `payload.msg` = `"[키라라의 최종 답변 텍스트]"` (도구 호출 실행 결과 포함)

#### 4. 파일 전송 (`type = CCP_TYPE_FILE`)
 - 미정

#### 5. 시스템 및 에러
- **시스템 메시지 (`type = CCP_TYPE_SYSTEM`)**
  - 서버가 브로드캐스트: 헤더 (`status = 현재_방_ID` (양수 ID)) / `payload.msg` = `"[이벤트 메시지]"`
- **에러 알림 (`type = CCP_TYPE_ERROR`)**
  - 서버 응답: 헤더 (`status = 에러코드(음수)`) / `payload.msg` = `"[error_message]"`

---

## 4. 제3자 클라이언트 및 서버 상호 호환성 규약

CCP(Chaton Communication Protocol)는 동일한 학기/과제 주제를 진행하는 타사(타 팀)의 클라이언트 및 서버와 매끄럽게 통신 호환을 이루는 것을 지향한다. 이를 위해 외부 개발자가 제작한 프로그램과 통신할 때 반드시 다음 호환성 수칙을 준수해야 한다.

### 4.1 아키텍처 및 정렬 무관성 준수
- **1바이트 패딩 정렬 강제**: 서로 다른 OS 및 컴파일러 아키텍처에서 컴파일하더라도 구조체의 바이트 레이아웃이 일치해야 한다. 반드시 `#pragma pack(push, 1)`을 사용하여 고정 크기 패킷의 총 바이트 크기가 정확히 `1108 바이트`가 되도록 강제해야 한다.
  - `ccp_packet` 크기 계산: `room_id(4)` + `status(4)` + `usr_id(4)` + `nickname(64)` + `payload(1024)` + `type(4)` + `flag(4)` = `1108 bytes`

### 4.2 네트워크 바이트 오더링 (엔디안 호환성)
- **정수 필드 인코딩**: 4바이트 정수 필드인 `room_id`, `status`, `usr_id`, `type`은 전송 직전 `htonl()`을 통해 빅 엔디안(Network Byte Order)으로 변환하여 전송하고, 수신 시에는 `ntohl()`을 통해 호스트 엔디안으로 복원해야 한다. 

### 4.3 구조체 멤버 필드 정렬 및 슬라이스 패딩 규격
- **인코딩 표준**: 모든 닉네임 및 패킷 내부의 문자열 데이터는 **UTF-8**을 표준 인코딩으로 삼는다.
- **슬라이스 내 문자열 채우기 & 널 종료**: 가변 인자를 64바이트 슬라이스(`slices[i]`)에 담을 때, 문자열 복사 함수(`strncpy` 등)를 사용하여 최대 63바이트 크기를 초과하지 않도록 제한하고, 항상 마지막 64번째 바이트는 널 문자(`\0`)로 종단되도록 보장한다.
- **패킷 전체 패딩**: 패킷 송신 전 버퍼 전체를 0으로 채우는 것(`memset`)을 권장하며, 사용하지 않는 슬라이스 영역은 항상 0(`\0`)으로 가득 찬 채 전송되도록 관리한다.

### 4.4 게스트 유저(미인증 클라이언트) 수용 규칙
- **회원 정보가 일치하지 않는 외부 클라이언트가 직접 로그인 없이 서버로 연결하여 메시지를 보내는 경우**:
  - 서버는 연결을 거부하지 않고, **임시 ID 20000(Guest ID)**를 강제로 발급한다.
  - 외부 클라이언트는 자신이 설정한 닉네임 뒤에 `(Guest)`가 강제로 덧붙여진 패킷을 전달받게 된다.
  - 이 게스트 유저가 일반 채팅 메시지 패킷(`type = CCP_TYPE_MESSAGE`)을 보내면, 서버는 요청 헤더의 `room_id` 값을 파싱해 해당 채팅방에 참여 중인 모든 사용자(타 팀원 포함)에게 성공적으로 브로드캐스트하여 호환 채팅이 가능하도록 처리한다.
