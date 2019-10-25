#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <iostream>
#include <vector>
#define SERVER_PORT 1234
#define QUEUE_SIZE 5
#define BUF_SIZE 1024

using namespace std;

//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
	int socket;
	char message[BUF_SIZE];
};

//Klasa klient - wezel podlaczony
class Client
{
    private:
        int socket;
        string ip;
    public:
    //Funkcje get oraz set
        void setSocket(int _socket)
            {
                socket = _socket;
            }
        int getSocket()
            {
                return socket;
            } 
        void setIP(string _ip)
            {
                ip = _ip;
            }   
        string getIP()
        {
            return ip;
        }
};

//Wektor klientow, do ktorego zapisywane sa podlaczone wezly
vector  <Client> clients;

//Mutex uzywany przy dopisywaniu klientow do wektora
pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;

//Funkcja przygotowujaca liste wszystkich podlaczonych klientow do wyslania 
string ParseClientList(vector<Client> clients)
{
    string clientList = "";
    for( int i = 0; i < clients.size(); i++ )
     {
         clientList+=to_string(clients[i].getSocket()) + "$" + clients[i].getIP() + ";";
     }
     return clientList;
}

//Funkcja usuwajaca wylaczonego klienta z wektora
vector<Client> remove_from_vector(vector<Client> clients, int to_close_socket)
{
    for( int i = 0; i < clients.size(); i++ )
    {
        if(clients[i].getSocket()==to_close_socket)
            clients.erase(clients.begin()+i);
    }
    return clients;
}

//Wyekstrachowanie socketu z wiadomosci od klienta
int take_socket(char msg[BUF_SIZE])
{
    int socket;
    string to_close_socket="";
    int i = 2;
   
        while(msg[i]!='\u0000')
        {
            to_close_socket+=msg[i];
            i++;
        }
        socket = atoi(to_close_socket.c_str());
    
    return socket;
}

//Wyekstrachowanie polecenia z wiadomosci od klienta
int take_command(char msg[BUF_SIZE])
{
    int result;
    string first_part="";
    first_part +=msg[0];
    result = atoi(first_part.c_str());
    
    return result;
}

//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ThreadBehavior(void *t_data)
{
    int sock; // zmienna socketu
    int command; //zmienna kierujaca instruckja switch
    int close_socket; //zmienna przechowujaca socekt do wylaczenia 
    struct sockaddr_in name; 
    socklen_t len = sizeof(name); 
    char buf[INET_ADDRSTRLEN] = ""; //zmienna przechowujaca adress ip
    pthread_detach(pthread_self());
    
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    //utworzenie obiektu klasy client i wypelnienie pola socket
    Client client;
    client.setSocket(th_data->socket);
    
    //przypisanie socketu do zmiennej
    sock = client.getSocket();
    //pobranie adresu klienta
    
    getpeername(sock, (struct sockaddr *)&name, &len);
    inet_ntop(AF_INET, &name.sin_addr, buf, sizeof buf);
    
    //wypelnienie pola ip
    client.setIP(buf);
    //wyczyszczenie bufora
    memset((th_data->message),0,BUF_SIZE);
    //dodanie klienta do wektora
    clients.push_back(client);
    //zwolnenie mutexa
    pthread_mutex_unlock(&clientsMutex);
    //wypisanie obecnego rozmiaru wektora klientow
    printf("rozmiar vecotra %s\n",to_string(clients.size()).c_str());
    
while(1){
        //wyczyszczenie bufora
        memset((th_data->message),0,BUF_SIZE);
        //oczekiwanie na polecenie 
        while(read(client.getSocket(), th_data->message, BUF_SIZE)<0);
        //wyekstrachowanie ewentualnego socketu do zamkniecia
        close_socket = take_socket(th_data->message);
        //wyekstrachowanie polecenia
        command = take_command(th_data->message);
        switch(command)
        {
            //Wylaczanie danego wezla 
            case 1:
            {
                //wyslanie polecenia do klienta
                write(close_socket,"sys",BUF_SIZE);
                 //usuwanie klienta z wektora
                pthread_mutex_lock(&clientsMutex);
                clients =  remove_from_vector(clients,close_socket);
                pthread_mutex_unlock(&clientsMutex);
                //wypisanie obecnego rozmiaru wektora klientow
                printf("rozmiar vectora %s\n",to_string(clients.size()).c_str());
                break;
            }
            //wyslanie listy podlaczonych klientow
            case 2:
            {
                //Parsowanie listy
                string listToSend = ParseClientList(clients);
                //Wyslanie listy do klienta
                write(sock,listToSend.c_str(),12*clients.size());
                //wypisanie obecnego rozmiaru wektora klientow
                printf("rozmiar vectora %s\n",to_string(clients.size()).c_str());
                break;
            }
            //wylogowanie klienta
            case 3:
                //usuwanie klienta z wektora
                pthread_mutex_lock(&clientsMutex);
                clients =  remove_from_vector(clients,sock);
                pthread_mutex_unlock(&clientsMutex);
                //wypisanie obecnego rozmiaru wektora klientow
                printf("rozmiar vectora %s\n",to_string(clients.size()).c_str());
                break;
            default:
                break;
        }
}
   
    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor) {
    //wynik funkcji tworzącej wątek
    int create_result = 0;
    //uchwyt na wątek
    pthread_t thread1;

    //dane, które zostaną przekazane do wątku
	thread_data_t * t_data = new thread_data_t;
    //Przekazanie socket'a
	t_data->socket = connection_socket_descriptor;
    //Zablokowanie mutex'a
    pthread_mutex_lock(&clientsMutex);
    create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)t_data);
    if (create_result){
       printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       exit(-1);
    }
}

int main(int argc, char* argv[])
{
   int server_socket_descriptor;
   int connection_socket_descriptor;
   int bind_result;
   int listen_result;
   char reuse_addr_val = 1;
   struct sockaddr_in server_address;

   //inicjalizacja gniazda serwera
   
   memset(&server_address, 0, sizeof(struct sockaddr));
   server_address.sin_family = AF_INET;
   server_address.sin_addr.s_addr = htonl(INADDR_ANY);
   server_address.sin_port = htons(SERVER_PORT);

   server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
   if (server_socket_descriptor < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
       exit(1);
   }
   setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

   bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
   if (bind_result < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
       exit(1);
   }

   listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
   if (listen_result < 0) {
       fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
       exit(1);
   }

   while(1)
   {
       connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
       if (connection_socket_descriptor < 0)
       {
           fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
           exit(1);
       }
        
       handleConnection(connection_socket_descriptor);
   }

   close(server_socket_descriptor);
   return(0);
}
