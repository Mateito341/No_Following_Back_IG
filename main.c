//Ejecutar gcc -o main main.c $(pkg-config --cflags --libs libxml-2.0)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#define MAX_LONGITUD 30

typedef struct Nodo {
    char clave[MAX_LONGITUD]; // Almacenamos cadenas como clave
    struct Nodo *siguiente;
} Nodo;

typedef struct _TablaHash {
    Nodo **datos; // Arreglo de punteros a nodos
    int tamano, carga;
} TablaHash;

#define TAM_INICIAL 1009
#define PASO 2

int funcionHash(TablaHash *t, const char *clave) {
    unsigned int hash = 0;
    for (int i = 0; clave[i] != '\0'; i++) {
        hash += clave[i];
    }
    return hash % t->tamano;
}

TablaHash *crearTablaHash() {
    TablaHash *t = malloc(sizeof(TablaHash));
    if (t == NULL) {
        printf("Error al asignar memoria para la tabla hash\n");
        return NULL;
    }

    t->tamano = TAM_INICIAL;
    t->datos = malloc(sizeof(Nodo *) * t->tamano);

    if (t->datos == NULL) {
        printf("Error al asignar memoria para los datos\n");
        free(t);
        return NULL;
    }

    for (int i = 0; i < TAM_INICIAL; i++) {
        t->datos[i] = NULL;
    }

    t->carga = 0;
    return t;
}

TablaHash *redimensionar(TablaHash *t) {
    int nuevoTamano = t->tamano * PASO;
    Nodo **nuevoDatos = malloc(sizeof(Nodo *) * nuevoTamano);
    if (nuevoDatos == NULL) {
        printf("Error al asignar memoria para la nueva tabla hash\n");
        return t;
    }

    for (int i = 0; i < nuevoTamano; i++) {
        nuevoDatos[i] = NULL;
    }

    for (int i = 0; i < t->tamano; i++) {
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
    t->tamano = nuevoTamano;
    return t;
}

void insertarElemento(TablaHash *t, const char *clave) {
    if (t == NULL || t->datos == NULL) {
        return;
    }

    if ((t->carga / (double)t->tamano) >= 0.7) {
        t = redimensionar(t);
    }

    int indice = funcionHash(t, clave);

    Nodo *nuevoNodo = malloc(sizeof(Nodo));
    if (nuevoNodo == NULL) {
        printf("Error al asignar memoria para el nodo\n");
        return;
    }
    strncpy(nuevoNodo->clave, clave, MAX_LONGITUD - 1);
    nuevoNodo->clave[MAX_LONGITUD - 1] = '\0'; // Garantiza que esté terminada
    nuevoNodo->siguiente = t->datos[indice];
    t->datos[indice] = nuevoNodo;

    t->carga++;
}

int contar(const char *nombreArchivo) {
    htmlDocPtr doc = htmlReadFile(nombreArchivo, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) {
        printf("Error al leer el archivo HTML\n");
        return 0;
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        xmlFreeDoc(doc);
        return 0;
    }

    const char *xpathExpression = "//div[@class='_a6-p']//a[contains(@href, 'instagram.com')]";
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar *)xpathExpression, xpathCtx);
    if (!xpathObj) {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return 0;
    }

    int totalNodos = xpathObj->nodesetval->nodeNr;
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    return totalNodos;
}

char **procesarSeguidosHTML(const char *nombreArchivo, int *cantidad) {
    htmlDocPtr doc = htmlReadFile(nombreArchivo, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) {
        printf("Error al leer el archivo HTML\n");
        *cantidad = 0;
        return NULL;
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        xmlFreeDoc(doc);
        *cantidad = 0;
        return NULL;
    }

    const char *xpathExpression = "//div[@class='_a6-p']//a[contains(@href, 'instagram.com')]";
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar *)xpathExpression, xpathCtx);
    if (!xpathObj) {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        *cantidad = 0;
        return NULL;
    }

    int totalNodos = xpathObj->nodesetval->nodeNr;
    char **nombres = malloc(totalNodos * sizeof(char *));
    *cantidad = 0;

    for (int i = 0; i < totalNodos; i++) {
        xmlNodePtr node = xpathObj->nodesetval->nodeTab[i];
        xmlChar *nombre = xmlNodeGetContent(node);
        if (nombre) {
            nombres[*cantidad] = strdup((char *)nombre);
            (*cantidad)++;
            xmlFree(nombre);
        }
    }

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    return nombres;
}

void procesarSeguidoresHTML(TablaHash *tabla, const char *nombreArchivo) {
    int cantidad = 0;
    char **nombres = procesarSeguidosHTML(nombreArchivo, &cantidad);
    if (!nombres) return;

    for (int i = 0; i < cantidad; i++) {
        insertarElemento(tabla, nombres[i]);
        free(nombres[i]);
    }
    free(nombres);
}

void liberarNombres(char **nombres, int cantidad) {
    for (int i = 0; i < cantidad; i++) {
        free(nombres[i]);
    }
    free(nombres);
}

//devuelve 1 si lo encontro (verdadero)
int buscar(TablaHash *t, const char *clave) {
    if (!t || !clave) return 0;

    int indice = funcionHash(t, clave);
    Nodo *actual = t->datos[indice];

    while (actual) {
        if (strcmp(actual->clave, clave) == 0) {
            return 1; // Encontrado
        }
        actual = actual->siguiente;
    }

    return 0; // No encontrado
}

void no_follow_back(TablaHash *t, char **seguidos, int cantSeg) {
    if (!t || !seguidos) {
        printf("Error: Tabla hash o lista de seguidos no válida.\n");
        return;
    }

    printf("Personas que sigues pero no te siguen de vuelta:\n");
    for (int i = 0; i < cantSeg; i++) {
        // Si NO está en la tabla hash de seguidores, significa que no te sigue
        if (!buscar(t, seguidos[i])) {
            printf("%s\n", seguidos[i]);
        }
    }
}


void liberarTablaHash(TablaHash *t) {
    if (t == NULL) {
        return;
    }

    for (int i = 0; i < t->tamano; i++) {
        Nodo *actual = t->datos[i];
        while (actual != NULL) {
            Nodo *temp = actual;
            actual = actual->siguiente;
            free(temp);
        }
    }
    free(t->datos);
    free(t);
}

//verificar que se haya almacenado bien los seguidos
void printearSeguidos(char **seguidos, int cantSeguidos){
    for(int i = 0; i < cantSeguidos; i++){
        printf("%s", seguidos[i]);
    }
}

//verificar que se haya almacenado bien los seguidores
void printearHash(TablaHash *t) {
    if (!t || !t->datos) {
        printf("La tabla hash está vacía o no inicializada.\n");
        return;
    }

    printf("Contenido de la tabla hash:\n");
    for (int i = 0; i < t->tamano; i++) {
        Nodo *actual = t->datos[i];
        if (actual) {
            printf("Índice %d: ", i);
            while (actual) {
                printf("%s -> ", actual->clave);
                actual = actual->siguiente;
            }
            printf("NULL\n");
        }
    }
}

int main() {
    int cantidadSeguidos;
    char **seguidos = procesarSeguidosHTML("following.html", &cantidadSeguidos);
    //printearSeguidos(seguidos, cantidadSeguidos);

    if (!seguidos) {
        return 1;
    }

    TablaHash *tabla = crearTablaHash();
    if (!tabla) {
        liberarNombres(seguidos, cantidadSeguidos);
        return 1;
    }

    procesarSeguidoresHTML(tabla, "followers_1.html");
    //printearHash(tabla);
    no_follow_back(tabla, seguidos, cantidadSeguidos);

    liberarNombres(seguidos, cantidadSeguidos);
    liberarTablaHash(tabla);
    return 0;
}
