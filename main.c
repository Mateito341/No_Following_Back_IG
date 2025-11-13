//Compilar: gcc -o main main.c -lm
//Ejecutar: ./main
//recordar que los archivos deben estar en formato json y debe tener toda la antiguedad

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LONGITUD 100
#define TAM_INICIAL 1009
#define PASO 2

// ============== ESTRUCTURAS ==============

typedef struct Nodo {
    char clave[MAX_LONGITUD];
    struct Nodo *siguiente;
} Nodo;

typedef struct _TablaHash {
    Nodo **datos;
    int tamano, carga;
} TablaHash;

// ============== FUNCIONES TABLA HASH ==============

unsigned int funcionHash(TablaHash *t, const char *clave) {
    unsigned int hash = 0;
    for (int i = 0; clave[i] != '\0'; i++) {
        hash = hash * 31 + clave[i];
    }
    return hash % t->tamano;
}

TablaHash *crearTablaHash() {
    TablaHash *t = malloc(sizeof(TablaHash));
    if (t == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para la tabla hash\n");
        return NULL;
    }

    t->tamano = TAM_INICIAL;
    t->datos = calloc(t->tamano, sizeof(Nodo *));

    if (t->datos == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para los datos\n");
        free(t);
        return NULL;
    }

    t->carga = 0;
    return t;
}

TablaHash *redimensionar(TablaHash *t) {
    int nuevoTamano = t->tamano * PASO;
    Nodo **nuevoDatos = calloc(nuevoTamano, sizeof(Nodo *));
    
    if (nuevoDatos == NULL) {
        fprintf(stderr, "Advertencia: No se pudo redimensionar la tabla hash\n");
        return t;
    }

    int antiguoTamano = t->tamano;
    t->tamano = nuevoTamano;

    for (int i = 0; i < antiguoTamano; i++) {
        Nodo *actual = t->datos[i];
        while (actual != NULL) {
            Nodo *temp = actual;
            actual = actual->siguiente;

            int nuevoIndice = funcionHash(t, temp->clave);
            temp->siguiente = nuevoDatos[nuevoIndice];
            nuevoDatos[nuevoIndice] = temp;
        }
    }

    free(t->datos);
    t->datos = nuevoDatos;
    return t;
}

void insertarElemento(TablaHash *t, const char *clave) {
    if (t == NULL || t->datos == NULL || clave == NULL) {
        return;
    }

    // Verificar si ya existe
    int indice = funcionHash(t, clave);
    Nodo *actual = t->datos[indice];
    while (actual != NULL) {
        if (strcmp(actual->clave, clave) == 0) {
            return; // Ya existe, no insertar duplicado
        }
        actual = actual->siguiente;
    }

    // Redimensionar si es necesario
    if ((t->carga / (double)t->tamano) >= 0.7) {
        t = redimensionar(t);
        indice = funcionHash(t, clave); // Recalcular índice después de redimensionar
    }

    Nodo *nuevoNodo = malloc(sizeof(Nodo));
    if (nuevoNodo == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nodo\n");
        return;
    }
    
    strncpy(nuevoNodo->clave, clave, MAX_LONGITUD - 1);
    nuevoNodo->clave[MAX_LONGITUD - 1] = '\0';
    nuevoNodo->siguiente = t->datos[indice];
    t->datos[indice] = nuevoNodo;

    t->carga++;
}

int buscar(TablaHash *t, const char *clave) {
    if (!t || !clave || !t->datos) return 0;

    int indice = funcionHash(t, clave);
    Nodo *actual = t->datos[indice];

    while (actual) {
        if (strcmp(actual->clave, clave) == 0) {
            return 1;
        }
        actual = actual->siguiente;
    }

    return 0;
}

void liberarTablaHash(TablaHash *t) {
    if (t == NULL) return;

    if (t->datos != NULL) {
        for (int i = 0; i < t->tamano; i++) {
            Nodo *actual = t->datos[i];
            while (actual != NULL) {
                Nodo *temp = actual;
                actual = actual->siguiente;
                free(temp);
            }
        }
        free(t->datos);
    }
    free(t);
}

// ============== FUNCIONES JSON ==============

char *leerArchivo(const char *nombreArchivo) {
    FILE *archivo = fopen(nombreArchivo, "r");
    if (!archivo) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", nombreArchivo);
        return NULL;
    }

    fseek(archivo, 0, SEEK_END);
    long tamano = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    char *contenido = malloc(tamano + 1);
    if (!contenido) {
        fclose(archivo);
        fprintf(stderr, "Error: No se pudo asignar memoria para leer el archivo\n");
        return NULL;
    }

    fread(contenido, 1, tamano, archivo);
    contenido[tamano] = '\0';
    fclose(archivo);

    return contenido;
}

char *extraerValor(const char *json, const char *clave) {
    char patron[256];
    snprintf(patron, sizeof(patron), "\"%s\":", clave);
    
    const char *inicio = strstr(json, patron);
    if (!inicio) return NULL;

    inicio += strlen(patron);
    while (*inicio && isspace(*inicio)) inicio++;

    if (*inicio == '"') {
        inicio++;
        const char *fin = strchr(inicio, '"');
        if (!fin) return NULL;

        int longitud = fin - inicio;
        if (longitud >= MAX_LONGITUD) longitud = MAX_LONGITUD - 1;
        
        char *valor = malloc(longitud + 1);
        if (!valor) return NULL;
        
        strncpy(valor, inicio, longitud);
        valor[longitud] = '\0';
        return valor;
    }

    return NULL;
}

int procesarFollowers(TablaHash *tabla, const char *nombreArchivo) {
    char *contenido = leerArchivo(nombreArchivo);
    if (!contenido) return 0;

    int contador = 0;
    const char *ptr = contenido;

    while ((ptr = strstr(ptr, "\"value\":")) != NULL) {
        ptr += 8; // Longitud de "\"value\":"
        while (*ptr && isspace(*ptr)) ptr++;

        if (*ptr == '"') {
            ptr++;
            const char *fin = strchr(ptr, '"');
            if (fin) {
                int longitud = fin - ptr;
                if (longitud > 0 && longitud < MAX_LONGITUD) {
                    char username[MAX_LONGITUD];
                    strncpy(username, ptr, longitud);
                    username[longitud] = '\0';
                    
                    insertarElemento(tabla, username);
                    contador++;
                }
                ptr = fin + 1;
            } else {
                break;
            }
        }
    }

    free(contenido);
    return contador;
}

char **procesarFollowing(const char *nombreArchivo, int *cantidad) {
    char *contenido = leerArchivo(nombreArchivo);
    if (!contenido) {
        *cantidad = 0;
        return NULL;
    }

    // Contar cuántos usuarios hay
    int count = 0;
    const char *ptr = contenido;
    while ((ptr = strstr(ptr, "\"title\":")) != NULL) {
        count++;
        ptr += 8;
    }

    if (count == 0) {
        free(contenido);
        *cantidad = 0;
        return NULL;
    }

    char **usuarios = malloc(count * sizeof(char *));
    if (!usuarios) {
        free(contenido);
        *cantidad = 0;
        return NULL;
    }

    // Extraer los usuarios
    ptr = contenido;
    int idx = 0;
    while ((ptr = strstr(ptr, "\"title\":")) != NULL && idx < count) {
        ptr += 8; // Longitud de "\"title\":"
        while (*ptr && isspace(*ptr)) ptr++;

        if (*ptr == '"') {
            ptr++;
            const char *fin = strchr(ptr, '"');
            if (fin) {
                int longitud = fin - ptr;
                if (longitud > 0 && longitud < MAX_LONGITUD) {
                    usuarios[idx] = malloc(longitud + 1);
                    if (usuarios[idx]) {
                        strncpy(usuarios[idx], ptr, longitud);
                        usuarios[idx][longitud] = '\0';
                        idx++;
                    }
                }
                ptr = fin + 1;
            } else {
                break;
            }
        }
    }

    free(contenido);
    *cantidad = idx;
    return usuarios;
}

void liberarArrayUsuarios(char **usuarios, int cantidad) {
    if (!usuarios) return;
    for (int i = 0; i < cantidad; i++) {
        free(usuarios[i]);
    }
    free(usuarios);
}

// ============== FUNCIÓN PRINCIPAL ==============

void encontrarNoFollowBack(TablaHash *seguidores, char **siguiendo, int cantSiguiendo) {
    if (!seguidores || !siguiendo) {
        fprintf(stderr, "Error: Datos inválidos\n");
        return;
    }

    printf("\n========================================\n");
    printf("PERSONAS QUE NO TE SIGUEN DE VUELTA:\n");
    printf("========================================\n\n");

    int contador = 0;
    for (int i = 0; i < cantSiguiendo; i++) {
        if (!buscar(seguidores, siguiendo[i])) {
            printf("%d. %s\n", ++contador, siguiendo[i]);
        }
    }

    if (contador == 0) {
        printf("¡Todos te siguen de vuelta! 🎉\n");
    } else {
        printf("\n========================================\n");
        printf("Total: %d persona(s) no te siguen de vuelta\n", contador);
        printf("========================================\n");
    }
}

// ============== MAIN ==============

int main() {
    printf("Analizando tus seguidores de Instagram...\n\n");

    // Procesar archivo de following
    int cantSiguiendo = 0;
    char **siguiendo = procesarFollowing("following.json", &cantSiguiendo);
    
    if (!siguiendo || cantSiguiendo == 0) {
        fprintf(stderr, "Error: No se pudo leer el archivo following.json\n");
        return 1;
    }

    printf("✓ Procesados %d usuarios que sigues\n", cantSiguiendo);

    // Crear tabla hash y procesar seguidores
    TablaHash *seguidores = crearTablaHash();
    if (!seguidores) {
        liberarArrayUsuarios(siguiendo, cantSiguiendo);
        return 1;
    }

    int cantSeguidores = procesarFollowers(seguidores, "followers_1.json");
    printf("✓ Procesados %d seguidores\n", cantSeguidores);

    // Encontrar quién no te sigue de vuelta
    encontrarNoFollowBack(seguidores, siguiendo, cantSiguiendo);

    // Liberar memoria
    liberarArrayUsuarios(siguiendo, cantSiguiendo);
    liberarTablaHash(seguidores);

    return 0;
}