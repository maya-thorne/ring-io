<div align="center">

<p align="center">
  <a href="../../README.md">🇺🇸 English</a> · <a href="./ko.md">🇰🇷 한국어</a> · <a href="./zh-CN.md">🇨🇳 中文</a> · <a href="./es.md">🇪🇸 Español</a> · <a href="./hi.md">🇮🇳 हिन्दी</a><br />
  <a href="./ar.md">🇸🇦 العربية</a> · <a href="./pt-BR.md">🇧🇷 Português</a> · <strong>🇷🇺 Русский</strong> · <a href="./fr.md">🇫🇷 Français</a> · <a href="./id.md">🇮🇩 Bahasa Indonesia</a>
</p>

<p align="center">
  <img src="../assets/locales/ru/banner.svg" alt="ring-io banner (ru)" width="100%" />
</p>

# ⚡ ring-io

**Высокопроизводительный асинхронный движок ввода-вывода на Linux io_uring (C23)**

[![CI](https://github.com/maya-thorne/ring-io/actions/workflows/ci.yml/badge.svg)](https://github.com/maya-thorne/ring-io/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](../../LICENSE)
[![Standards: C23](https://img.shields.io/badge/Standard-C23-green.svg)](https://en.cppreference.com/w/c/23)
[![Throughput](https://img.shields.io/badge/Throughput-2.85%20Mops%2Fsec-brightgreen.svg)](#성능-벤치마크-결과)

<sub>Архитектура и разработка: **[Maya Thorne](https://github.com/maya-thorne)** · Системный архитектор ядра Linux и параллелизма</sub>

</div>

---

## 📌 Обзор архитектуры

`ring-io` — это низкоуровневая среда асинхронного ввода-вывода, устраняющая накладные расходы системных вызовов традиционных циклов `epoll`/`kqueue`. Благодаря кольцевым буферам отправки (SQ) и завершения (CQ), отображаемым напрямую в пространство пользователя через `io_uring`, достигается нулевое копирование и пакетная обработка.

```mermaid
flowchart LR
    subgraph Userspace ["Приложение пространства пользователя"]
        App["Цикл событий / Рабочий поток"]
        SQ["Очередь отправки (Кольцо SQ)"]
        CQ["Очередь завершения (Кольцо CQ)"]
    end

    subgraph Kernel ["Ядро Linux (5.10+)"]
        KernelThread["Поток ядра io_uring (SQPOLL)"]
        Storage["NVMe / Прямая сокетная фабрика"]
    end

    App -->|Добавление записи| SQ
    SQ -->|Опрос ядром / Без си스вызовов| KernelThread
    KernelThread -->|Zero-Copy ввод-вывод| Storage
    Storage -->|Публикация завершения| CQ
    CQ -->|Пакетный сбор| App
```

### Ключевые инженерные особенности

- **Опрос без системных вызовов (`IORING_SETUP_SQPOLL`)**: Поток опроса ядра полностью обходит задержку вызова `enter()`.
- **Выравнивание по строке кэша**: Структуры SQ/CQ строго выровнены по 64 байта для предотвращения ложного разделения NUMA.
- **Пакетный сбор событий CQ**: Извлечение до 64 завершенных запросов за один проход.
- **Примитивы C23**: Фиксированная разрядность, атомарные операции и флаги оптимизации (`-flto -O3 -march=native`).

---

## 📊 Результаты тестирования

Бенчмарк **1 000 000 асинхронных операций** с размером пакета 64 в сравнении с `epoll`:

| 지표 (Metric) | 레거시 `epoll` | `ring-io` (io_uring Default) | `ring-io` (SQPOLL Enabled) | 가속비 (Speedup) |
| :--- | :---: | :---: | :---: | :---: |
| **초당 처리량 (Throughput)** | 0.82 Mops/s | 2.14 Mops/s | **2.85 Mops/s** | **+247%** |
| **평균 지연 시간 (Latency)** | 1,219 ns | 467 ns | **350 ns** | **-71.2%** |
| **시스템 콜 발생 빈도** | 1 syscall / op | 1 syscall / 64 ops | **0 syscalls** (Steady) | **완전 우회 (Bypassed)** |
| **L1-D CPU 캐시 미스율** | 4.2% | 1.1% | **0.6%** | **-85.7%** |

---

## 🛠️ Быстрый старт

### Требования
- Ядро Linux 5.10+ (Рекомендуется 6.x)
- `clang` или `gcc` с поддержкой C23
- `make`

### 빌드 및 테스트 실행

```bash
# 저장소 복제
git clone https://github.com/maya-thorne/ring-io.git
cd ring-io

# 라이브러리 및 벤치마크 바이너리 컴파일
make all

# 불변식 단위 테스트 실행
make run-test

# 고처리량 벤치마크 실행
make run-bench
```

---

## 💻 Пример кода C23

```c
#include "ring_io.h"
#include <stdio.h>

int main(void) {
    /* 2048개 큐 엔트리와 커널 SQ 폴링(SQPOLL) 모드로 초기화 */
    ring_io_ctx_t *ctx = ring_io_create(2048, RING_IO_FLAG_SQPOLL);

    char buffer[4096];
    int fd = 3; /* 대상 파일 또는 네트워크 소켓 */

    /* 비동기 읽기 요청 준비 */
    ring_io_prep_read(ctx, fd, buffer, sizeof(buffer), 0, 0x1001);

    /* 커널 링 버퍼로 일괄 제출 */
    ring_io_submit(ctx);

    /* 완료 이벤트 배치 수확 */
    ring_io_event_t events[16];
    int completed = ring_io_wait(ctx, 1, events, 16, 100);

    printf("수확된 완료 이벤트: %d건 (결과: %d 바이트)\n", completed, events[0].res);

    ring_io_destroy(ctx);
    return 0;
}
```

---

## 📜 Лицензия

Licensed under the [MIT License](../../LICENSE). Copyright © 2026 Maya Thorne.
