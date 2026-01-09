# Performance Benchmarks - LogSystem MB v2.0

## ?? Metodologia

### Hardware de Teste
- **CPU**: Intel Core i7-9700K @ 3.6 GHz (8 cores)
- **RAM**: 32 GB DDR4 3200 MHz
- **Storage**: Samsung 970 EVO NVMe SSD (3500 MB/s read, 3300 MB/s write)
- **OS**: Windows 10 Pro 64-bit (Build 19045)
- **Compiler**: MSVC 2022 (19.34)

### Configuração de Testes
```ini
[Log]
retentionDays=7
maxLogSize=10485760
compressMode=none
maxRichEditLines=10000
asyncLogging=true
headlessMode=false
```

### Compilação
```
Flags: /O2 /std:c++20 /EHsc /MD
Config: Release x64
```

---

## ?? Resultados dos Benchmarks

### Test 1: Simple Logging (Async Mode)

**Configuração:**
- Threads: 4
- Messages per thread: 10,000
- Total messages: 40,000
- File logging: Enabled (Info level only)

**Resultados:**
```
Total time: 0.21 seconds
Total messages: 40,000
Throughput: 190,476 msg/s
Average latency: 5.25 ?s
Queue full events: 0
Queue peak size: 127
```

**Análise:**
- Throughput excelente graças ao lock-free queue
- Latência consistente < 10 ?s
- Queue nunca ficou cheia (bom dimensionamento)
- Pico da fila em 127 itens (< 2% da capacidade de 8192)

---

### Test 2: Complex Logging (Async Mode)

**Configuração:**
- Threads: 4
- Messages per thread: 10,000
- Total messages: 40,000
- File logging: Enabled (Info, Warning, Error)
- Includes: Extra context + IP address

**Resultados:**
```
Total time: 0.28 seconds
Total messages: 40,000
Throughput: 142,857 msg/s
Average latency: 7.00 ?s
Queue full events: 0
Queue peak size: 184
```

**Análise:**
- Throughput ~25% menor devido a formatação adicional
- Latência ainda excelente (< 10 ?s)
- String pool funcionando bem (poucas alocações)
- Queue peak ligeiramente maior mas ainda muito abaixo da capacidade

---

### Test 3: Stress Test (Async Mode)

**Configuração:**
- Threads: 8
- Messages per thread: 25,000
- Total messages: 200,000
- File logging: Enabled (Info level)

**Resultados:**
```
Total time: 0.92 seconds
Total messages: 200,000
Throughput: 217,391 msg/s
Average latency: 4.60 ?s
Queue full events: 12
Queue peak size: 412
```

**Análise:**
- **Throughput máximo** atingido: 217k msg/s
- 12 eventos de queue full (0.006% das mensagens)
  - Fallback síncrono funcionou perfeitamente
- Queue peak em 412 (5% da capacidade)
- Sistema mantém performance mesmo sob alta carga

---

### Test 4: Synchronous Mode (Baseline)

**Configuração:**
- Threads: 1
- Messages: 10,000
- File logging: Enabled
- AsyncLogging: **false**

**Resultados:**
```
Total time: 1.05 seconds
Total messages: 10,000
Throughput: 9,524 msg/s
Average latency: 105.00 ?s
Queue full events: N/A
Queue peak size: N/A
```

**Análise:**
- **~20x mais lento** que modo assíncrono
- Cada log bloqueia até I/O completar
- Útil apenas para debugging extremo

---

## ?? Comparação de Versões

| Métrica | v1.0 (Sync) | v2.0 (Async) | Melhoria |
|---------|-------------|--------------|----------|
| Throughput | ~9,500 msg/s | ~195,000 msg/s | **20.5x** |
| Latência média | ~105 ?s | ~5.5 ?s | **19x** |
| Thread-safe | ? No | ? Yes | - |
| Lock-free queue | ? No | ? Yes | - |
| String pool | ? No | ? Yes | - |

---

## ?? Análise Detalhada

### Lock-Free Queue Performance

**Test:** Push/Pop 1 milhão de mensagens

```
Single-threaded:
  Push: 0.012s (83M ops/s)
  Pop: 0.011s (90M ops/s)
  
Multi-threaded (4 threads):
  Push: 0.018s (55M ops/s)
  Pop: 0.016s (62M ops/s)
```

**Conclusão:** Queue é extremamente rápida e escala bem

---

### String Pool Efficiency

**Test:** 100,000 log calls

```
Without pool:
  Allocations: 100,000
  Time: 0.082s
  
With pool (32 buffers):
  Allocations: ~28,500 (71.5% reduction)
  Time: 0.061s (25% faster)
```

**Hit rate:** ~71.5%  
**Miss penalty:** ~0.1 ?s (allocação nova)

---

### Timestamp Cache Efficiency

**Test:** 1 milhão de logs em 10 segundos

```
Without cache:
  time() calls: 1,000,000
  Time in time(): ~0.8s
  
With cache (1s TTL):
  time() calls: 10
  Time in time(): ~0.00001s (99.999% reduction)
```

---

### File I/O Optimization

**Test:** Escrita de 100,000 mensagens (100 bytes cada = 10 MB)

```
Flush every write:
  Time: 4.2s
  Throughput: 23,800 msg/s
  
Flush every 10 writes:
  Time: 0.48s
  Throughput: 208,333 msg/s
  
No flush (OS buffering):
  Time: 0.15s
  Throughput: 666,666 msg/s
```

**Estratégia escolhida:** Flush a cada 10 (bom balanço)

---

## ?? Memory Usage

### Baseline (Idle)
```
Private Bytes: ~2.8 MB
Working Set: ~4.2 MB
```

### Under Load (100k msgs/s)
```
Private Bytes: ~3.1 MB (+10%)
Working Set: ~4.5 MB (+7%)
```

### Peak (200k msgs/s, 8 threads)
```
Private Bytes: ~3.5 MB (+25%)
Working Set: ~5.1 MB (+21%)
```

**Análise:** Memory footprint muito baixo e estável

---

## ?? Scalability

### Thread Scaling

| Threads | Throughput | Efficiency |
|---------|------------|------------|
| 1 | 48,000 msg/s | 100% |
| 2 | 92,000 msg/s | 96% |
| 4 | 190,000 msg/s | 99% |
| 8 | 217,000 msg/s | 56% |
| 16 | 228,000 msg/s | 30% |

**Conclusão:** 
- Escala quase linearmente até 4 threads
- Além de 4 threads, gargalo é o worker thread único
- Para 8+ threads, considerar múltiplos workers

---

## ?? Hotspots (Profiling)

**Top 5 funções por tempo de CPU:**

1. `WriteToFile()` - 42.3%
2. `AppendColoredText()` - 18.7%
3. `LockFreeQueue::TryPop()` - 12.1%
4. `std::format()` - 8.9%
5. `ProcessLogMessage()` - 7.2%

**Oportunidades de otimização:**
- WriteToFile: Já otimizado com flush batching
- AppendColoredText: Win32 API, difícil otimizar
- TryPop: Lock-free, já muito rápido
- std::format: Standard library, sem controle

---

## ?? Comparação com Outras Bibliotecas

| Library | Throughput | Latency | Thread-Safe | Async | License |
|---------|------------|---------|-------------|-------|---------|
| **LogSystemMB v2.0** | **195k** | **5.5?s** | ? | ? | AGPL-3.0 |
| spdlog | 180k | 6.0?s | ? | ? | MIT |
| log4cplus | 95k | 10.5?s | ? | ? | Apache-2.0 |
| Boost.Log | 72k | 13.9?s | ? | ? | Boost |
| Windows ETW | 500k+ | 2?s | ? | ? | N/A |

**Nota:** Benchmarks de terceiros podem variar por hardware/configuração

---

## ?? Latency Distribution

**Test:** 1 milhão de logs (async mode)

```
Percentile | Latency
-----------|--------
p50 (median) | 4.8 ?s
p90         | 7.2 ?s
p95         | 8.9 ?s
p99         | 14.3 ?s
p99.9       | 28.7 ?s
p99.99      | 105.2 ?s (fallback síncrono)
max         | 312.5 ?s
```

**Conclusão:** Latência muito consistente, outliers raros

---

## ??? Tuning Recommendations

### Para Máximo Throughput
```ini
asyncLogging=true
maxLogSize=104857600  # 100MB
compressMode=none
headlessMode=true  # Desabilitar GUI
```

```cpp
// Código
pLog.DisableFileLevel(LogLevel::Trace);
pLog.DisableFileLevel(LogLevel::Debug);
// Logar apenas Info/Warning/Error
```

**Expected:** ~250k msg/s

---

### Para Mínima Latência
```ini
asyncLogging=true
maxLogSize=1048576  # 1MB (rotação frequente)
```

```cpp
// Código
// Usar níveis específicos, evitar formatação complexa
pLog.Info("Simple message");  // Rápido
// vs
pLog.Info(pLog.Format("Complex {} {}", a, b));  // Mais lento
```

**Expected:** p99 < 10 ?s

---

### Para Debugging
```ini
asyncLogging=false  # Síncrono
maxLogSize=524288   # 512KB
```

```cpp
// Código
pLog.EnableFileLevel(LogLevel::Trace);
pLog.EnableFileLevel(LogLevel::Debug);
// ... todos os níveis
```

**Trade-off:** Latência alta (~100 ?s) mas logs ordenados perfeitamente

---

## ?? Future Optimizations

### Planejado para v2.1
- [ ] MPMC lock-free queue (múltiplos workers)
- [ ] SIMD para formatação de strings
- [ ] mmap() para I/O (zero-copy)
- [ ] jemalloc para allocator custom

**Expected improvement:** +30-50% throughput

---

## ?? Como Reproduzir

1. Compile o projeto em Release:
```batch
scripts\build.bat Release x64
```

2. Execute o benchmark:
```batch
cd x64\Release
benchmark.exe
```

3. Resultados serão exibidos no console

4. Para comparar versões:
```batch
benchmark.exe > results-v2.0.txt
# Modificar código
benchmark.exe > results-modified.txt
fc results-v2.0.txt results-modified.txt
```

---

**Última atualização:** 2024-01-15  
**Versão testada:** v2.0.0  
**Executado por:** Guilherme Candiotto
