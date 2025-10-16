/*
 * CP 2 - Sistema de Dados Robusto
 * Sistemas de Tempo Real - FIAP 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_task_wdt.h"
#include "esp_system.h"

// ======== CONFIGURAÇÕES - ALTERE AQUI ========
#define NOME_ALUNO "Ricardo Fogaca"
#define RM_ALUNO "86603"
// ==============================================

#define TAG_PREFIX "{" NOME_ALUNO "-RM:" RM_ALUNO "}"
#define QUEUE_SIZE 10
#define TIMEOUT_LEVE 10
#define TIMEOUT_MODERADO 20
#define TIMEOUT_SEVERO 30

// Estrutura de flags de status das tarefas
typedef struct {
    bool task1_ativa;
    bool task2_ativa;
    uint32_t task1_contador;
    uint32_t task2_contador;
} StatusFlags;

// Variáveis globais
QueueHandle_t fila = NULL;
StatusFlags status_flags = {false, false, 0, 0};

// ======== MÓDULO 1: GERAÇÃO DE DADOS ========
void Task1_Geracao(void *pv)
{
    int value = 0;
    status_flags.task1_ativa = true;
    
    printf("%s [TASK1] Módulo de Geração iniciado\n", TAG_PREFIX);
    
    while(1)
    {
        // Tenta enviar para a fila (sem bloquear)
        if(xQueueSend(fila, &value, 0) != pdTRUE)
        {
            printf("%s [TASK1] Fila cheia - valor %d descartado\n", TAG_PREFIX, value);
        }
        else 
        {
            printf("%s [TASK1] Valor %d enviado com sucesso\n", TAG_PREFIX, value);
            value++;
        }
        
        status_flags.task1_contador++;
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ======== MÓDULO 2: RECEPÇÃO DE DADOS ========
void Task2_Recepcao(void *pv)
{
    int timeout = 0;
    status_flags.task2_ativa = true;
    
    printf("%s [TASK2] Módulo de Recepção iniciado\n", TAG_PREFIX);
    
    while(1)
    {
        // Aloca memória dinamicamente para o valor
        int *valor_recebido = (int *)malloc(sizeof(int));
        
        if(valor_recebido == NULL)
        {
            printf("%s [TASK2] ERRO - Falha ao alocar memória!\n", TAG_PREFIX);
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        
        // Tenta receber da fila
        if (xQueueReceive(fila, valor_recebido, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            timeout = 0;
            printf("%s [TASK2] Dado recebido: %d (transmitindo...)\n", TAG_PREFIX, *valor_recebido);
            printf("%s [TASK2] Memória alocada: %p, liberando...\n", TAG_PREFIX, (void*)valor_recebido);
            
            // Libera a memória após uso
            free(valor_recebido);
            valor_recebido = NULL;
            
            status_flags.task2_contador++;
        }
        else
        {
            // Libera memória mesmo em caso de timeout
            free(valor_recebido);
            valor_recebido = NULL;
            
            timeout++;
            
            if(timeout == TIMEOUT_LEVE)
            {
                printf("%s [TASK2] TIMEOUT - Recuperação leve (esperando dados...)\n", TAG_PREFIX);
            }
            else if (timeout == TIMEOUT_MODERADO)
            {
                printf("%s [TASK2] TIMEOUT - Recuperação moderada (limpando fila)\n", TAG_PREFIX);
                xQueueReset(fila);
            }
            else if (timeout >= TIMEOUT_SEVERO)
            {
                printf("%s [TASK2] TIMEOUT - Recuperação severa (reiniciando sistema)\n", TAG_PREFIX);
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            }
        }
        
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ======== MÓDULO 3: SUPERVISÃO ========
void Task3_Supervisao(void *pv)
{
    uint32_t contador_task1_anterior = 0;
    uint32_t contador_task2_anterior = 0;
    
    printf("%s [SUPERVISÃO] Módulo de Supervisão iniciado\n", TAG_PREFIX);
    vTaskDelay(pdMS_TO_TICKS(2000)); // Aguarda inicialização
    
    while(1)
    {
        printf("%s [SUPERVISÃO] ========== STATUS DO SISTEMA ==========\n", TAG_PREFIX);
        
        // Verifica Task1
        if(status_flags.task1_ativa)
        {
            if(status_flags.task1_contador > contador_task1_anterior)
            {
                printf("%s [SUPERVISÃO] ✓ Task1 (Geração): ATIVA - %lu operações\n", 
                       TAG_PREFIX, status_flags.task1_contador);
                contador_task1_anterior = status_flags.task1_contador;
            }
            else
            {
                printf("%s [SUPERVISÃO] ✗ Task1 (Geração): TRAVADA!\n", TAG_PREFIX);
            }
        }
        else
        {
            printf("%s [SUPERVISÃO] ✗ Task1 (Geração): INATIVA\n", TAG_PREFIX);
        }
        
        // Verifica Task2
        if(status_flags.task2_ativa)
        {
            if(status_flags.task2_contador > contador_task2_anterior)
            {
                printf("%s [SUPERVISÃO] ✓ Task2 (Recepção): ATIVA - %lu recepções\n", 
                       TAG_PREFIX, status_flags.task2_contador);
                contador_task2_anterior = status_flags.task2_contador;
            }
            else
            {
                printf("%s [SUPERVISÃO] ⚠ Task2 (Recepção): SEM DADOS RECENTES\n", TAG_PREFIX);
            }
        }
        else
        {
            printf("%s [SUPERVISÃO] ✗ Task2 (Recepção): INATIVA\n", TAG_PREFIX);
        }
        
        // Status da fila
        UBaseType_t itens_fila = uxQueueMessagesWaiting(fila);
        printf("%s [SUPERVISÃO] Fila: %d/%d itens\n", TAG_PREFIX, itens_fila, QUEUE_SIZE);
        
        printf("%s [SUPERVISÃO] ========================================\n\n", TAG_PREFIX);
        
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(5000)); // Status a cada 5 segundos
    }
}

// ======== MÓDULO 4: LOG DO SISTEMA ========
void Task4_Log(void *pv)
{
    uint32_t uptime = 0;
    
    printf("%s [LOG] Módulo de Log iniciado\n", TAG_PREFIX);
    
    while(1)
    {
        uptime++;
        printf("%s [LOG] Sistema em execução - Uptime: %lu segundos\n", 
               TAG_PREFIX, uptime * 10);
        
        // Informações do heap
        printf("%s [LOG] Memória livre: %lu bytes\n", 
               TAG_PREFIX, esp_get_free_heap_size());
        
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10000)); // Log a cada 10 segundos
    }
}

// ======== FUNÇÃO PRINCIPAL ========
void app_main(void)
{
    printf("\n\n%s ==========================================\n", TAG_PREFIX);
    printf("%s   CP 2 - SISTEMA DE DADOS ROBUSTO\n", TAG_PREFIX);
    printf("%s   Aluno: %s - RM: %s\n", TAG_PREFIX, NOME_ALUNO, RM_ALUNO);
    printf("%s ==========================================\n\n", TAG_PREFIX);
    
    // Configurar Watchdog Timer
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 5000,
        .idle_core_mask = (1 << 0) | (1 << 1),
        .trigger_panic = true
    };
    
    esp_err_t wdt_init = esp_task_wdt_init(&wdt_config);
    if(wdt_init == ESP_OK)
    {
        printf("%s [SISTEMA] Watchdog Timer configurado (5s timeout)\n", TAG_PREFIX);
    }
    else
    {
        printf("%s [SISTEMA] ERRO ao configurar Watchdog!\n", TAG_PREFIX);
    }
    
    // Criar fila
    fila = xQueueCreate(QUEUE_SIZE, sizeof(int));
    if (fila == NULL) {
        printf("%s [SISTEMA] ERRO CRÍTICO - Falha ao criar fila!\n", TAG_PREFIX);
        esp_restart();
    }
    printf("%s [SISTEMA] Fila criada com sucesso (%d posições)\n", TAG_PREFIX, QUEUE_SIZE);
    
    // Criar tarefas
    TaskHandle_t task1_handle = NULL;
    TaskHandle_t task2_handle = NULL;
    TaskHandle_t task3_handle = NULL;
    TaskHandle_t task4_handle = NULL;
    
    xTaskCreate(Task1_Geracao, "Geracao", 4096, NULL, 5, &task1_handle);
    xTaskCreate(Task2_Recepcao, "Recepcao", 4096, NULL, 5, &task2_handle);
    xTaskCreate(Task3_Supervisao, "Supervisao", 4096, NULL, 3, &task3_handle);
    xTaskCreate(Task4_Log, "Log", 4096, NULL, 2, &task4_handle);
    
    // Adicionar tarefas críticas ao watchdog
    if (task1_handle != NULL) {
        esp_task_wdt_add(task1_handle);
        printf("%s [SISTEMA] Task1 (Geração) adicionada ao WDT\n", TAG_PREFIX);
    }
    if (task2_handle != NULL) {
        esp_task_wdt_add(task2_handle);
        printf("%s [SISTEMA] Task2 (Recepção) adicionada ao WDT\n", TAG_PREFIX);
    }
    if (task3_handle != NULL) {
        esp_task_wdt_add(task3_handle);
        printf("%s [SISTEMA] Task3 (Supervisão) adicionada ao WDT\n", TAG_PREFIX);
    }
    if (task4_handle != NULL) {
        esp_task_wdt_add(task4_handle);
        printf("%s [SISTEMA] Task4 (Log) adicionada ao WDT\n", TAG_PREFIX);
    }
    
    printf("\n%s [SISTEMA] ✓ Sistema iniciado com sucesso!\n\n", TAG_PREFIX);
}