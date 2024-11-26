#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define SHM_NAME "/shared_memory"
#define SEM_NAME "/semaphore"
#define MAX_COUNT 1000

void process_logic(int id) {
    // Deschide semaforul
    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    // Deschide memoria partajată
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    // Maparea memoriei partajate
    int *shared_value = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_value == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    // Procesul începe să citească și să scrie
    while (1) {
        sem_wait(sem); // Așteaptă accesul la semafor

        if (*shared_value >= MAX_COUNT) {
            sem_post(sem); // Eliberează semaforul
            break;
        }

        // Dă cu banul
        if (rand() % 2 == 1) {
            (*shared_value)++;
            printf("Proces %d a incrementat valoarea la %d\n", id, *shared_value);
        }

        sem_post(sem); // Eliberează semaforul

        // Simulează o întârziere
        usleep(rand() % 100000);
    }

    // Curățare
    munmap(shared_value, sizeof(int));
    close(shm_fd);
    sem_close(sem);
}

int main() {
    srand(time(NULL));

    // Creează semaforul
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    // Creează memoria partajată
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }

    // Alocă dimensiunea memoriei partajate
    if (ftruncate(shm_fd, sizeof(int)) == -1) {
        perror("ftruncate failed");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }

    // Maparea memoriei partajate
    int *shared_value = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_value == MAP_FAILED) {
        perror("mmap failed");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }

    // Inițializează valoarea partajată
    *shared_value = 0;

    // Creează procesele
    pid_t pid1 = fork();
    if (pid1 == 0) {
        // Procesul copil 1
        process_logic(1);
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Procesul copil 2
        process_logic(2);
        exit(0);
    }

    // Așteaptă finalizarea proceselor copil
    wait(NULL);
    wait(NULL);

    printf("Numărarea s-a terminat. Valoarea finală: %d\n", *shared_value);

    // Curățare
    munmap(shared_value, sizeof(int));
    close(shm_fd);
    shm_unlink(SHM_NAME);
    sem_close(sem);
    sem_unlink(SEM_NAME);

    return 0;
}
