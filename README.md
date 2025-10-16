# CP2 - Sistema de Dados Robusto
## Sistemas de Tempo Real - FIAP 2025

### Informações do Aluno
- **Nome:** Ricardo Fogaca
- **RM:** 86603
- **Sala:** 5ECS

### Descrição do Projeto
Sistema multitarefa implementado com FreeRTOS para ESP32, dividido em três módulos principais:

#### Módulo 1: Geração de Dados
- Produz valores inteiros sequenciais continuamente
- Envia cada valor para uma fila de comunicação
- Caso a fila esteja cheia, o valor é descartado

#### Módulo 2: Recepção de Dados
- Recebe os valores da fila e os transmite (exibe no terminal)
- Usa alocação de memória dinâmica (malloc e free)
- Implementa reação escalonada a timeouts:
  - 10s: Recuperação leve
  - 20s: Recuperação moderada (limpa fila)
  - 30s: Recuperação severa (reinicia sistema)

#### Módulo 3: Supervisão
- Monitora o funcionamento dos outros módulos
- Exibe status usando flags de controle

#### Watchdog Timer
- Configurado para monitorar as tarefas críticas
- Timeout de 5 segundos
- Reinicia o dispositivo em caso de travamento

### Execução
![Print da Execução](screenshot.png)

### Estrutura do Projeto
```
cp2_ricardo_fogaca/
├── main/
│   ├── main.c
│   └── CMakeLists.txt
├── CMakeLists.txt
└── README.md
```

### Como Compilar e Executar
```bash
idf.py set-target esp32
idf.py build
idf.py -p COM5 flash monitor
```

### Requisitos
<img width="886" height="601" alt="image" src="https://github.com/user-attachments/assets/e894dd48-0b7b-46d8-88ed-b435a40fd273" />
<img width="886" height="598" alt="image" src="https://github.com/user-attachments/assets/3956b2f9-fdda-4966-a267-846f9e35c629" />
<img width="886" height="381" alt="image" src="https://github.com/user-attachments/assets/716fc95a-c4f3-47d6-a428-ff46205ba96a" />
<img width="886" height="313" alt="image" src="https://github.com/user-attachments/assets/75b183c5-81b5-4f00-9c79-8e6d6921e2cf" />
<img width="886" height="415" alt="image" src="https://github.com/user-attachments/assets/62d38e93-2d38-4450-ade6-1003cd954f83" />







- ESP-IDF v5.4.2
- ESP32
- FreeRTOS
