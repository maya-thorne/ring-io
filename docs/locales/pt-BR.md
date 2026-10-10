<div align="center">

<p align="center">
  <a href="../../README.md">🇺🇸 English</a> · <a href="./ko.md">🇰🇷 한국어</a> · <a href="./zh-CN.md">🇨🇳 中文</a> · <a href="./es.md">🇪🇸 Español</a> · <a href="./hi.md">🇮🇳 हिन्दी</a><br />
  <a href="./ar.md">🇸🇦 العربية</a> · <strong>🇧🇷 Português</strong> · <a href="./ru.md">🇷🇺 Русский</a> · <a href="./fr.md">🇫🇷 Français</a> · <a href="./id.md">🇮🇩 Bahasa Indonesia</a>
</p>

<p align="center">
  <img src="../assets/locales/pt-BR/banner.svg" alt="ring-io banner (pt-BR)" width="100%" />
</p>

# ⚡ ring-io

**Motor de E/S Assíncrono de Ultra Alto Desempenho via Linux io_uring (C23)**

[![CI](https://github.com/maya-thorne/ring-io/actions/workflows/ci.yml/badge.svg)](https://github.com/maya-thorne/ring-io/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](../../LICENSE)
[![Standards: C23](https://img.shields.io/badge/Standard-C23-green.svg)](https://en.cppreference.com/w/c/23)
[![Throughput](https://img.shields.io/badge/Throughput-2.85%20Mops%2Fsec-brightgreen.svg)](#성능-벤치마크-결과)

<sub>Projetado por **[Maya Thorne](https://github.com/maya-thorne)** · Arquiteta de Sistemas & Kernel Linux</sub>

</div>

---

## 📌 Visão Geral da Arquitetura

`ring-io` é um runtime de E/S assíncrono bare-metal projetado para eliminar o overhead de chamadas de sistema dos loops tradicionais (`epoll`/`kqueue`). Mapeando buffers circulares de submissão (SQ) e conclusão (CQ) no espaço do usuário via `io_uring`, obtém alto rendimento com zero cópia.

```mermaid
flowchart LR
    subgraph Userspace ["Aplicação em Espaço de Usuário"]
        App["Loop de Eventos / Thread de Trabalho"]
        SQ["Fila de Submissão (Anel SQ)"]
        CQ["Fila de Conclusão (Anel CQ)"]
    end

    subgraph Kernel ["Kernel Linux (5.10+)"]
        KernelThread["Thread de Kernel io_uring (SQPOLL)"]
        Storage["NVMe / Malha de Sockets Diretos"]
    end

    App -->|Inserir Entrada| SQ
    SQ -->|Polling do Kernel / Sem Syscall| KernelThread
    KernelThread -->|E/S de Cópia Zero| Storage
    Storage -->|Publicar Conclusão| CQ
    CQ -->|Colheita em Lote| App
```

### Recursos de Engenharia

- **Polling sem chamadas de sistema (`IORING_SETUP_SQPOLL`)**: Thread do kernel elimina latência de `enter()`.
- **Alinhamento de linha de cache**: Estruturas SQ/CQ alinhadas a 64 bytes para evitar falso compartilhamento NUMA.
- **Coleta em lote de eventos CQ**: Coleta até 64 eventos completados em uma única varredura.
- **Primitivas C23 Modernas**: Operações atômicas de largura fixa e otimizações (`-flto -O3 -march=native`).

---

## 📊 Resultados de Desempenho

Benchmark executando **1.000.000 de operações assíncronas** com lote 64 frente ao `epoll`:

| 지표 (Metric) | 레거시 `epoll` | `ring-io` (io_uring Default) | `ring-io` (SQPOLL Enabled) | 가속비 (Speedup) |
| :--- | :---: | :---: | :---: | :---: |
| **초당 처리량 (Throughput)** | 0.82 Mops/s | 2.14 Mops/s | **2.85 Mops/s** | **+247%** |
| **평균 지연 시간 (Latency)** | 1,219 ns | 467 ns | **350 ns** | **-71.2%** |
| **시스템 콜 발생 빈도** | 1 syscall / op | 1 syscall / 64 ops | **0 syscalls** (Steady) | **완전 우회 (Bypassed)** |
| **L1-D CPU 캐시 미스율** | 4.2% | 1.1% | **0.6%** | **-85.7%** |

---

## 🛠️ Início Rápido

### Pré-requisitos
- Kernel Linux 5.10+ (6.x recomendado)
- `clang` ou `gcc` com suporte C23
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

## 💻 Exemplo de Código C23

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

## 📜 Licença

Licensed under the [MIT License](../../LICENSE). Copyright © 2026 Maya Thorne.
