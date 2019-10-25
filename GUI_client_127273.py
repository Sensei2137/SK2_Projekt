from tkinter import *
import socket, time, struct, string, sys, os, threading,signal,re
from functools import partial


#dekodowanie listy uzytkownikow, zwraca slownik z socketem oraz ip 
def unzipList(recivedList):
    userlist=[]
    c=""
    d=""
    socketlist=[]
    newuserlist=[]
    finallist={}
    for i in recivedList:
            if(i!=";"):
                c+=i
            else:
                userlist.append(c)
                c=""
    for i in userlist:
        for c in i:
            if(c!="$"):
                d+=c
            else:
                socketlist.append(d)
                d=""
                break
    for i,c in zip(userlist,socketlist):
        newuserlist.append(i.replace(c+"$",""))
    for i,c in zip(newuserlist,socketlist):
        finallist[c]=i
    return finallist


#watek odbierajacy wiadomosc od serwera, wylaczajaca lub liste aktywnych wezlow
def thread_function(client,listbox):
    while(1):
        #blokujace wywolanie recv
        data = client.recv(60)
        msg = data.decode("utf-8")
        if(msg[0]=="s"):
            print("wylaczam")
            #zamkniecie socketu
            client.close()
            #shudown -k symuluje zamkniecie systemu operacyjnego
            os.system("shutdown -k")
            #zamkniecie programu
            os.kill(os.getpid(), signal.SIGUSR1)
        else:
            #wyswietlenie listy wezlow 
            clients_list = unzipList(msg)
            listbox.delete(1, END)
            for item in clients_list:
                listbox.insert(END, '{}: {}'.format(item, clients_list[item]))
        
#polaczenie z serwerem
def connection(s_name,p_name):
    #inicjalizacja gniazda
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #nazwa serwera oraz port podawane jako argumenty
    server_name = s_name
    port_name = p_name
    #polaczenie z serwerem
    client.connect((server_name, port_name))
    #zwraca socket
    return client

#utworzenie GUI
class App:
    def __init__(self, master):
        #polaczenie z serwerem (argument pierwszy - nazwa hosta, argument drugi - numer portu)
        client = connection(sys.argv[1],int(sys.argv[2]))
        #perrmision = 1 proces "zombie" | inna liczba proces moze zamykac inne wezly
        perrmission = int(sys.argv[3])
        
        #inicjalizacja okienka
        self.frame = Frame(master)
        self.frame.pack()
        
        #listbox do wyswietlania polaczonych klientow
        self.listbox = Listbox(master)
        self.listbox.pack()
        self.listbox.insert(END, "socket | ip")
        
        #inicjalizacja watku odbierajacy wiadomosci
        recv_thread = threading.Thread(target = thread_function,args=(client,self.listbox))
        #daemon = watek zamknie sie wraz z koncem programu
        recv_thread.daemon = True
        #start watku
        recv_thread.start()
        
        #pole entry do wprowadzenia nr socketu do wylaczenia
        self.socket_entry = Entry(self.frame)
        self.socket_entry.pack(side=LEFT)
        
        #button do czyszczenia pola socket_entry
        self.clear_button = Button(self.frame, text="Wyczysc", fg="blue", command=self.clear_entry)
        self.clear_button.pack(side=LEFT)
        
        #button do zamykania wybranych wezlow
        self.close_button = Button(self.frame, text="Zamknij",command=partial(self.close_socket, client))
        self.close_button.pack(side=LEFT)
       
        #button do wyswietlania listy aktywnych wezlow
        self.view_button = Button(self.frame, text="Wyswietl",command=partial(self.view_clients, client))
        self.view_button.pack(side=LEFT)
        
        #button do wylacznia klienta
        self.quit_button = Button(self.frame, text="QUIT", fg="red", command=partial(self.close_connection, client))
        self.quit_button.pack(side=RIGHT)
        
        #proces z flaga 1 jedynie istnieje w systemie, nie moze wylaczac innnych wezlow
        if (perrmission==1):
            self.close_button.config(state="disabled")
            self.socket_entry.config(state="disabled")
            self.clear_button.config(state="disabled")
    
    
    #definicje funkcji 
    
    
    #funkcja obslugujaca wyswietlanie listy aktywnych wezlow
    def view_clients(self,client):
        #polecenie dla serwera - wyslij liste wezlow
        client.send("2".encode())        
        
    #funkcja obslugujaca czyszczenie pola do wprowadzania socketu
    def clear_entry(self):
        self.socket_entry.delete(0,END)
        
    #funkcja obslugujaca zamkniecie danego wezla
    def close_socket(self,client):
        text=self.socket_entry.get()
        #wyczyszczenie pola socket_entry
        self.socket_entry.delete(0,END)
        #sprawdzenie, czy uzytkownik nie podal litery badz pustego pola
        if ((re.findall("[a-zA-Z]", text)) or (text=="")):
            self.socket_entry.insert(0,"wpisz poprawny numer socketu")
        else:
            # 1 - polecenie "wylacz" dla serwera 
            text="1$"+text
            client.send(text.encode())
            
    #funkcja obslugujaca zamykanie klienta
    def close_connection(self, client):
        #3 - polecenie rozlacz dla serwera
        client.send("3".encode())
        #zamkniecie soketu
        client.close()
        #wylaczenie programu
        self.frame.quit()

#Start aplikacji
root = Tk()
app = App(root)


root.mainloop()

root.destroy() 
