# Contributing to LogSystem MB

Obrigado pelo interesse em contribuir! Este documento fornece diretrizes para contribuições.

## ?? Como Contribuir

### Reportar Bugs

Abra uma [issue](https://github.com/GuilhermeCandiotto/LogSystemMB/issues) com:

- **Título descritivo**
- **Versão** do LogSystem
- **Sistema operacional** e versão
- **Passos para reproduzir**
- **Comportamento esperado vs. atual**
- **Logs ou screenshots** (se aplicável)

**Template de Bug Report:**
```markdown
**Versão:** v2.0
**OS:** Windows 10 64-bit
**Compilador:** MSVC 2022

**Descrição:**
[Descreva o bug]

**Passos para reproduzir:**
1. ...
2. ...

**Esperado:**
[O que deveria acontecer]

**Atual:**
[O que está acontecendo]

**Logs:**
```
[Cole logs relevantes]
```
```

---

### Sugerir Features

Abra uma [issue](https://github.com/GuilhermeCandiotto/LogSystemMB/issues) com:

- **Título**: `[FEATURE] Nome da feature`
- **Descrição** detalhada
- **Caso de uso** (por que é necessário?)
- **Proposta de implementação** (se tiver)

---

### Pull Requests

1. **Fork** o repositório
2. **Crie uma branch** para sua feature:
   ```bash
   git checkout -b feature/minha-feature
   ```
3. **Faça commits** semânticos:
   ```bash
   git commit -m "feat: adiciona suporte a JSON logging"
   git commit -m "fix: corrige memory leak no string pool"
   git commit -m "docs: atualiza README com exemplos"
   ```
4. **Push** para seu fork:
   ```bash
   git push origin feature/minha-feature
   ```
5. **Abra um Pull Request** com:
   - Título descritivo
   - Descrição detalhada das mudanças
   - Testes realizados
   - Screenshots (se aplicável)

---

## ?? Convenções de Código

### Estilo de Código

Seguimos o estilo do código existente:

```cpp
// Naming
class MyClass { };              // PascalCase para classes
void MyFunction() { }            // PascalCase para funções
int myVariable = 0;              // camelCase para variáveis
constexpr int MY_CONSTANT = 42;  // UPPER_CASE para constantes

// Indentação
void MyFunction() {
    if (condition) {
        // 4 espaços
        DoSomething();
    }
}

// Brackets
if (condition) {   // Opening brace na mesma linha
    // ...
}                   // Closing brace em linha própria

// Namespaces
namespace WYD_Server {
    // ...
} // namespace WYD_Server
```

### Comentários

```cpp
// Comentários de linha única para explicações breves

/**
 * Comentários de múltiplas linhas para:
 * - Funções públicas
 * - Classes
 * - Algoritmos complexos
 */

// TODO: Marcar tarefas futuras
// FIXME: Marcar bugs conhecidos
// NOTE: Notas importantes
```

### Includes

```cpp
// 1. Header do próprio arquivo
#include "LogSystem.h"

// 2. Headers do projeto
#include "OtherClass.h"

// 3. Headers de bibliotecas externas
#include <zlib.h>

// 4. Headers da STL
#include <string>
#include <vector>

// 5. Headers do sistema
#include <windows.h>
```

---

## ?? Testes

Antes de submeter um PR:

1. **Compile sem warnings**:
   ```bash
   # MSVC
   cl /W4 /WX ...
   
   # GCC/Clang
   g++ -Wall -Wextra -Werror ...
   ```

2. **Execute testes manuais**:
   - Teste funcionalidade adicionada
   - Teste casos extremos
   - Teste em modo Debug e Release

3. **Verifique memory leaks** (se possível):
   - Visual Studio: Diagnostic Tools
   - Valgrind (Linux)

4. **Execute benchmarks** (para mudanças de performance):
   ```bash
   .\benchmark.exe
   ```

---

## ?? Estrutura de Diretórios

```
LogSystemMB/
??? docs/                 # Documentação
?   ??? ARCHITECTURE.md
?   ??? API_REFERENCE.md
?   ??? INSTALLATION.md
??? Config/               # Arquivos de configuração
?   ??? logconfig.ini
??? Log/                  # Logs gerados (git ignored)
??? LogSystem.h          # Header principal
??? LogSystem.cpp        # Implementação
??? main.cpp             # Exemplo de uso (GUI)
??? benchmark.cpp        # Benchmarks de performance
??? CMakeLists.txt       # Build system
??? README.md            # Documentação principal
??? CONTRIBUTING.md      # Este arquivo
??? CHANGELOG.md         # Histórico de versões
??? LICENSE.txt          # Licença AGPL-3.0
```

---

## ?? Áreas que Precisam de Contribuição

### Alta Prioridade
- [ ] Testes unitários (GoogleTest)
- [ ] Testes de integração
- [ ] Suporte Linux/Mac
- [ ] JSON logging
- [ ] Documentação em inglês

### Média Prioridade
- [ ] Plugins (Elasticsearch, Splunk)
- [ ] Web dashboard
- [ ] Lock-free queue MPMC
- [ ] Filtros dinâmicos na GUI

### Baixa Prioridade
- [ ] Temas customizáveis
- [ ] Log replay system
- [ ] AI anomaly detection

---

## ?? Processo de Review

1. **Automated checks** (GitHub Actions)
   - Build em múltiplas plataformas
   - Testes unitários (quando implementados)
   - Code linting

2. **Manual review**
   - Code quality
   - Performance implications
   - Thread safety
   - Documentação

3. **Approval & Merge**
   - Pelo menos 1 aprovação de mantenedor
   - Squash merge para manter histórico limpo

---

## ?? Comunicação

- **Issues**: Para bugs e features
- **Pull Requests**: Para código
- **Discussões**: Para dúvidas gerais

---

## ?? Licença

Ao contribuir, você concorda que suas contribuições serão licenciadas sob a [GNU AGPL-3.0](LICENSE.txt).

---

## ?? Reconhecimento

Contribuidores serão listados em:
- README.md (seção "Contributors")
- CHANGELOG.md (para cada release)

---

**Obrigado por contribuir!** ??
