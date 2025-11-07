/**
 * @file main.cpp
 * @brief Implementación del Decodificador PRT-7 para tramas de comunicación serial.
 *
 * Este programa simula el funcionamiento de un decodificador de mensajes basado en un
 * rotor de cifrado tipo Enigma simple. Lee tramas desde un puerto serial,
 * las parsea en operaciones de carga de caracteres ('L') o rotación del rotor ('M'),
 * procesa cada trama y acumula el mensaje decodificado.
 *
 * @author Gemini
 * @date 06 de Noviembre de 2025
 */

#include <iostream>
#include <fcntl.h>      // Para open()
#include <unistd.h>     // Para read(), write(), close()
#include <termios.h>    // Para configuración del puerto serial
#include <cstring>      // Para memset()

using namespace std;

// ESTRUCTURAS DE NODOS

/**
 * @struct NodoRotor
 * @brief Nodo para la lista doblemente enlazada circular que representa el rotor de mapeo.
 */
struct NodoRotor {
    char dato;             ///< Carácter almacenado en el nodo (A-Z).
    NodoRotor* siguiente;  ///< Puntero al siguiente nodo.
    NodoRotor* previo;     ///< Puntero al nodo previo.
    
    /**
     * @brief Constructor del nodo del rotor.
     * @param d El carácter inicial para el nodo.
     */
    NodoRotor(char d) : dato(d), siguiente(nullptr), previo(nullptr) {}
};

/**
 * @struct NodoCarga
 * @brief Nodo para la lista doblemente enlazada que almacena la carga/mensaje decodificado.
 */
struct NodoCarga {
    char dato;             ///< Carácter decodificado almacenado.
    NodoCarga* siguiente;  ///< Puntero al siguiente nodo.
    NodoCarga* previo;     ///< Puntero al nodo previo.
    
    /**
     * @brief Constructor del nodo de carga.
     * @param d El carácter inicial para el nodo.
     */
    NodoCarga(char d) : dato(d), siguiente(nullptr), previo(nullptr) {}
};

// CLASE: ROTOR DE MAPEO

/**
 * @class RotorDeMapeo
 * @brief Implementa el mecanismo de cifrado/descifrado mediante un rotor circular.
 *
 * Simula el rotor de una máquina de cifrado. Está compuesto por una lista
 * doblemente enlazada circular con los caracteres 'A' a 'Z'. La rotación
 * cambia el punto de inicio del mapeo (la 'cabeza').
 */
class RotorDeMapeo {
private:
    NodoRotor* cabeza; ///< Puntero al nodo actual que representa el inicio del mapeo.
    
public:
    /**
     * @brief Constructor. Inicializa el rotor con el alfabeto ordenado (A-Z) en forma circular.
     */
    RotorDeMapeo() {
    const char alfabeto[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";  // SIN espacio
    int longitud = 26;  // Solo 26 letras
    
    cabeza = new NodoRotor(alfabeto[0]);
    NodoRotor* actual = cabeza;
    
    for (int i = 1; i < longitud; i++) {
        NodoRotor* nuevo = new NodoRotor(alfabeto[i]);
        actual->siguiente = nuevo;
        nuevo->previo = actual;
        actual = nuevo;
    }
    
    // Cerrar el círculo
    actual->siguiente = cabeza;
    cabeza->previo = actual;
}
    
    /**
     * @brief Destructor. Libera toda la memoria asignada a los nodos del rotor.
     */
    ~RotorDeMapeo() {
    if (cabeza == nullptr) return;
    
    NodoRotor* ultimo = cabeza->previo;
    ultimo->siguiente = nullptr; // Romper el círculo para la iteración
    
    NodoRotor* actual = cabeza;
    while (actual != nullptr) {
        NodoRotor* siguiente = actual->siguiente;
        delete actual;
        actual = siguiente;
    }
}
    
    /**
     * @brief Rota el rotor N posiciones.
     * @param N El número de posiciones a rotar. Positivo para avanzar (siguiente), negativo para retroceder (previo).
     */
    void rotar(int N) {
        if (N == 0) return;
        
        if (N > 0) {
            for (int i = 0; i < N; i++) {
                cabeza = cabeza->siguiente;
            }
        } else {
            // Rotación negativa (hacia atrás)
            N = -N;
            for (int i = 0; i < N; i++) {
                cabeza = cabeza->previo;
            }
        }
    }
    
    /**
     * @brief Obtiene el carácter de mapeo (decodificado) para el carácter de entrada.
     * @param in El carácter de entrada (cifrado). Se espera un carácter en mayúscula A-Z.
     * @return El carácter decodificado. Devuelve el mismo carácter si es un espacio o no es A-Z.
     */
    char getMapeo(char in) {
    // CASO ESPECIAL: El espacio NO se cifra, se devuelve tal cual
    if (in == ' ') {
        return ' ';
    }
    
    // Solo procesar letras A-Z
    if (in < 'A' || in > 'Z') {
        return in;
    }
    
    // La posición del carácter de entrada se calcula desde 'A'
    int posicionDelCaracter = in - 'A';
    
    // Desde la cabeza actual (que está rotada), avanzar esa cantidad de posiciones
    // para obtener el carácter de mapeo.
    NodoRotor* resultado = cabeza;
    for (int i = 0; i < posicionDelCaracter; i++) {
        resultado = resultado->siguiente;
    }
    
    return resultado->dato;
    
    // NOTA: El bucle para encontrar 'A' en la implementación original
    // ('for (int i = 0; i < 27; i++) { if (nodoA->dato == 'A') { break; } nodoA = nodoA->siguiente; }')
    // es innecesario y potencialmente incorrecto si la rotación no es un desplazamiento simple
    // o si el alfabeto no fuera A-Z, pero para el cifrado tipo Caesar/Enigma simple implementado,
    // el mapeo se define por el *desplazamiento* desde la cabeza del rotor, lo cual se simplifica
    // a la posición relativa desde 'A'. Se ha dejado la implementación simplificada aquí,
    // asumiendo que el 'mapeo' es un cifrado César modificado por la rotación.
}
};

// CLASE: LISTA DE CARGA

/**
 * @class ListaDeCarga
 * @brief Lista doblemente enlazada para almacenar el mensaje decodificado.
 */
class ListaDeCarga {
private:
    NodoCarga* cabeza; ///< Puntero al primer nodo de la lista (inicio del mensaje).
    NodoCarga* cola;   ///< Puntero al último nodo de la lista (fin del mensaje).
    
public:
    /**
     * @brief Constructor. Inicializa una lista vacía.
     */
    ListaDeCarga() : cabeza(nullptr), cola(nullptr) {}
    
    /**
     * @brief Destructor. Libera toda la memoria asignada a los nodos de la carga.
     */
    ~ListaDeCarga() {
        NodoCarga* actual = cabeza;
        while (actual != nullptr) {
            NodoCarga* siguiente = actual->siguiente;
            delete actual;
            actual = siguiente;
        }
    }
    
    /**
     * @brief Inserta un carácter al final de la lista de carga.
     * @param dato El carácter a insertar.
     */
    void insertarAlFinal(char dato) {
        NodoCarga* nuevo = new NodoCarga(dato);
        
        if (cabeza == nullptr) {
            cabeza = nuevo;
            cola = nuevo;
        } else {
            cola->siguiente = nuevo;
            nuevo->previo = cola;
            cola = nuevo;
        }
    }
    
    /**
     * @brief Imprime el mensaje completo contenido en la lista.
     */
    void imprimirMensaje() {
        NodoCarga* actual = cabeza;
        while (actual != nullptr) {
            cout << actual->dato;
            actual = actual->siguiente;
        }
        cout << endl;
    }
};

// CLASE BASE: TRAMA

/**
 * @class TramaBase
 * @brief Clase base abstracta para todos los tipos de tramas de comunicación.
 */
class TramaBase {
public:
    /**
     * @brief Método virtual puro para procesar la trama. Debe ser implementado por las clases derivadas.
     * @param carga Puntero a la lista de carga donde se almacena el mensaje.
     * @param rotor Puntero al rotor de mapeo para realizar operaciones.
     */
    virtual void procesar(ListaDeCarga* carga, RotorDeMapeo* rotor) = 0;
    
    /**
     * @brief Destructor virtual.
     */
    virtual ~TramaBase() {}
};

// CLASE: TRAMA LOAD

/**
 * @class TramaLoad
 * @brief Representa una trama de carga de carácter ('L').
 *
 * Contiene un carácter cifrado que debe ser decodificado por el rotor
 * y añadido a la lista de carga.
 */
class TramaLoad : public TramaBase {
private:
    char caracter; ///< El carácter cifrado a decodificar.
    
public:
    /**
     * @brief Constructor.
     * @param c El carácter cifrado.
     */
    TramaLoad(char c) : caracter(c) {}
    
    /**
     * @brief Procesa la trama: decodifica el carácter y lo añade a la lista de carga.
     * @param carga Puntero a la lista de carga.
     * @param rotor Puntero al rotor de mapeo.
     */
    void procesar(ListaDeCarga* carga, RotorDeMapeo* rotor) {
        char decodificado = rotor->getMapeo(caracter);
        carga->insertarAlFinal(decodificado);
        cout << "Fragmento '" << caracter << "' decodificado como '" 
             << decodificado << "'." << endl;
    }
};

// CLASE: TRAMA MAP

/**
 * @class TramaMap
 * @brief Representa una trama de mapeo/rotación ('M').
 *
 * Indica una rotación que debe aplicarse al rotor de mapeo.
 */
class TramaMap : public TramaBase {
private:
    int rotacion; ///< El valor de rotación (positivo o negativo) a aplicar.
    
public:
    /**
     * @brief Constructor.
     * @param n El valor de rotación.
     */
    TramaMap(int n) : rotacion(n) {}
    
    /**
     * @brief Procesa la trama: aplica la rotación al rotor.
     * @param carga Puntero a la lista de carga (no se utiliza, pero es requerido por la interfaz base).
     * @param rotor Puntero al rotor de mapeo.
     */
    void procesar(ListaDeCarga* carga, RotorDeMapeo* rotor) {
        rotor->rotar(rotacion);
        
        if (rotacion > 0) {
            cout << "ROTANDO ROTOR +" << rotacion << endl;
        } else {
            cout << "ROTANDO ROTOR " << rotacion << endl;
        }
    }
};

// FUNCIÓN: CONFIGURAR PUERTO SERIAL

/**
 * @brief Configura y abre el puerto serial para la comunicación.
 * @param puerto El path del dispositivo serial (e.g., "/dev/ttyUSB0").
 * @return El descriptor de archivo del puerto abierto, o -1 en caso de error.
 */
int configurarSerial(const char* puerto) {
    // Abrir el puerto serial
    int fd = open(puerto, O_RDWR | O_NOCTTY);
    
    if (fd == -1) {
        return -1;
    }
    
    // Configurar parámetros del puerto
    struct termios opciones;
    
    // Obtener configuración actual
    tcgetattr(fd, &opciones);
    
    // Configurar velocidad (9600 baudios)
    cfsetispeed(&opciones, B9600);
    cfsetospeed(&opciones, B9600);
    
    // Configuración: 8N1 (8 bits, sin paridad, 1 bit de stop)
    opciones.c_cflag &= ~PARENB;  // Sin paridad
    opciones.c_cflag &= ~CSTOPB;  // 1 bit de stop
    opciones.c_cflag &= ~CSIZE;   // Limpiar tamaño
    opciones.c_cflag |= CS8;      // 8 bits
    
    // Deshabilitar control de flujo hardware
    opciones.c_cflag &= ~CRTSCTS;
    
    // Habilitar lectura
    opciones.c_cflag |= CREAD | CLOCAL;
    
    // Modo raw (sin procesamiento)
    opciones.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    opciones.c_iflag &= ~(IXON | IXOFF | IXANY);
    opciones.c_oflag &= ~OPOST;
    
    // Aplicar configuración
    tcsetattr(fd, TCSANOW, &opciones);
    
    // Esperar un momento para que se estabilice
    usleep(100000); // 100ms
    
    return fd;
}

// FUNCIÓN: LEER LÍNEA DEL PUERTO SERIAL

/**
 * @brief Lee una línea de texto desde el puerto serial.
 * @param fd Descriptor de archivo del puerto serial.
 * @param buffer Buffer donde se almacenará la línea leída.
 * @param maxLen Longitud máxima del buffer.
 * @return true si se leyó una línea no vacía terminada por '\n' o '\r', false en caso contrario.
 */
bool leerLineaSerial(int fd, char* buffer, int maxLen) {
    int posicion = 0;
    
    while (posicion < maxLen - 1) {
        char c;
        int n = read(fd, &c, 1);
        
        if (n <= 0) {
            // No hay datos disponibles o error
            continue;
        }
        
        // Si es salto de línea, terminamos
        if (c == '\n' || c == '\r') {
            if (posicion > 0) {
                buffer[posicion] = '\0';
                return true;
            }
            continue;
        }
        
        // Agregar carácter al buffer
        buffer[posicion++] = c;
    }
    
    buffer[posicion] = '\0';
    return posicion > 0;
}

// FUNCIÓN: PARSEAR LÍNEA

/**
 * @brief Parsea una línea de texto (trama) y crea el objeto TramaBase correspondiente.
 * @param linea La cadena de texto de la trama (e.g., "L,A" o "M,-5").
 * @return Un puntero a un nuevo objeto TramaBase (TramaLoad o TramaMap) o nullptr si la trama está mal formada.
 * @note El objeto retornado debe ser liberado con `delete`.
 */
TramaBase* parsearLinea(char* linea) {
    int longitud = 0;
    while (linea[longitud] != '\0') {
        longitud++;
    }
    
    if (longitud < 3) return nullptr; // Trama mínima "L,X" o "M,N" (M,N requiere al menos 3)
    
    char tipo = linea[0];
    
    if (linea[1] != ',') return nullptr;
    
    if (tipo == 'L') {
        // Trama: L,X
        char caracter = linea[2];
        return new TramaLoad(caracter);
    }
    else if (tipo == 'M') {
        // Trama: M,N o M,-N
        int numero = 0;
        int signo = 1;
        int indice = 2;
        
        if (linea[indice] == '-') {
            signo = -1;
            indice++;
        }
        
        while (linea[indice] >= '0' && linea[indice] <= '9') {
            numero = numero * 10 + (linea[indice] - '0');
            indice++;
        }
        
        if (indice == 2 || (signo == -1 && indice == 3)) {
             // Caso de M, o M,- (sin número)
             return nullptr; 
        }
        
        numero *= signo;
        return new TramaMap(numero);
    }
    
    return nullptr;
}

// FUNCIÓN PRINCIPAL

/**
 * @brief Punto de entrada principal del programa.
 *
 * Inicializa las estructuras de datos, solicita el puerto serial al usuario,
 * establece la conexión y entra en un bucle para leer, parsear y procesar
 * las tramas seriales hasta recibir la señal "FIN".
 * @return 0 si la ejecución finaliza con éxito, 1 en caso de error de conexión.
 */
int main() {
    cout << "  DECODIFICADOR PRT-7" << endl;
    
    // Crear estructuras de datos
    ListaDeCarga miListaDeCarga;
    RotorDeMapeo miRotorDeMapeo;
    
    // Pedir puerto
    cout << "Ingrese el puerto serial del Arduino:" << endl;
    cout << "(Puerto: /dev/ttyUSB0)" << endl;
    
    char puerto[50];
    cin.getline(puerto, 50);
    
    // Configurar y abrir puerto serial
    cout << endl << "Conectando al puerto " << puerto << "..." << endl;
    int fd = configurarSerial(puerto);
    
    if (fd == -1) {
        cout << endl << "ERROR: No se pudo abrir el puerto " << puerto << endl;
        return 1;
    }
    
    cout << "Conexion establecida!" << endl;
    
    // Buffer para leer líneas
    char linea[100];
    
    // Bucle principal
    while (true) {
        // Leer una línea del puerto serial
        if (!leerLineaSerial(fd, linea, 100)) {
            continue;
        }
        
        // Ignorar líneas vacías
        if (linea[0] == '\0') continue;
        
        // Verificar señales especiales (I para Inicio, FIN para Final)
        if (linea[0] == 'I') {
            cout << "--- Inicio de transmision ---" << endl << endl;
            continue;
        }
        if (linea[0] == 'F' && linea[1] == 'I' && linea[2] == 'N') {
            cout << endl << "--- Fin de transmision ---" << endl;
            break;
        }
        
        // Mostrar trama recibida
        cout << "Trama: [" << linea << "] -> ";
        
        // Parsear y crear trama
        TramaBase* trama = parsearLinea(linea);
        
        if (trama == nullptr) {
            cout << "ERROR: Trama mal formada" << endl;
            continue;
        }
        
        // Procesar (polimorfismo)
        trama->procesar(&miListaDeCarga, &miRotorDeMapeo);
        
        // Liberar memoria
        delete trama;
        
        cout << endl;
    }
    
    // Cerrar puerto
    close(fd);
    
    // Mostrar resultado final
    cout << "  --- Mensaje Decodificado ---:" << endl;
    miListaDeCarga.imprimirMensaje();    
    cout << endl << "Sistema apagado correctamente." << endl;
    
    return 0;
}